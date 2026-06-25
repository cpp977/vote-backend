/**
 *
 *  AuthController.cc
 *  Implementation of authentication endpoints.
 *
 *  Flow:
 *    /register  – create a new user, hash password with argon2.
 *    /login     – verify credentials, issue access + refresh tokens.
 *    /logout    – revoke the given refresh token.
 *    /refresh   – exchange a valid refresh token for a new token pair.
 */

#include "vote-backend/controllers/AuthController.h"

#include <argon2.h>
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>
#include <fmt/core.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <fstream>
#include <iomanip>
#include <sstream>

#include "vote-backend/models/Users.h"
#include "vote-backend/utils/JwtService.h"

using namespace drogon;
using namespace drogon_model::vote;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {
/**
 * @brief Hash a password using Argon2id.
 * @param password The plaintext password.
 * @return The encoded Argon2 hash string (includes salt, parameters, hash).
 * @throws std::runtime_error on failure.
 */
std::string hash_password(const std::string& password) {
  // Argon2id parameters: t_cost=2, m_cost=65536 (64 MiB), parallelism=1
  constexpr uint32_t t_cost = 2;
  constexpr uint32_t m_cost = 1 << 16;  // 64 MiB
  constexpr uint32_t parallelism = 1;
  constexpr size_t salt_len = 16;
  constexpr size_t hash_len = 32;

  // Generate a random salt using OpenSSL
  unsigned char salt_buf[salt_len];
  if (RAND_bytes(salt_buf, static_cast<int>(salt_len)) != 1)
    throw std::runtime_error("RAND_bytes failed for salt generation");

  // Determine required buffer size for the encoded hash
  size_t encoded_len = argon2_encodedlen(
      t_cost, m_cost, parallelism, static_cast<uint32_t>(salt_len),
      static_cast<uint32_t>(hash_len), Argon2_id);
  std::string encoded(encoded_len, '\0');

  int rc = argon2id_hash_encoded(t_cost, m_cost, parallelism, password.c_str(),
                                 password.size(), salt_buf, salt_len, hash_len,
                                 encoded.data(), encoded.size());

  if (rc != ARGON2_OK) {
    throw std::runtime_error(std::string("argon2id_hash_encoded failed: ") +
                             argon2_error_message(rc));
  }
  // argon2id_hash_encoded writes a null-terminated string into encoded
  return std::string(encoded.data());
}

/**
 * @brief Verify a password against an Argon2 hash.
 * @param password The plaintext password.
 * @param hash The encoded Argon2 hash.
 * @return true if the password matches.
 */
bool verify_password(const std::string& password, const std::string& hash) {
  int rc = argon2id_verify(hash.c_str(), password.c_str(), password.size());
  return rc == ARGON2_OK;
}

/**
 * @brief Compute SHA-256 hex digest of a string.
 * Used to store a hash of refresh tokens (we don't store raw tokens).
 */
std::string sha256_hex(const std::string& input) {
  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int hash_len = 0;

  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
  EVP_DigestUpdate(ctx, input.data(), input.size());
  EVP_DigestFinal_ex(ctx, hash, &hash_len);
  EVP_MD_CTX_free(ctx);

  std::ostringstream oss;
  for (unsigned int i = 0; i < hash_len; ++i)
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(hash[i]);
  return oss.str();
}

/**
 * @brief Helper to send a JSON error response.
 */
void send_error(const std::function<void(const HttpResponsePtr&)>& cb,
                const std::string& msg, HttpStatusCode code) {
  Json::Value err;
  err["error"] = msg;
  auto resp = HttpResponse::newHttpJsonResponse(err);
  resp->setStatusCode(code);
  cb(resp);
}

}  // anonymous namespace

