/**
 *
 *  UserController.cpp
 *  Implementation of the user management controller.
 *
 *  Admin endpoints:
 *    GET  /admin/users          – list all users (id, username)
 *    GET  /admin/users/{1}      – get a specific user by ID
 *    POST /admin/users/{1}/inactive – set a user inactive
 *    POST /admin/users/{1}/active   – set a user active
 *
 *  User endpoint:
 *    POST /users/{1}/delete    – delete the authenticated user's own
 *                                account (JwtAuthFilter-protected).
 */

#include "vote-backend/controllers/UserController.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>
#include <drogon/orm/Exception.h>
#include <fmt/format.h>
#include <json/json.h>
#include <trantor/utils/Logger.h>

#include <string>
#include <vector>

#include "vote-backend/utils/ErrorResponse.hpp"

using namespace drogon;
using drogon::orm::DrogonDbException;
using drogon::orm::Result;

void UserController::list_users(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& cb) {
  auto dbClient = app().getDbClient();
  auto callbackPtr =
      std::make_shared<std::function<void(const HttpResponsePtr&)>>(
          std::move(cb));

  // Query to retrieve both user IDs and usernames
  const std::string sql = "SELECT id, username FROM users ORDER BY username";

  dbClient->execSqlAsync(
      sql,
      [callbackPtr](const Result& r) {
        try {
          Json::Value arr(Json::arrayValue);
          for (const auto& row : r) {
            // Return both ID and username fields
            Json::Value userObj;
            userObj["id"] = row.at("id").as<int64_t>();
            userObj["username"] = row.at("username").as<std::string>();
            arr.append(userObj);
          }
          (*callbackPtr)(HttpResponse::newHttpJsonResponse(arr));
        } catch (const std::exception& e) {
          LOG_ERROR << fmt::format("UserController::list_users failed: {}",
                                   e.what());
          auto resp = HttpResponse::newHttpResponse();
          resp->setStatusCode(k500InternalServerError);
          resp->setBody(std::string("Internal error: ") + e.what());
          (*callbackPtr)(resp);
        }
      },
      [callbackPtr](const DrogonDbException& e) {
        LOG_ERROR << fmt::format("UserController::list_users DB error: {}",
                                 e.base().what());
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody(e.base().what());
        (*callbackPtr)(resp);
      });
}

void UserController::get_user_by_id(
    const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb,
    int64_t user_id) {
  auto dbClient = app().getDbClient();

  // Query to retrieve all user columns except password_hash for the given ID
  const std::string sql =
      "SELECT id, username, email, birth_year, gender, "
      "nationality, created_at, updated_at, is_admin, is_active "
      "FROM users WHERE id = $1 ORDER BY username";

  dbClient->execSqlAsync(
      sql,
      [cb](const Result& r) {
        try {
          if (r.size() == 0) {
            send_error(cb, "User not found", k404NotFound);
            return;
          }

          const auto& row = r[0];
          Json::Value userObj;

          // Map all columns except password_hash
          userObj["id"] = row.at("id").as<int64_t>();
          userObj["username"] = row.at("username").as<std::string>();
          userObj["email"] = row.at("email").as<std::string>();
          userObj["birth_year"] = row.at("birth_year").as<int>();
          userObj["gender"] = row.at("gender").as<std::string>();
          userObj["nationality"] = row.at("nationality").as<std::string>();
          userObj["created_at"] = row.at("created_at").as<std::string>();
          userObj["updated_at"] = row.at("updated_at").as<std::string>();
          userObj["is_admin"] = row.at("is_admin").as<bool>();
          userObj["is_active"] = row.at("is_active").as<bool>();

          (cb)(HttpResponse::newHttpJsonResponse(userObj));
        } catch (const std::exception& e) {
          LOG_ERROR << fmt::format("UserController::get_user_by_id failed: {}",
                                   e.what());
          auto resp = HttpResponse::newHttpResponse();
          resp->setStatusCode(k500InternalServerError);
          resp->setBody(std::string("Internal error: ") + e.what());
          (cb)(resp);
        }
      },
      [cb](const DrogonDbException& e) {
        LOG_ERROR << fmt::format("UserController::get_user_by_id DB error: {}",
                                 e.base().what());
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody(e.base().what());
        (cb)(resp);
      },
      user_id);
}

