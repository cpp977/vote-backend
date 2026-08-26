/**
 *
 *  AuthController.cpp
 *  Implementation of authentication endpoints.
 *
 *  Flow:
 *    /register  – create a new user, hash password with argon2.
 *    /login     – verify credentials, issue access + refresh tokens.
 *    /logout    – revoke the given refresh token.
 *    /refresh   – exchange a valid refresh token for a new token pair.
 *    /forgot    – request a password-reset link (generic response, rate
 *                 limited per email).
 *    /reset     – consume a reset token + new password to change the password.
 */

#include "vote-backend/controllers/AuthController.hpp"

#include <argon2.h>
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpClient.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <json/json.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <trantor/utils/Logger.h>

#include <cstdlib>
#include <fstream>
#include <optional>
#include <string>

#include "vote-backend/models/Users.hpp"
#include "vote-backend/utils/ErrorResponse.hpp"
#include "vote-backend/utils/JwtService.hpp"
#include "vote-backend/utils/Nationality.hpp"
#include "vote-backend/utils/Region.hpp"

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

  std::string out;
  out.reserve(hash_len * 2);
  for (unsigned int i = 0; i < hash_len; ++i)
    out += fmt::format("{:02x}", static_cast<int>(hash[i]));
  return out;
}

// ---------------------------------------------------------------------------
// Password-reset helpers (anonymous namespace)
// ---------------------------------------------------------------------------

/**
 * @brief Configuration loaded from config.json for the password-reset flow.
 *
 * Mirrors the structure used by JwtService::make_jwt_service(): the config is
 * read directly from the JSON file located via the VOTE_BACKEND_CONFPATH
 * environment variable.
 */
struct PasswordResetConfig {
  std::string resend_api_url{};
  std::string resend_api_key{};
  std::string resend_from_email{};
  std::string app_reset_url{};
  int token_expiry_hours = 1;
  int max_attempts = 5;
  int rate_limit_window_minutes = 15;
  int rate_limit_max_requests = 3;

  [[nodiscard]] std::string to_string() const {
    return fmt::format(
        "resend_api_url: {}, resend_api_key: {}, resend_from_email: "
        "{}, app_reset_url: {}",
        resend_api_url, resend_api_key, resend_from_email, app_reset_url);
  }
};

/**
 * @brief Load password-reset configuration from the project's config.json.
 *
 * Falls back to sensible defaults if the file cannot be read.
 */
PasswordResetConfig load_password_reset_config() {
  PasswordResetConfig cfg;

  // Resolve config path via VOTE_BACKEND_CONFPATH (same convention as
  // JwtService::make_jwt_service()).
  const char* conf_path_c = std::getenv("VOTE_BACKEND_CONFPATH");
  std::string config_path =
      conf_path_c ? std::string(conf_path_c) : "/etc/vote";

  Json::Value json_cfg;
  std::ifstream ifs(fmt::format("{}/config.json", config_path));
  if (ifs.is_open()) {
    ifs >> json_cfg;
    ifs.close();
  } else {
    // Fallback defaults
    json_cfg["resend_api_url"] = "https://api.resend.com";
    json_cfg["resend_api_key"] = "";
    json_cfg["resend_from_email"] = "onboarding@resend.dev";
    json_cfg["app_reset_url"] = "https://vote-backend.local/reset-password";
    json_cfg["password_reset_token_expiry_hours"] = 1;
    json_cfg["password_reset_max_attempts"] = 5;
    json_cfg["password_reset_rate_limit_window_minutes"] = 15;
    json_cfg["password_reset_rate_limit_max_requests"] = 3;
  }

  LOG_DEBUG << fmt::format("Raw config: {}", json_cfg.toStyledString());
  cfg.resend_api_url = json_cfg["resend_api_url"].asString();
  cfg.resend_api_key = json_cfg["resend_api_key"].asString();
  cfg.resend_from_email = json_cfg["resend_from_email"].asString();
  cfg.app_reset_url = json_cfg["app_reset_url"].asString();
  cfg.token_expiry_hours =
      json_cfg["password_reset_token_expiry_hours"].asInt();
  cfg.max_attempts = json_cfg["password_reset_max_attempts"].asInt();
  cfg.rate_limit_window_minutes =
      json_cfg["password_reset_rate_limit_window_minutes"].asInt();
  cfg.rate_limit_max_requests =
      json_cfg["password_reset_rate_limit_max_requests"].asInt();
  return cfg;
}

