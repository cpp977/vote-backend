#include "vote-backend/controllers/QuestionController.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <drogon/drogon.h>
#include <drogon/orm/Mapper.h>
#include <json/json.h>

#include <stdexcept>
#include <string>
#include <unordered_map>

#include "vote-backend/models/AnswerOptions.h"
#include "vote-backend/models/Categories.h"
#include "vote-backend/models/Questions.h"

using drogon::orm::DrogonDbException;
using drogon::orm::Result;
using namespace drogon;
using namespace drogon_model::vote;

void QuestionController::getQuestionsWithCategories(
    const drogon::HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback) {
  auto dbClient = app().getDbClient();

  // Step 1: Load all questions via the ORM.
  // Step 2: Collect category IDs and fetch names in a second query.
  // Step 3: Join in C++ and return.
  auto callbackPtr =
      std::make_shared<std::function<void(const HttpResponsePtr&)>>(
          std::move(callback));

  drogon::orm::Mapper<Questions> mapper(dbClient);
  mapper.findAll(
      [dbClient, callbackPtr](const std::vector<Questions>& questions) {
        try {
          if (questions.empty()) {
            (*callbackPtr)(HttpResponse::newHttpJsonResponse(
                Json::Value(Json::arrayValue)));
            return;
          }

          // Collect all category IDs into a comma-separated list.
          std::string categoryIds;
          for (size_t i = 0; i < questions.size(); ++i) {
            if (i > 0) categoryIds += ",";
            categoryIds += std::to_string(*questions[i].getCategoryId());
          }

          // Fetch all needed categories in one async query.
          std::string sql = "SELECT id, name FROM categories WHERE id IN (" +
                            categoryIds + ")";
          dbClient->execSqlAsync(
              sql,
              [questions, callbackPtr](const Result& r) {
                try {
                  // Build id -> name map.
                  std::unordered_map<int64_t, std::string> catMap;
                  for (const auto& crow : r) {
                    int64_t catId = crow.at("id").as<int64_t>();
                    catMap[catId] = crow.at("name").as<std::string>();
                  }

                  Json::Value arr(Json::arrayValue);
                  arr.resize(0);
                  for (const auto& question : questions) {
                    Json::Value q = question.toJson();
                    auto it = catMap.find(*question.getCategoryId());
                    q["category_name"] =
                        (it != catMap.end()) ? it->second : "Unknown";
                    arr.append(q);
                  }
                  (*callbackPtr)(HttpResponse::newHttpJsonResponse(arr));
                } catch (const std::exception& e) {
                  LOG_ERROR << "getQuestionsWithCategories failed: "
                            << e.what();
                  auto resp = HttpResponse::newHttpResponse();
                  resp->setStatusCode(k500InternalServerError);
                  resp->setBody(std::string("Internal error: ") + e.what());
                  (*callbackPtr)(resp);
                }
              },
              [callbackPtr](const DrogonDbException& e) {
                LOG_ERROR << "getQuestionsWithCategories DB error: "
                          << e.base().what();
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k500InternalServerError);
                resp->setBody(e.base().what());
                (*callbackPtr)(resp);
              });
        } catch (const std::exception& e) {
          LOG_ERROR << "getQuestionsWithCategories failed: " << e.what();
          auto resp = HttpResponse::newHttpResponse();
          resp->setStatusCode(k500InternalServerError);
          resp->setBody(std::string("Internal error: ") + e.what());
          (*callbackPtr)(resp);
        }
      },
      [callbackPtr](const DrogonDbException& e) {
        LOG_ERROR << "getQuestionsWithCategories DB error: " << e.base().what();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody(e.base().what());
        (*callbackPtr)(resp);
      });
}

void QuestionController::getAnswerOptions(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& cb, int questionId) {
  auto dbClient = app().getDbClient();
  auto callbackPtr =
      std::make_shared<std::function<void(const HttpResponsePtr&)>>(
          std::move(cb));

  drogon::orm::Mapper<Questions> mapper(dbClient);
  mapper.findByPrimaryKey(
      static_cast<int64_t>(questionId),
      [dbClient, callbackPtr](Questions question) {
        try {
          question.getAnswerOptions(
              dbClient,
              [callbackPtr](const std::vector<AnswerOptions>& options) {
                Json::Value arr(Json::arrayValue);
                for (const auto& option : options) {
                  arr.append(option.toJson());
                }
                (*callbackPtr)(HttpResponse::newHttpJsonResponse(arr));
              },
              [callbackPtr](const DrogonDbException& e) {
                LOG_ERROR << "getAnswerOptions DB error: " << e.base().what();
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k500InternalServerError);
                resp->setBody(e.base().what());
                (*callbackPtr)(resp);
              });
        } catch (const std::exception& e) {
          LOG_ERROR << "getAnswerOptions failed: " << e.what();
          auto resp = HttpResponse::newHttpResponse();
          resp->setStatusCode(k500InternalServerError);
          resp->setBody(std::string("Internal error: ") + e.what());
          (*callbackPtr)(resp);
        }
      },
      [callbackPtr](const DrogonDbException& e) {
        const drogon::orm::UnexpectedRows* s =
            dynamic_cast<const drogon::orm::UnexpectedRows*>(&e.base());
        if (s) {
          auto resp = HttpResponse::newHttpResponse();
          resp->setStatusCode(k404NotFound);
          (*callbackPtr)(resp);
          return;
        }
        LOG_ERROR << "getAnswerOptions DB error: " << e.base().what();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody(e.base().what());
        (*callbackPtr)(resp);
      });
}