// ---------------------------------------------------------------------------
// POST /register
// ---------------------------------------------------------------------------
void AuthController::register_user(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& cb) {
  auto json = req->getJsonObject();
  if (!json) {
    send_error(cb, "Invalid JSON body", k400BadRequest);
    return;
  }

  std::string username = (*json)["username"].asString();
  std::string email = (*json)["email"].asString();
  std::string password = (*json)["password"].asString();

  // Optional fields
  bool has_birth_year =
      (*json).isMember("birth_year") && !(*json)["birth_year"].isNull();
  bool has_gender = (*json).isMember("gender") && !(*json)["gender"].isNull();
  bool has_nationality =
      (*json).isMember("nationality") && !(*json)["nationality"].isNull();

  int birth_year = has_birth_year ? (*json)["birth_year"].asInt() : 0;
  std::string gender = has_gender ? (*json)["gender"].asString() : "";
  std::string nationality =
      has_nationality ? (*json)["nationality"].asString() : "";

  // Validate gender if provided
  if (has_gender && gender != "m" && gender != "w" && gender != "d") {
    send_error(cb, "gender must be one of 'm', 'w', 'd'", k400BadRequest);
    return;
  }

  // Basic validation
  if (username.empty() || email.empty() || password.empty()) {
    send_error(cb, "username, email, and password are required",
               k400BadRequest);
    return;
  }

  if (password.size() < 8) {
    send_error(cb, "password must be at least 8 characters", k400BadRequest);
    return;
  }

  auto db = app().getDbClient();

  // Check for existing user (username or email)
  db->execSqlAsync(
      "SELECT id FROM users WHERE username = $1 OR email = $2 LIMIT 1",
      [cb, db, username, email, password, has_birth_year, birth_year,
       has_gender, gender, has_nationality,
       nationality](const drogon::orm::Result& r) {
        if (r.size() > 0) {
          send_error(cb, "username or email already exists", k409Conflict);
          return;
        }

        // Hash password and insert
        std::string pw_hash;
        try {
          pw_hash = hash_password(password);
        } catch (const std::exception& e) {
          send_error(cb, std::string("Internal error: ") + e.what(),
                     k500InternalServerError);
          return;
        }

        if (has_birth_year && has_gender && has_nationality) {
          // All optional fields present
          db->execSqlAsync(
              "INSERT INTO users (username, email, password_hash, birth_year, "
              "gender, nationality) "
              "VALUES ($1, $2, $3, $4, $5, $6) RETURNING id, username, email, "
              "birth_year, gender, nationality, created_at",
              [cb](const drogon::orm::Result& r2) {
                if (r2.size() == 0) {
                  send_error(cb, "Failed to create user",
                             k500InternalServerError);
                  return;
                }
                const auto& row = r2[0];
                Json::Value user;
                user["id"] = Json::Int64(row["id"].as<int64_t>());
                user["username"] = row["username"].as<std::string>();
                user["email"] = row["email"].as<std::string>();
                user["birth_year"] = row["birth_year"].as<int>();
                user["gender"] = row["gender"].as<std::string>();
                user["nationality"] = row["nationality"].as<std::string>();
                user["created_at"] = row["created_at"].as<std::string>();
                auto resp = HttpResponse::newHttpJsonResponse(user);
                resp->setStatusCode(k201Created);
                cb(resp);
              },
              [cb](const drogon::orm::DrogonDbException& e) {
                send_error(cb,
                           std::string("Database error: ") + e.base().what(),
                           k500InternalServerError);
              },
              username, email, pw_hash, birth_year, gender, nationality);
        } else if (has_birth_year && has_gender) {
          db->execSqlAsync(
              "INSERT INTO users (username, email, password_hash, birth_year, "
              "gender) "
              "VALUES ($1, $2, $3, $4, $5) RETURNING id, username, email, "
              "birth_year, gender, created_at",
              [cb](const drogon::orm::Result& r2) {
                if (r2.size() == 0) {
                  send_error(cb, "Failed to create user",
                             k500InternalServerError);
                  return;
                }
                const auto& row = r2[0];
                Json::Value user;
                user["id"] = Json::Int64(row["id"].as<int64_t>());
                user["username"] = row["username"].as<std::string>();
                user["email"] = row["email"].as<std::string>();
                user["birth_year"] = row["birth_year"].as<int>();
                user["gender"] = row["gender"].as<std::string>();
                user["created_at"] = row["created_at"].as<std::string>();
                auto resp = HttpResponse::newHttpJsonResponse(user);
                resp->setStatusCode(k201Created);
                cb(resp);
              },
              [cb](const drogon::orm::DrogonDbException& e) {
                send_error(cb,
                           std::string("Database error: ") + e.base().what(),
                           k500InternalServerError);
              },
              username, email, pw_hash, birth_year, gender);
        } else if (has_birth_year && has_nationality) {
          db->execSqlAsync(
              "INSERT INTO users (username, email, password_hash, birth_year, "
              "nationality) "
              "VALUES ($1, $2, $3, $4, $5) RETURNING id, username, email, "
              "birth_year, nationality, created_at",
              [cb](const drogon::orm::Result& r2) {
                if (r2.size() == 0) {
                  send_error(cb, "Failed to create user",
                             k500InternalServerError);
                  return;
                }
                const auto& row = r2[0];
                Json::Value user;
                user["id"] = Json::Int64(row["id"].as<int64_t>());
                user["username"] = row["username"].as<std::string>();
                user["email"] = row["email"].as<std::string>();
                user["birth_year"] = row["birth_year"].as<int>();
                user["nationality"] = row["nationality"].as<std::string>();
                user["created_at"] = row["created_at"].as<std::string>();
                auto resp = HttpResponse::newHttpJsonResponse(user);
                resp->setStatusCode(k201Created);
                cb(resp);
              },
              [cb](const drogon::orm::DrogonDbException& e) {
                send_error(cb,
                           std::string("Database error: ") + e.base().what(),
                           k500InternalServerError);
              },
              username, email, pw_hash, birth_year, nationality);
        } else if (has_gender && has_nationality) {
          db->execSqlAsync(
              "INSERT INTO users (username, email, password_hash, gender, "
              "nationality) "
              "VALUES ($1, $2, $3, $4, $5) RETURNING id, username, email, "
              "gender, nationality, created_at",
              [cb](const drogon::orm::Result& r2) {
                if (r2.size() == 0) {
                  send_error(cb, "Failed to create user",
                             k500InternalServerError);
                  return;
                }
                const auto& row = r2[0];
                Json::Value user;
                user["id"] = Json::Int64(row["id"].as<int64_t>());
                user["username"] = row["username"].as<std::string>();
                user["email"] = row["email"].as<std::string>();
                user["gender"] = row["gender"].as<std::string>();
                user["nationality"] = row["nationality"].as<std::string>();
                user["created_at"] = row["created_at"].as<std::string>();
                auto resp = HttpResponse::newHttpJsonResponse(user);
                resp->setStatusCode(k201Created);
                cb(resp);
              },
              [cb](const drogon::orm::DrogonDbException& e) {
                send_error(cb,
                           std::string("Database error: ") + e.base().what(),
                           k500InternalServerError);
              },
              username, email, pw_hash, gender, nationality);
        } else if (has_birth_year) {
          db->execSqlAsync(
              "INSERT INTO users (username, email, password_hash, birth_year) "
              "VALUES ($1, $2, $3, $4) RETURNING id, username, email, "
              "birth_year, created_at",
              [cb](const drogon::orm::Result& r2) {
                if (r2.size() == 0) {
                  send_error(cb, "Failed to create user",
                             k500InternalServerError);
                  return;
                }
                const auto& row = r2[0];
                Json::Value user;
                user["id"] = Json::Int64(row["id"].as<int64_t>());
                user["username"] = row["username"].as<std::string>();
                user["email"] = row["email"].as<std::string>();
                user["birth_year"] = row["birth_year"].as<int>();
                user["created_at"] = row["created_at"].as<std::string>();
                auto resp = HttpResponse::newHttpJsonResponse(user);
                resp->setStatusCode(k201Created);
                cb(resp);
              },
              [cb](const drogon::orm::DrogonDbException& e) {
                send_error(cb,
                           std::string("Database error: ") + e.base().what(),
                           k500InternalServerError);
              },
              username, email, pw_hash, birth_year);
        } else if (has_gender) {
          db->execSqlAsync(
              "INSERT INTO users (username, email, password_hash, gender) "
              "VALUES ($1, $2, $3, $4) RETURNING id, username, email, gender, "
              "created_at",
              [cb](const drogon::orm::Result& r2) {
                if (r2.size() == 0) {
                  send_error(cb, "Failed to create user",
                             k500InternalServerError);
                  return;
                }
                const auto& row = r2[0];
                Json::Value user;
                user["id"] = Json::Int64(row["id"].as<int64_t>());
                user["username"] = row["username"].as<std::string>();
                user["email"] = row["email"].as<std::string>();
                user["gender"] = row["gender"].as<std::string>();
                user["created_at"] = row["created_at"].as<std::string>();
                auto resp = HttpResponse::newHttpJsonResponse(user);
                resp->setStatusCode(k201Created);
                cb(resp);
              },
              [cb](const drogon::orm::DrogonDbException& e) {
                send_error(cb,
                           std::string("Database error: ") + e.base().what(),
                           k500InternalServerError);
              },
              username, email, pw_hash, gender);
        } else if (has_nationality) {
          db->execSqlAsync(
              "INSERT INTO users (username, email, password_hash, nationality) "
              "VALUES ($1, $2, $3, $4) RETURNING id, username, email, "
              "nationality, created_at",
              [cb](const drogon::orm::Result& r2) {
                if (r2.size() == 0) {
                  send_error(cb, "Failed to create user",
                             k500InternalServerError);
                  return;
                }
                const auto& row = r2[0];
                Json::Value user;
                user["id"] = Json::Int64(row["id"].as<int64_t>());
                user["username"] = row["username"].as<std::string>();
                user["email"] = row["email"].as<std::string>();
                user["nationality"] = row["nationality"].as<std::string>();
                user["created_at"] = row["created_at"].as<std::string>();
                auto resp = HttpResponse::newHttpJsonResponse(user);
                resp->setStatusCode(k201Created);
                cb(resp);
              },
              [cb](const drogon::orm::DrogonDbException& e) {
                send_error(cb,
                           std::string("Database error: ") + e.base().what(),
                           k500InternalServerError);
              },
              username, email, pw_hash, nationality);
        } else {
          // No optional fields
          db->execSqlAsync(
              "INSERT INTO users (username, email, password_hash) "
              "VALUES ($1, $2, $3) RETURNING id, username, email, created_at",
              [cb](const drogon::orm::Result& r2) {
                if (r2.size() == 0) {
                  send_error(cb, "Failed to create user",
                             k500InternalServerError);
                  return;
                }
                const auto& row = r2[0];
                Json::Value user;
                user["id"] = Json::Int64(row["id"].as<int64_t>());
                user["username"] = row["username"].as<std::string>();
                user["email"] = row["email"].as<std::string>();
                user["created_at"] = row["created_at"].as<std::string>();
                auto resp = HttpResponse::newHttpJsonResponse(user);
                resp->setStatusCode(k201Created);
                cb(resp);
              },
              [cb](const drogon::orm::DrogonDbException& e) {
                send_error(cb,
                           std::string("Database error: ") + e.base().what(),
                           k500InternalServerError);
              },
              username, email, pw_hash);
        }
      },
      [cb](const drogon::orm::DrogonDbException& e) {
        send_error(cb, std::string("Database error: ") + e.base().what(),
                   k500InternalServerError);
      },
      username, email);
}