/**
 * @brief Generate a 32-byte cryptographically secure random token,
 *        hex-encoded (same approach as salt generation in hash_password).
 */
std::string generate_reset_token() {
  constexpr size_t token_len = 32;
  unsigned char buf[token_len];
  if (RAND_bytes(buf, static_cast<int>(token_len)) != 1)
    throw std::runtime_error("RAND_bytes failed for reset token generation");

  std::string out;
  out.reserve(token_len * 2);
  for (size_t i = 0; i < token_len; ++i)
    out += fmt::format("{:02x}", static_cast<int>(buf[i]));
  return out;
}

/**
 * @brief Fire-and-forget HTTP POST to the Resend API to send a password-reset
 * email. The token is included as a query parameter on the app's reset URL.
 *
 * Failures are logged but never propagated to the HTTP response — the caller
 * has already returned (or will return) the same generic response regardless.
 */
void send_password_reset_email(const std::string& email,
                               const std::string& raw_token,
                               const PasswordResetConfig& cfg) {
  std::string reset_link = cfg.app_reset_url + "?token=" + raw_token;

  Json::Value email_body;
  email_body["from"] = cfg.resend_from_email;
  email_body["to"] = email;
  email_body["subject"] = "Password Reset";
  email_body["html"] = "<p>Click <a href=\"" + reset_link +
                       "\">here</a> to reset your "
                       "password. This link expires in " +
                       std::to_string(cfg.token_expiry_hours) + " hour(s).</p>";

  auto req = HttpRequest::newHttpJsonRequest(email_body);
  req->addHeader("Authorization", "Bearer " + cfg.resend_api_key);
  req->setPath("/emails");
  req->setMethod(drogon::HttpMethod::Post);

  LOG_DEBUG << fmt::format("E-Mail send request: {}, path: {}", req->body(),
                           req->getPath());
  auto client = drogon::HttpClient::newHttpClient(cfg.resend_api_url);
  client->sendRequest(
      req, [client](drogon::ReqResult result, const HttpResponsePtr& resp) {
        if (result != drogon::ReqResult::Ok) {
          LOG_WARN << "Failed to send password reset email: " << result;
        } else {
          int status = resp ? resp->statusCode() : 0;
          if (status < 200 || status >= 300) {
            LOG_WARN << "Resend API returned non-success status: " << status;
          }
          LOG_DEBUG << fmt::format("Response (result={}) is: {}",
                                   static_cast<int>(result), resp->body());
        }
      });
}

/**
 * @brief Build the generic "if an account exists …" response.
 *
 * This response is intentionally identical whether or not an account exists
 * for the given email, whether or not the email address is valid, and whether
 * or not the request was rate-limited — preventing user enumeration.
 */
