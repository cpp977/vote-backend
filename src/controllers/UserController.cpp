/**
 *
 *  UserController.cpp
 *  Implementation of the admin-only user management controller.
 *
 *  Endpoint: GET /admin/users
 *  Returns: JSON array of user objects containing id and username fields only.
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
      "nationality, created_at, updated_at, is_admin "
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
