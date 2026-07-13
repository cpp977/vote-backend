#include "vote-backend/controllers/CategoryController.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <drogon/orm/DbClient.h>
#include <drogon/orm/Exception.h>
#include <json/json.h>
#include <trantor/utils/Logger.h>

#include <stdexcept>
#include <string>

using drogon::orm::DrogonDbException;
using namespace drogon;

void CategoryController::getCategoriesByLanguage(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& cb,
    const std::string& language) {
  auto dbClient = app().getDbClient();
  auto callbackPtr =
      std::make_shared<std::function<void(const HttpResponsePtr&)>>(
          std::move(cb));

  // language is a CHAR(2) column, so a small index lookup returns only the
  // categories of the requested language.
  std::string sql =
      "SELECT id, name, language "
      "FROM categories "
      "WHERE language = $1 "
      "ORDER BY id";

  dbClient->execSqlAsync(
      sql,
      [callbackPtr](const orm::Result& r) {
        try {
          Json::Value arr(Json::arrayValue);
          for (const auto& row : r) {
            Json::Value c;
            c["id"] = Json::Value(
                static_cast<Json::Int64>(row.at("id").as<long long>()));
            c["name"] = row.at("name").as<std::string>();
            c["language"] = row.at("language").as<std::string>();
            arr.append(c);
          }
          (*callbackPtr)(HttpResponse::newHttpJsonResponse(arr));
        } catch (const std::exception& e) {
          LOG_ERROR << "getCategoriesByLanguage failed: " << e.what();
          auto resp = HttpResponse::newHttpResponse();
          resp->setStatusCode(k500InternalServerError);
          resp->setBody(std::string("Internal error: ") + e.what());
          (*callbackPtr)(resp);
        }
      },
      [callbackPtr](const DrogonDbException& e) {
        LOG_ERROR << "getCategoriesByLanguage DB error: " << e.base().what();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody(e.base().what());
        (*callbackPtr)(resp);
      },
      language);
}