void UserController::set_user_inactive(
    const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb,
    int64_t user_id) {
  auto dbClient = app().getDbClient();

  const std::string sql =
      "UPDATE users SET is_active = false, updated_at = NOW() WHERE id = $1 "
      "RETURNING id, is_active";

  dbClient->execSqlAsync(
      sql,
      [cb](const Result& r) {
        try {
          if (r.size() == 0) {
            send_error(cb, "User not found", k404NotFound);
            return;
          }

          const auto& row = r[0];
          Json::Value respObj;
          respObj["id"] = row.at("id").as<int64_t>();
          respObj["is_active"] = row.at("is_active").as<bool>();
          respObj["message"] = "User set to inactive";

          (cb)(HttpResponse::newHttpJsonResponse(respObj));
        } catch (const std::exception& e) {
          LOG_ERROR << fmt::format(
              "UserController::set_user_inactive failed: {}", e.what());
          auto resp = HttpResponse::newHttpResponse();
          resp->setStatusCode(k500InternalServerError);
          resp->setBody(std::string("Internal error: ") + e.what());
          (cb)(resp);
        }
      },
      [cb](const DrogonDbException& e) {
        LOG_ERROR << fmt::format(
            "UserController::set_user_inactive DB error: {}", e.base().what());
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody(e.base().what());
        (cb)(resp);
      },
      user_id);
}

void UserController::set_user_active(
    const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb,
    int64_t user_id) {
  auto dbClient = app().getDbClient();

  const std::string sql =
      "UPDATE users SET is_active = true, updated_at = NOW() WHERE id = $1 "
      "RETURNING id, is_active";

  dbClient->execSqlAsync(
      sql,
      [cb](const Result& r) {
        try {
          if (r.size() == 0) {
            send_error(cb, "User not found", k404NotFound);
            return;
          }

          const auto& row = r[0];
          Json::Value respObj;
          respObj["id"] = row.at("id").as<int64_t>();
          respObj["is_active"] = row.at("is_active").as<bool>();
          respObj["message"] = "User set to active";

          (cb)(HttpResponse::newHttpJsonResponse(respObj));
        } catch (const std::exception& e) {
          LOG_ERROR << fmt::format("UserController::set_user_active failed: {}",
                                   e.what());
          auto resp = HttpResponse::newHttpResponse();
          resp->setStatusCode(k500InternalServerError);
          resp->setBody(std::string("Internal error: ") + e.what());
          (cb)(resp);
        }
      },
      [cb](const DrogonDbException& e) {
        LOG_ERROR << fmt::format("UserController::set_user_active DB error: {}",
                                 e.base().what());
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody(e.base().what());
        (cb)(resp);
      },
      user_id);
}

// ---------------------------------------------------------------------------
// POST /users/{1}/delete
//
// Deletes the authenticated user's own account.  The JwtAuthFilter
// stores the caller's user_id in the request attributes; we verify that
// it matches the path parameter so that a user can only delete themselves.
//
// The database cascade rules handle the dependent rows automatically:
//   - refresh_tokens  ON DELETE CASCADE  → all tokens are removed
//   - questions       ON DELETE SET NULL → submitted_by/reviewed_by become NULL
//   - question_user   has no FK to users → rows are kept (hash is opaque)
// ---------------------------------------------------------------------------
void UserController::delete_user(
    const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb,
    int64_t user_id) {
  // Retrieve the caller's identity from the JWT (set by JwtAuthFilter).
  auto calling_user_id = req->attributes()->get<int64_t>("user_id");
  if (calling_user_id == 0) {
    send_error(cb, "Unauthorized", k401Unauthorized);
    return;
  }

  // Only the user themselves can delete their own account.
  if (calling_user_id != user_id) {
    send_error(cb, "Forbidden: can only delete your own account",
               k403Forbidden);
    return;
  }

  auto dbClient = app().getDbClient();

  const std::string sql = "DELETE FROM users WHERE id = $1 RETURNING id";

  dbClient->execSqlAsync(
      sql,
      [cb](const Result& r) {
        try {
          if (r.size() == 0) {
            send_error(cb, "User not found", k404NotFound);
            return;
          }

          auto resp = HttpResponse::newHttpResponse();
          resp->setStatusCode(k204NoContent);
          cb(resp);
        } catch (const std::exception& e) {
          LOG_ERROR << fmt::format("UserController::delete_user failed: {}",
                                   e.what());
          auto resp = HttpResponse::newHttpResponse();
          resp->setStatusCode(k500InternalServerError);
          resp->setBody(std::string("Internal error: ") + e.what());
          cb(resp);
        }
      },
      [cb](const DrogonDbException& e) {
        LOG_ERROR << fmt::format("UserController::delete_user DB error: {}",
                                 e.base().what());
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody(e.base().what());
        cb(resp);
      },
      user_id);
}