// ---------------------------------------------------------------------------
// POST /login
// ---------------------------------------------------------------------------
void AuthController::login(const HttpRequestPtr& req,
                           std::function<void(const HttpResponsePtr&)>&& cb) {
  auto json = req->getJsonObject();
  if (!json) {
    send_error(cb, "Invalid JSON body", k400BadRequest);
    return;
  }

  std::string username = (*json)["username"].asString();
  std::string password = (*json)["password"].asString();

  if (username.empty() || password.empty()) {
    send_error(cb, "username and password are required", k400BadRequest);
    return;
  }

  auto db = app().getDbClient();

  db->execSqlAsync(
      "SELECT id, username, email, password_hash FROM users WHERE username = "
      "$1",
      [cb, db, password](const drogon::orm::Result& r) {
        if (r.size() == 0) {
          send_error(cb, "Invalid credentials", k401Unauthorized);
          return;
        }

        const auto& row = r[0];
        std::string stored_hash = row["password_hash"].as<std::string>();

        if (!verify_password(password, stored_hash)) {
          send_error(cb, "Invalid credentials", k401Unauthorized);
          return;
        }

        int64_t user_id = row["id"].as<int64_t>();
        std::string uname = row["username"].as<std::string>();

        // Generate tokens
        auto jwt_svc = vote_backend::utils::make_jwt_service();
        std::string access_token =
            jwt_svc.generate_access_token(user_id, uname);
        std::string refresh_token = jwt_svc.generate_refresh_token(user_id);

        // Store refresh token hash in DB
        std::string refresh_hash = sha256_hex(refresh_token);

        db->execSqlAsync(
            "INSERT INTO refresh_tokens (user_id, token_hash, expires_at) "
            "VALUES ($1, $2, NOW() + INTERVAL '7 days')",
            [cb, access_token, refresh_token](const drogon::orm::Result&) {
              Json::Value resp;
              resp["access_token"] = access_token;
              resp["refresh_token"] = refresh_token;
              resp["token_type"] = "Bearer";
              cb(HttpResponse::newHttpJsonResponse(resp));
            },
            [cb](const drogon::orm::DrogonDbException& e) {
              send_error(cb, std::string("Database error: ") + e.base().what(),
                         k500InternalServerError);
            },
            user_id, refresh_hash);
      },
      [cb](const drogon::orm::DrogonDbException& e) {
        send_error(cb, std::string("Database error: ") + e.base().what(),
                   k500InternalServerError);
      },
      username);
}