void QuestionController::getQuestionsByLanguage(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& cb,
    const std::string& language) {
  auto dbClient = app().getDbClient();
  auto callbackPtr =
      std::make_shared<std::function<void(const HttpResponsePtr&)>>(
          std::move(cb));

  drogon::orm::Mapper<Questions> mapper(dbClient);
  mapper.findBy(
      drogon::orm::Criteria(Questions::Cols::_language, language),
      [callbackPtr](const std::vector<Questions>& questions) {
        try {
          Json::Value arr(Json::arrayValue);
          for (const auto& question : questions) {
            arr.append(question.toJson());
          }
          (*callbackPtr)(HttpResponse::newHttpJsonResponse(arr));
        } catch (const std::exception& e) {
          LOG_ERROR << "getQuestionsByLanguage failed: " << e.what();
          auto resp = HttpResponse::newHttpResponse();
          resp->setStatusCode(k500InternalServerError);
          resp->setBody(std::string("Internal error: ") + e.what());
          (*callbackPtr)(resp);
        }
      },
      [callbackPtr](const DrogonDbException& e) {
        LOG_ERROR << "getQuestionsByLanguage DB error: " << e.base().what();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody(e.base().what());
        (*callbackPtr)(resp);
      });
}

void QuestionController::getStats(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& cb, int questionId) {
  // Optional tag filtering via query parameters ?tagKey=...&tagValue=...
  std::string tagKey = req->getParameter("tagKey");
  std::string tagValue = req->getParameter("tagValue");
  bool filterTags = !tagKey.empty() && !tagValue.empty();

  // Build SQL. We use positional parameters ($1, $2, $3) for safety.
  std::string sql =
      "SELECT ao.id        AS answer_id, "
      "       ao.text      AS answer_text, "
      "       COUNT(ua.id) AS cnt "
      "FROM user_answers ua "
      "JOIN answer_options ao ON ua.answer_id = ao.id "
      "WHERE ua.question_id = $1";
  if (filterTags) {
    // JSONB existence and equality operators.
    sql += " AND ua.tags ? $2::text AND ua.tags->>$2 = $3";
  }
  sql += " GROUP BY ao.id, ao.text";

  auto dbClient = app().getDbClient();
  if (filterTags) {
    dbClient->execSqlAsync(
        sql,
        [cb](const Result& r) {
          // Compute total votes.
          long long total = 0;
          for (size_t i = 0; i < r.size(); ++i)
            total += r[i].at("cnt").as<long long>();

          Json::Value arr(Json::arrayValue);
          for (size_t i = 0; i < r.size(); ++i) {
            Json::Value obj;
            obj["answer_id"] = Json::Value(
                static_cast<Json::Int64>(r[i].at("answer_id").as<long long>()));
            obj["answer_text"] =
                Json::Value(r[i].at("answer_text").as<std::string>());
            long long cnt = r[i].at("cnt").as<long long>();
            obj["count"] = Json::Value(static_cast<Json::UInt64>(cnt));
            double percent = total > 0 ? (static_cast<double>(cnt) * 100.0 /
                                          static_cast<double>(total))
                                       : 0.0;
            obj["percent"] = Json::Value(percent);
            arr.append(obj);
          }
          auto resp = HttpResponse::newHttpJsonResponse(arr);
          cb(resp);
        },
        [cb](const DrogonDbException& e) {
          auto resp = HttpResponse::newHttpResponse();
          resp->setStatusCode(k500InternalServerError);
          resp->setBody(e.base().what());
          cb(resp);
        },
        static_cast<int64_t>(questionId), tagKey, tagValue);
  } else {
    dbClient->execSqlAsync(
        sql,
        [cb](const Result& r) {
          long long total = 0;
          for (size_t i = 0; i < r.size(); ++i)
            total += r[i].at("cnt").as<long long>();

          Json::Value arr(Json::arrayValue);
          for (size_t i = 0; i < r.size(); ++i) {
            Json::Value obj;
            obj["answer_id"] =
                Json::Value(Json::Int64(r[i].at("answer_id").as<long long>()));
            obj["answer_text"] =
                Json::Value(r[i].at("answer_text").as<std::string>());
            long long cnt = r[i].at("cnt").as<long long>();
            obj["count"] = Json::Value(Json::UInt64(cnt));
            double percent = total > 0 ? (static_cast<double>(cnt) * 100.0 /
                                          static_cast<double>(total))
                                       : 0.0;
            obj["percent"] = Json::Value(percent);
            arr.append(obj);
          }
          auto resp = HttpResponse::newHttpJsonResponse(arr);
          cb(resp);
        },
        [cb](const DrogonDbException& e) {
          auto resp = HttpResponse::newHttpResponse();
          resp->setStatusCode(k500InternalServerError);
          resp->setBody(e.base().what());
          cb(resp);
        },
        static_cast<int64_t>(questionId));
  }
}
