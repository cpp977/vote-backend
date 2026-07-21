/**
 *
 *  UserController.cpp
 *  Implementation of the admin-only user management controller.
 *
 *  Endpoint: GET /admin/users
 *  Returns: JSON array of usernames only (no sensitive data).
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

  // Query to retrieve all usernames only
  const std::string sql = "SELECT username FROM users ORDER BY username";

  dbClient->execSqlAsync(
      sql,
      [callbackPtr](const Result& r) {
        try {
          Json::Value arr(Json::arrayValue);
          for (const auto& row : r) {
            // Only return the username field - no other user data
            arr.append(row.at("username").as<std::string>());
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