// ---------------------------------------------------------------------------
// POST /logout
// ---------------------------------------------------------------------------
void AuthController::logout(const HttpRequestPtr& req,
                            std::function<void(const HttpResponsePtr&)>&& cb) {
  auto json = req->getJsonObject();
  if (!json) {
    send_error(cb, "Invalid JSON body", k400BadRequest);
    return;
  }

  std::string refresh_token = (*json)["refresh_token"].asString();
  if (refresh_token.empty()) {
    send_error(cb, "refresh_token is required", k400BadRequest);
    return;
  }

  auto db = app().getDbClient();
  std::string token_hash = sha256_hex(refresh_token);

  db->execSqlAsync(
      "UPDATE refresh_tokens SET revoked = TRUE "
      "WHERE token_hash = $1 AND revoked = FALSE",
      [cb](const drogon::orm::Result&) {
        // Always return 204, even if token was not found (idempotent).
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k204NoContent);
        cb(resp);
      },
      [cb](const drogon::orm::DrogonDbException& e) {
        send_error(cb, std::string("Database error: ") + e.base().what(),
                   k500InternalServerError);
      },
      token_hash);
}

// ---------------------------------------------------------------------------
// GET /me
// ---------------------------------------------------------------------------
void AuthController::me(const HttpRequestPtr& req,
                        std::function<void(const HttpResponsePtr&)>&& cb) {
  // Retrieve user_id set by JwtAuthFilter
  auto user_id = req->attributes()->get<int64_t>("user_id");
  if (user_id == 0) {
    send_error(cb, "Unauthorized", k401Unauthorized);
    return;
  }

  auto db = app().getDbClient();

  db->execSqlAsync(
      "SELECT id, username, email, birth_year, gender, nationality, "
      "created_at FROM users WHERE id = $1",
      [cb](const drogon::orm::Result& r) {
        if (r.size() == 0) {
          send_error(cb, "User not found", k404NotFound);
          return;
        }

        const auto& row = r[0];
        Json::Value user;
        user["id"] = Json::Int64(row["id"].as<int64_t>());
        user["username"] = row["username"].as<std::string>();
        user["email"] = row["email"].as<std::string>();

        if (!row["birth_year"].isNull()) {
          user["birth_year"] = row["birth_year"].as<int>();
        }
        if (!row["gender"].isNull()) {
          user["gender"] = row["gender"].as<std::string>();
        }
        if (!row["nationality"].isNull()) {
          user["nationality"] = row["nationality"].as<std::string>();
        }
        user["created_at"] = row["created_at"].as<std::string>();

        auto resp = HttpResponse::newHttpJsonResponse(user);
        resp->setStatusCode(k200OK);
        cb(resp);
      },
      [cb](const drogon::orm::DrogonDbException& e) {
        send_error(cb, std::string("Database error: ") + e.base().what(),
                   k500InternalServerError);
      },
      user_id);
}