HttpResponsePtr send_generic_reset_response() {
  Json::Value resp;
  resp["message"] =
      "If an account with that email exists, a password reset link has been "
      "sent.";
  auto http_resp = HttpResponse::newHttpJsonResponse(resp);
  http_resp->setStatusCode(k200OK);
  return http_resp;
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
  bool has_region = (*json).isMember("region") && !(*json)["region"].isNull();

  int birth_year = has_birth_year ? (*json)["birth_year"].asInt() : 0;
  std::string gender = has_gender ? (*json)["gender"].asString() : "";
  std::string nationality =
      has_nationality ? (*json)["nationality"].asString() : "";
  std::string region = has_region ? (*json)["region"].asString() : "";

  // Validate gender if provided
  if (has_gender && gender != "m" && gender != "w" && gender != "d") {
    send_error(cb, "gender must be one of 'm', 'w', 'd'", k400BadRequest);
    return;
  }

  // Nationality, when provided, must be an ISO 3166-1 alpha-2 country code;
  // it is normalized to uppercase before storage so that statistics filters
  // match deterministically instead of fragmenting over spelling variants.
  if (has_nationality) {
    nationality = vote_backend::utils::normalize_nationality(nationality);
    if (nationality.empty()) {
      send_error(cb,
                 "nationality must be an ISO 3166-1 alpha-2 country code "
                 "(e.g. 'DE')",
                 k400BadRequest);
      return;
    }
  }

  // Region, when provided, must be an ISO 3166-2 subdivision code; it is
  // normalized to uppercase before storage (see nationality above).
  if (has_region) {
    region = vote_backend::utils::normalize_region(region);
    if (region.empty()) {
      send_error(cb,
                 "region must be an ISO 3166-2 subdivision code "
                 "(e.g. 'DE-BE')",
                 k400BadRequest);
      return;
    }
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

  // Preflight in a single round-trip: check whether the username/email is
  // taken and, when supplied, whether the normalized nationality / region
  // codes exist in the reference tables. Verifying them here turns what
  // would otherwise surface as foreign-key violations on INSERT (generic
  // 500) into precise 400s. Empty strings are the "field absent" sentinels:
  // both columns are only ever stored as normalized uppercase codes, never
  // as ''.
  db->execSqlAsync(
      "SELECT EXISTS(SELECT 1 FROM users WHERE username = $1 OR email = $2) "
      "AS user_exists, "
      "($3 = '' OR EXISTS(SELECT 1 FROM countries WHERE code = $3)) "
      "AS country_ok, "
      "($4 = '' OR EXISTS(SELECT 1 FROM regions WHERE code = $4)) "
      "AS region_ok",
      [cb, db, username, email, password, has_birth_year, birth_year,
       has_gender, gender, has_nationality, has_region, nationality,
       region](const drogon::orm::Result& r) {
        if (!r[0]["country_ok"].as<bool>()) {
          send_error(
              cb, "nationality must be a known ISO 3166-1 alpha-2 country code",
              k400BadRequest);
          return;
        }

        if (!r[0]["region_ok"].as<bool>()) {
          send_error(cb, "region must be a known ISO 3166-2 subdivision code",
                     k400BadRequest);
          return;
        }

        if (r[0]["user_exists"].as<bool>()) {
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

        // Single dynamically-built INSERT containing exactly the provided
        // optional columns; omitted ones default to NULL in the schema.
        // RETURNING covers every column so one response builder suffices.
        std::string sql = "INSERT INTO users (username, email, password_hash";
        std::string placeholders = "$1, $2, $3";
        int next_param = 3;
        const auto add_optional_column = [&](const char* column) {
          sql += std::string(", ") + column;
          placeholders += ", $" + std::to_string(++next_param);
        };
        if (has_birth_year) {
          add_optional_column("birth_year");
        }
        if (has_gender) {
          add_optional_column("gender");
        }
        if (has_nationality) {
          add_optional_column("nationality");
        }
        if (has_region) {
          add_optional_column("region");
        }
        sql += ") VALUES (" + placeholders +
               ") RETURNING id, username, email, birth_year, gender, "
               "nationality, region, created_at";

        auto binder = *db << std::move(sql);
        binder << username << email << pw_hash;
        if (has_birth_year) {
          binder << birth_year;
        }
        if (has_gender) {
          binder << gender;
        }
        if (has_nationality) {
          binder << nationality;
        }
        if (has_region) {
          binder << region;
        }
        binder >> [cb](const drogon::orm::Result& r2) {
          if (r2.size() == 0) {
            send_error(cb, "Failed to create user", k500InternalServerError);
            return;
          }
          const auto& row = r2[0];
          Json::Value user;
          user["id"] = row["id"].as<std::string>();
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
          if (!row["region"].isNull()) {
            user["region"] = row["region"].as<std::string>();
          }
          user["created_at"] = row["created_at"].as<std::string>();
          auto resp = HttpResponse::newHttpJsonResponse(user);
          resp->setStatusCode(k201Created);
          cb(resp);
        } >> [cb](const drogon::orm::DrogonDbException& e) {
          send_error(cb, std::string("Database error: ") + e.base().what(),
                     k500InternalServerError);
        };
      },
      [cb](const drogon::orm::DrogonDbException& e) {
        send_error(cb, std::string("Database error: ") + e.base().what(),
                   k500InternalServerError);
      },
      username, email, nationality, region);
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
      "SELECT id, username, email, password_hash, is_admin, is_active FROM "
      "users WHERE "
      "username = $1",
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

        std::string user_id = row["id"].as<std::string>();
        std::string uname = row["username"].as<std::string>();
        bool is_admin = row["is_admin"].as<bool>();
        bool is_active = row["is_active"].as<bool>();

        // Generate tokens
        auto jwt_svc = vote_backend::utils::make_jwt_service();
        std::string access_token =
            jwt_svc.generate_access_token(user_id, uname, is_admin, is_active);
        std::string refresh_token = jwt_svc.generate_refresh_token(user_id);

        // Store refresh token hash in DB
        std::string refresh_hash = sha256_hex(refresh_token);

        db->execSqlAsync(
            "INSERT INTO refresh_tokens (user_id, token_hash, expires_at) "
            "VALUES ($1::uuid, $2, NOW() + INTERVAL '7 days')",
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
  auto user_id = req->attributes()->get<std::string>("user_id");
  if (user_id.length() == 0) {
    send_error(cb, "Unauthorized", k401Unauthorized);
    return;
  }

  auto db = app().getDbClient();

  db->execSqlAsync(
      "SELECT id, username, email, birth_year, gender, nationality, region, "
      "created_at, is_admin FROM users WHERE id = $1",
      [cb](const drogon::orm::Result& r) {
        if (r.size() == 0) {
          send_error(cb, "User not found", k404NotFound);
          return;
        }

        const auto& row = r[0];
        Json::Value user;
        user["id"] = row["id"].as<std::string>();
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
        if (!row["region"].isNull()) {
          user["region"] = row["region"].as<std::string>();
        }
        user["is_admin"] = row["is_admin"].as<bool>();
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
// PATCH /me  (and PUT /me) – update the authenticated user's own profile
//
// Only `email`, `gender`, `password`, `nationality` and `region` may be
// modified. The `username` is the user's identity (derived from the JWT) and
// is therefore never modifiable; any attempt to change it (or any other
// field) is rejected with 400. `password` is re-hashed with Argon2id before
// being stored. The nullable reference columns `nationality` and `region`
// additionally accept an explicit JSON null to clear them.
// ---------------------------------------------------------------------------
void AuthController::update_me(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& cb) {
  // Identify the user from the JWT (set by JwtAuthFilter).
  auto user_id = req->attributes()->get<std::string>("user_id");
  if (user_id.length() == 0) {
    send_error(cb, "Unauthorized", k401Unauthorized);
    return;
  }

  auto json = req->getJsonObject();
  if (!json) {
    send_error(cb, "Invalid JSON body", k400BadRequest);
    return;
  }
  if (!json->isObject()) {
    send_error(cb, "Request body must be a JSON object", k400BadRequest);
    return;
  }

  // Collect the (optional) modifiable fields from the body.
  bool has_email = false, has_gender = false, has_password = false;
  std::string email;
  std::string gender;
  std::string password;
  // nationality / region have three states: absent (no change), engaged empty
  // string (clear the column) or engaged value (new normalized code). The
  // empty-string-as-clear sentinel is safe because normalized codes are never
  // empty.
  std::optional<std::string> nationality_update;
  std::optional<std::string> region_update;

  for (auto it = json->begin(); it != json->end(); ++it) {
    const std::string key = it.name();
    if (key == "email") {
      if (it->isNull()) {
        send_error(cb, "email cannot be null", k400BadRequest);
        return;
      }
      if (!it->isString()) {
        send_error(cb, "email must be a string", k400BadRequest);
        return;
      }
      email = it->asString();
      if (email.empty()) {
        send_error(cb, "email cannot be empty", k400BadRequest);
        return;
      }
      has_email = true;
    } else if (key == "gender") {
      if (it->isNull()) {
        send_error(cb, "gender cannot be null", k400BadRequest);
        return;
      }
      if (!it->isString()) {
        send_error(cb, "gender must be a string", k400BadRequest);
        return;
      }
      gender = it->asString();
      if (gender != "m" && gender != "w" && gender != "d") {
        send_error(cb, "gender must be one of 'm', 'w', 'd'", k400BadRequest);
        return;
      }
      has_gender = true;
    } else if (key == "password") {
      if (it->isNull()) {
        send_error(cb, "password cannot be null", k400BadRequest);
        return;
      }
      if (!it->isString()) {
        send_error(cb, "password must be a string", k400BadRequest);
        return;
      }
      password = it->asString();
      if (password.empty()) {
        send_error(cb, "password cannot be empty", k400BadRequest);
        return;
      }
      if (password.size() < 8) {
        send_error(cb, "password must be at least 8 characters",
                   k400BadRequest);
        return;
      }
      has_password = true;
    } else if (key == "nationality") {
      if (it->isNull()) {
        nationality_update = std::string("");
      } else if (it->isString()) {
        std::string value =
            vote_backend::utils::normalize_nationality(it->asString());
        if (value.empty()) {
          send_error(cb,
                     "nationality must be an ISO 3166-1 alpha-2 country code "
                     "(e.g. 'DE')",
                     k400BadRequest);
          return;
        }
        nationality_update = value;
      } else {
        send_error(cb, "nationality must be a string or null", k400BadRequest);
        return;
      }
    } else if (key == "region") {
      if (it->isNull()) {
        region_update = std::string("");
      } else if (it->isString()) {
        std::string value =
            vote_backend::utils::normalize_region(it->asString());
        if (value.empty()) {
          send_error(cb,
                     "region must be an ISO 3166-2 subdivision code "
                     "(e.g. 'DE-BE')",
                     k400BadRequest);
          return;
        }
        region_update = value;
      } else {
        send_error(cb, "region must be a string or null", k400BadRequest);
        return;
      }
    } else {
      // username and every other field are not modifiable via this endpoint.
      send_error(cb, "field '" + key + "' is not modifiable", k400BadRequest);
      return;
    }
  }

  if (!has_email && !has_gender && !has_password && !nationality_update &&
      !region_update) {
    send_error(cb, "No modifiable fields provided", k400BadRequest);
    return;
  }

  // Hash the new password (if any) before touching the database.
  std::string pw_hash;
  if (has_password) {
    try {
      pw_hash = hash_password(password);
    } catch (const std::exception& e) {
      send_error(cb, std::string("Internal error: ") + e.what(),
                 k500InternalServerError);
      return;
    }
  }

  auto db = app().getDbClient();

  // When a new nationality / region value was supplied (not a clearing
  // request), verify it against the reference tables first so unknown codes
  // fail with a precise 400 instead of as foreign-key violations on UPDATE
  // (generic 500). Empty strings are the "nothing to check" sentinels.
  db->execSqlAsync(
      "SELECT ($1 = '' OR EXISTS(SELECT 1 FROM countries WHERE code = $1)) "
      "AS country_ok, "
      "($2 = '' OR EXISTS(SELECT 1 FROM regions WHERE code = $2)) "
      "AS region_ok",
      [cb, db, user_id, has_email, email, has_gender, gender, has_password,
       pw_hash, nationality_update,
       region_update](const drogon::orm::Result& r) {
        if (!r[0]["country_ok"].as<bool>()) {
          send_error(
              cb, "nationality must be a known ISO 3166-1 alpha-2 country code",
              k400BadRequest);
          return;
        }
        if (!r[0]["region_ok"].as<bool>()) {
          send_error(cb, "region must be a known ISO 3166-2 subdivision code",
                     k400BadRequest);
          return;
        }

        // Build the SET clause dynamically from the provided fields only;
        // engaged-but-empty nationality / region values clear their column.
        std::string sql = "UPDATE users SET ";
        int param_count = 0;
        const auto placeholder = [&param_count]() {
          return "$" + std::to_string(++param_count);
        };
        if (has_email) {
          sql += "email = " + placeholder() + ", ";
        }
        if (has_gender) {
          sql += "gender = " + placeholder() + ", ";
        }
        if (has_password) {
          sql += "password_hash = " + placeholder() + ", ";
        }
        if (nationality_update) {
          sql += nationality_update->empty()
                     ? std::string("nationality = NULL, ")
                     : "nationality = " + placeholder() + ", ";
        }
        if (region_update) {
          sql += region_update->empty() ? std::string("region = NULL, ")
                                        : "region = " + placeholder() + ", ";
        }
        sql += "updated_at = NOW() WHERE id = " + placeholder() +
               " RETURNING id, username, email, birth_year, gender, "
               "nationality, region, created_at, updated_at, is_admin";

        auto binder = *db << std::move(sql);
        if (has_email) {
          binder << email;
        }
        if (has_gender) {
          binder << gender;
        }
        if (has_password) {
          binder << pw_hash;
        }
        if (nationality_update && !nationality_update->empty()) {
          binder << *nationality_update;
        }
        if (region_update && !region_update->empty()) {
          binder << *region_update;
        }
        binder << user_id;

        binder >> [cb](const drogon::orm::Result& r2) {
          if (r2.size() == 0) {
            send_error(cb, "User not found", k404NotFound);
            return;
          }

          const auto& row = r2[0];
          Json::Value user;
          user["id"] = row["id"].as<std::string>();
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
          if (!row["region"].isNull()) {
            user["region"] = row["region"].as<std::string>();
          }
          user["is_admin"] = row["is_admin"].as<bool>();
          user["created_at"] = row["created_at"].as<std::string>();
          user["updated_at"] = row["updated_at"].as<std::string>();

          auto resp = HttpResponse::newHttpJsonResponse(user);
          resp->setStatusCode(k200OK);
          cb(resp);
        } >> [cb](const drogon::orm::DrogonDbException& e) {
          std::string msg = e.base().what();
          // The only unique constraint that can be violated here is
          // the one on email (username is never updated).
          if (msg.find("duplicate") != std::string::npos ||
              msg.find("unique") != std::string::npos) {
            send_error(cb, "email already in use", k409Conflict);
            return;
          }
          send_error(cb, std::string("Database error: ") + msg,
                     k500InternalServerError);
        };
      },
      [cb](const drogon::orm::DrogonDbException& e) {
        send_error(cb, std::string("Database error: ") + e.base().what(),
                   k500InternalServerError);
      },
      nationality_update ? *nationality_update : std::string(""),
      region_update ? *region_update : std::string(""));
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

  std::string user_id = claims["sub"].asString();
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
                  "SELECT username, is_admin FROM users WHERE id = $1",
                  [cb, db, user_id, jwt_svc](const drogon::orm::Result& r2) {
                    if (r2.size() == 0) {
                      send_error(cb, "User not found", k401Unauthorized);
                      return;
                    }

                    std::string username = r2[0]["username"].as<std::string>();
                    bool is_admin = r2[0]["is_admin"].as<bool>();

                    // Generate new token pair
                    std::string new_access = jwt_svc.generate_access_token(
                        user_id, username, is_admin,
                        true);  // Always true for refresh token generation
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

// ---------------------------------------------------------------------------
// POST /user/password/forgot
// ---------------------------------------------------------------------------
void AuthController::forgot_password(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& cb) {
  auto json = req->getJsonObject();
  if (!json) {
    send_error(cb, "Invalid JSON body", k400BadRequest);
    return;
  }

  std::string email = (*json)["email"].asString();
  if (email.empty()) {
    send_error(cb, "email is required", k400BadRequest);
    return;
  }

  auto cfg = load_password_reset_config();
  LOG_DEBUG << fmt::format("Password reset config:\n {}", cfg.to_string());
  auto db = app().getDbClient();

  // Single query checks both email existence AND rate limiting:
  // the correlated subquery counts password_resets rows created for this user
  // within the rate-limit window; if that count is below the max, the row is
  // returned and we proceed. If the email doesn't exist or the rate limit is
  // exceeded, no rows are returned.
  std::string sql = fmt::format(
      "SELECT id FROM users WHERE email = $1 AND "
      "(SELECT COUNT(*) FROM password_resets pr "
      "WHERE pr.user_id = users.id "
      "AND pr.created_at > NOW() - INTERVAL '{} minutes') < {} LIMIT 1",
      cfg.rate_limit_window_minutes, cfg.rate_limit_max_requests);

  db->execSqlAsync(
      sql,
      [cb, db, email, cfg](const drogon::orm::Result& r) {
        if (r.size() == 0) {
          // Email not found OR rate-limited — same generic response.
          cb(send_generic_reset_response());
          return;
        }

        std::string user_id = r[0]["id"].as<std::string>();

        // Generate a cryptographically secure token and hash it for storage.
        std::string raw_token;
        std::string token_hash;
        try {
          raw_token = generate_reset_token();
          token_hash = sha256_hex(raw_token);
        } catch (const std::exception& e) {
          LOG_ERROR << "Token generation failed: " << e.what();
          cb(send_generic_reset_response());
          return;
        }

        // Persist the hashed token with an expiry timestamp.
        std::string insert_sql = fmt::format(
            "INSERT INTO password_resets (user_id, token_hash, expires_at) "
            "VALUES ($1::uuid, $2::text, NOW() + INTERVAL '{} hours')",
            cfg.token_expiry_hours);

        db->execSqlAsync(
            insert_sql,
            [cb, email, raw_token, cfg](const drogon::orm::Result&) {
              // Fire-and-forget: send the reset email asynchronously.
              send_password_reset_email(email, raw_token, cfg);
              cb(send_generic_reset_response());
            },
            [cb](const drogon::orm::DrogonDbException& e) {
              LOG_ERROR << "Failed to store password reset token: "
                        << e.base().what();
              cb(send_generic_reset_response());
            },
            user_id, token_hash);
      },
      [cb](const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Password reset query failed: " << e.base().what();
        cb(send_generic_reset_response());
      },
      email);
}

// ---------------------------------------------------------------------------
// POST /user/password/reset
// ---------------------------------------------------------------------------
void AuthController::reset_password(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& cb) {
  auto json = req->getJsonObject();
  if (!json) {
    send_error(cb, "Invalid JSON body", k400BadRequest);
    return;
  }

  std::string token = (*json)["token"].asString();
  std::string password = (*json)["password"].asString();

  if (token.empty()) {
    send_error(cb, "token is required", k400BadRequest);
    return;
  }

  if (password.size() < 8) {
    send_error(cb, "password must be at least 8 characters", k400BadRequest);
    return;
  }

  auto cfg = load_password_reset_config();
  auto db = app().getDbClient();

  // Hash the incoming token with SHA-256 before looking it up.
  std::string token_hash = sha256_hex(token);

  // Claim the token and run the reset inside one transaction. The claim
  // itself is the atomic UPDATE below: it only affects — and thus returns —
  // a row if the token is currently unused, unexpired, and under the
  // attempt budget. Concurrent requests racing on the same token can no
  // longer both pass a check-then-act window; at most one of them will see
  // a row back from this statement.
  db->newTransactionAsync([cb, db, cfg, password, token_hash](
                              const std::shared_ptr<drogon::orm::Transaction>&
                                  txn) {
    if (!txn) {
      send_error(cb, "Failed to create transaction", k500InternalServerError);
      return;
    }

    txn->execSqlAsync(
        "UPDATE password_resets SET used = TRUE "
        "WHERE token_hash = $1::text AND used = FALSE "
        "AND expires_at > NOW() AND attempt_count < $2::int "
        "RETURNING id, user_id",
        [txn, cb, db, cfg, password, token_hash](const drogon::orm::Result& r) {
          if (r.size() == 0) {
            // Nothing claimed: token missing, expired, already used,
            // or attempts exhausted. Roll back (no-op, nothing was
            // written) and record the attempt for bookkeeping outside
            // the dead transaction.
            txn->rollback();
            db->execSqlAsync(
                "UPDATE password_resets "
                "SET attempt_count = attempt_count + 1 "
                "WHERE token_hash = $1::text",
                [](const drogon::orm::Result&) {},
                [](const drogon::orm::DrogonDbException& e) {
                  LOG_ERROR << "Failed to record reset attempt: "
                            << e.base().what();
                },
                token_hash);
            send_error(cb, "Invalid or expired token", k400BadRequest);
            return;
          }

          std::string user_id = r[0]["user_id"].as<std::string>();

          // Token successfully claimed — hash the new password.
          std::string pw_hash;
          try {
            pw_hash = hash_password(password);
          } catch (const std::exception& e) {
            LOG_ERROR << "Password hashing failed: " << e.what();
            txn->rollback();
            send_error(cb, "Internal server error", k500InternalServerError);
            return;
          }

          txn->setCommitCallback([cb](bool committed) {
            if (committed) {
              Json::Value resp;
              resp["message"] = "Password reset successfully";
              cb(HttpResponse::newHttpJsonResponse(resp));
            } else {
              send_error(cb, "Password reset failed", k500InternalServerError);
            }
          });

          // 1. Update the user's password hash.
          txn->execSqlAsync(
              "UPDATE users SET password_hash = $1::text, "
              "password_changed_at = NOW() WHERE id = $2::uuid",
              [txn, cb, user_id](const drogon::orm::Result&) {
                // 2. Invalidate all refresh tokens for this user.
                txn->execSqlAsync(
                    "UPDATE refresh_tokens SET revoked = TRUE "
                    "WHERE user_id = $1::uuid",
                    [txn](const drogon::orm::Result&) {
                      // Both writes succeeded; the transaction
                      // auto-commits when this lambda returns and
                      // the SqlCmd holding the txn shared_ptr is
                      // released.
                    },
                    [txn, cb](const drogon::orm::DrogonDbException& e) {
                      LOG_ERROR << "Failed to invalidate refresh "
                                   "tokens: "
                                << e.base().what();
                      txn->rollback();
                      send_error(cb, "Internal server error",
                                 k500InternalServerError);
                    },
                    user_id);
              },
              [txn, cb](const drogon::orm::DrogonDbException& e) {
                LOG_ERROR << "Failed to update password: " << e.base().what();
                txn->rollback();
                send_error(cb, "Internal server error",
                           k500InternalServerError);
              },
              pw_hash, user_id);
        },
        [txn, cb](const drogon::orm::DrogonDbException& e) {
          LOG_ERROR << "Failed to claim reset token: " << e.base().what();
          txn->rollback();
          send_error(cb, "Internal server error", k500InternalServerError);
        },
        token_hash, cfg.max_attempts);
  });
}