// ---------------------------------------------------------------------------
// POST /refresh
// ---------------------------------------------------------------------------
void AuthController::refresh(const HttpRequestPtr& req,
                             std::function<void(const HttpResponsePtr&)>&& cb) {
  auto json = req->getJsonObject();
  if (!json) {
    send_error(cb, "Invalid JSON body", k400BadRequest);
    return;
  }

  std::string refresh_token = (*json)["refresh_token"].asString();
  if (refresh_token.empty()) {
    send_error(cb, "refresh_token is required", k400BadRequest);
    return;
  }

  // Verify JWT signature and expiry
  auto jwt_svc = vote_backend::utils::make_jwt_service();
  auto claims = jwt_svc.verify_token(refresh_token);

  if (claims.isNull() || claims.empty()) {
    send_error(cb, "Invalid or expired refresh token", k401Unauthorized);
    return;
  }

  // Check that it's a refresh token
  if (claims["type"].asString() != "refresh") {
    send_error(cb, "Token is not a refresh token", k401Unauthorized);
    return;
  }

  int64_t user_id = std::stoll(claims["sub"].asString());
  std::string token_hash = sha256_hex(refresh_token);

  auto db = app().getDbClient();

  // Look up the refresh token in DB and ensure it's not revoked
  db->execSqlAsync(
      "SELECT id FROM refresh_tokens "
      "WHERE token_hash = $1 AND user_id = $2 AND revoked = FALSE "
      "AND expires_at > NOW()",
      [cb, db, refresh_token, user_id,
       jwt_svc](const drogon::orm::Result& r) mutable {
        if (r.size() == 0) {
          send_error(cb, "Refresh token not found or revoked",
                     k401Unauthorized);
          return;
        }

        int64_t old_token_id = r[0]["id"].as<int64_t>();

        // Revoke old refresh token
        db->execSqlAsync(
            "UPDATE refresh_tokens SET revoked = TRUE WHERE id = $1",
            [cb, db, user_id, jwt_svc](const drogon::orm::Result&) {
              // Fetch username for the new access token
              db->execSqlAsync(
                  "SELECT username FROM users WHERE id = $1",
                  [cb, db, user_id, jwt_svc](const drogon::orm::Result& r2) {
                    if (r2.size() == 0) {
                      send_error(cb, "User not found", k401Unauthorized);
                      return;
                    }

                    std::string username = r2[0]["username"].as<std::string>();

                    // Generate new token pair
                    std::string new_access =
                        jwt_svc.generate_access_token(user_id, username);
                    std::string new_refresh =
                        jwt_svc.generate_refresh_token(user_id);
                    std::string new_hash = sha256_hex(new_refresh);

                    db->execSqlAsync(
                        "INSERT INTO refresh_tokens (user_id, token_hash, "
                        "expires_at) "
                        "VALUES ($1, $2, NOW() + INTERVAL '7 days')",
                        [cb, new_access,
                         new_refresh](const drogon::orm::Result&) {
                          Json::Value resp;
                          resp["access_token"] = new_access;
                          resp["refresh_token"] = new_refresh;
                          resp["token_type"] = "Bearer";
                          cb(HttpResponse::newHttpJsonResponse(resp));
                        },
                        [cb](const drogon::orm::DrogonDbException& e) {
                          send_error(
                              cb,
                              std::string("Database error: ") + e.base().what(),
                              k500InternalServerError);
                        },
                        user_id, new_hash);
                  },
                  [cb](const drogon::orm::DrogonDbException& e) {
                    send_error(
                        cb, std::string("Database error: ") + e.base().what(),
                        k500InternalServerError);
                  },
                  user_id);
            },
            [cb](const drogon::orm::DrogonDbException& e) {
              send_error(cb, std::string("Database error: ") + e.base().what(),
                         k500InternalServerError);
            },
            old_token_id);
      },
      [cb](const drogon::orm::DrogonDbException& e) {
        send_error(cb, std::string("Database error: ") + e.base().what(),
                   k500InternalServerError);
      },
      token_hash, user_id);
}
