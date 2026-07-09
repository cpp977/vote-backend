#include "vote-backend/controllers/QuestionController.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <drogon/drogon.h>
#include <drogon/orm/Mapper.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <json/json.h>
#include <trantor/utils/Logger.h>

#include <stdexcept>
#include <string>

#include "vote-backend/models/AnswerOptions.h"
#include "vote-backend/models/Questions.h"

using drogon::orm::DrogonDbException;
using drogon::orm::Result;
using namespace drogon;
using namespace drogon_model::vote;

void QuestionController::getQuestionsWithCategories(
    const drogon::HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback) {
  auto dbClient = app().getDbClient();
  auto callbackPtr =
      std::make_shared<std::function<void(const HttpResponsePtr&)>>(
          std::move(callback));

  std::string sql =
      "SELECT q.id, q.text, q.language, q.category_id, "
      "       q.min_age, q.created_at, "
      "       c.name AS category_name "
      "FROM questions q "
      "JOIN categories c ON q.category_id = c.id";

  dbClient->execSqlAsync(
      sql,
      [callbackPtr](const Result& r) {
        try {
          Json::Value arr(Json::arrayValue);
          for (const auto& row : r) {
            Json::Value q;
            q["id"] = Json::Value(
                static_cast<Json::Int64>(row.at("id").as<long long>()));
            q["text"] = row.at("text").as<std::string>();
            q["language"] = row.at("language").as<std::string>();
            q["category_id"] = Json::Value(static_cast<Json::Int64>(
                row.at("category_id").as<long long>()));
            q["category_name"] = row.at("category_name").as<std::string>();
            q["min_age"] = Json::Value(
                static_cast<Json::Int64>(row.at("min_age").as<long long>()));
            q["created_at"] = row.at("created_at").as<std::string>();
            arr.append(q);
          }
          (*callbackPtr)(HttpResponse::newHttpJsonResponse(arr));
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

void QuestionController::searchQuestions(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
  auto dbClient = app().getDbClient();
  auto callbackPtr =
      std::make_shared<std::function<void(const HttpResponsePtr&)>>(
          std::move(cb));

  // Get search term from query parameter
  std::string searchTerm = req->getParameter("q");

  std::string sql =
      "SELECT q.id, q.text, q.language, q.category_id, "
      "       c.name AS category_name, q.created_at "
      "FROM questions q "
      "JOIN categories c ON q.category_id = c.id "
      "WHERE q.text ILIKE $1 "
      "ORDER BY q.created_at DESC";

  dbClient->execSqlAsync(
      sql,
      [callbackPtr](const Result& r) {
        try {
          Json::Value arr(Json::arrayValue);
          for (const auto& row : r) {
            Json::Value q;
            q["id"] = Json::Value(
                static_cast<Json::Int64>(row.at("id").as<long long>()));
            q["text"] = row.at("text").as<std::string>();
            q["language"] = row.at("language").as<std::string>();
            q["category_id"] = Json::Value(static_cast<Json::Int64>(
                row.at("category_id").as<long long>()));
            q["category_name"] = row.at("category_name").as<std::string>();
            q["created_at"] = row.at("created_at").as<std::string>();
            arr.append(q);
          }
          (*callbackPtr)(HttpResponse::newHttpJsonResponse(arr));
        } catch (const std::exception& e) {
          LOG_ERROR << "searchQuestions failed: " << e.what();
          auto resp = HttpResponse::newHttpResponse();
          resp->setStatusCode(k500InternalServerError);
          resp->setBody(std::string("Internal error: ") + e.what());
          (*callbackPtr)(resp);
        }
      },
      [callbackPtr](const DrogonDbException& e) {
        LOG_ERROR << "searchQuestions DB error: " << e.base().what();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody(e.base().what());
        (*callbackPtr)(resp);
      },
      "%" + searchTerm + "%");
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

  std::string sql =
      "SELECT q.id, q.text, q.language, q.category_id, "
      "       c.name AS category_name "
      "FROM questions q "
      "JOIN categories c ON q.category_id = c.id "
      "WHERE q.language = $1";

  dbClient->execSqlAsync(
      sql,
      [callbackPtr](const Result& r) {
        try {
          Json::Value arr(Json::arrayValue);
          for (const auto& row : r) {
            Json::Value q;
            q["id"] = Json::Value(
                static_cast<Json::Int64>(row.at("id").as<long long>()));
            q["text"] = row.at("text").as<std::string>();
            q["language"] = row.at("language").as<std::string>();
            q["category_id"] = Json::Value(static_cast<Json::Int64>(
                row.at("category_id").as<long long>()));
            q["category_name"] = row.at("category_name").as<std::string>();
            arr.append(q);
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
      },
      language);
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

enum class FilterKind { Equal, GreaterEq, LessEq, ILike, InArray };

struct FilterDef {
  std::string jsonKey;  // key expected in the JSON payload
  std::string column;   // trusted, server-defined column name
  FilterKind kind;
  std::string castSuffix = "";  // e.g. "::int", "::timestamp"
  std::string nullValue = "";   // string repr. of the empty value; the WHERE
                                // clause is skipped when the value matches it
};

// Server-defined, not derived from user input -> safe from injection
static const std::vector<FilterDef> filters = {
    {.jsonKey = "language",
     .column = "language",
     .kind = FilterKind::Equal,
     .nullValue = ""},  // default: empty string -> skip WHERE clause
    {.jsonKey = "search",
     .column = "text",
     .kind = FilterKind::ILike,  // trigram-indexed column
     .nullValue = ""},           // default: empty string -> skip WHERE clause
    {.jsonKey = "categoryIds",
     .column = "category_id",
     .kind = FilterKind::InArray,
     .castSuffix = "::int[]",
     .nullValue = "[]"},  // default: empty list -> skip WHERE clause
    {.jsonKey = "age",
     .column = "min_age",
     .kind = FilterKind::GreaterEq,
     .castSuffix = "::int",
     .nullValue = "0"},  // default: "0" -> skip WHERE clause
};

// Returns a canonical string representation of a filter value. Strings are
// returned verbatim; other JSON values (e.g. arrays) are serialized compactly
// so that an empty array becomes "[]".
static std::string filterValueRepr(const Json::Value& value) {
  if (value.isString()) {
    return value.asString();
  }
  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";
  return Json::writeString(builder, value);
}

void appendFilter(const FilterDef& f, const Json::Value& value,
                  std::string& sql, std::vector<std::string>& params,
                  int& idx) {
  // Skip the WHERE clause when the value equals the filter's null (empty)
  // value.
  if (filterValueRepr(value) == f.nullValue) {
    return;
  }

  switch (f.kind) {
    case FilterKind::Equal:
      sql += fmt::format(" AND {} = ${}{}", f.column, idx++, f.castSuffix);
      params.push_back(value.asString());
      break;

    case FilterKind::GreaterEq:
      sql += fmt::format(" AND {} >= ${}{}", f.column, idx++, f.castSuffix);
      params.push_back(value.asString());
      break;

    case FilterKind::LessEq:
      sql += fmt::format(" AND {} <= ${}{}", f.column, idx++, f.castSuffix);
      params.push_back(value.asString());
      break;

    case FilterKind::ILike:
      sql += fmt::format(" AND {} ILIKE ${}", f.column, idx++);
      // wildcard wrapping happens on the VALUE, not the SQL text
      params.push_back("%" + value.asString() + "%");
      break;

    case FilterKind::InArray: {
      // value is expected to be a JSON array, e.g. [1, 2, 5]
      std::vector<std::string> elems;
      for (const auto& v : value) elems.push_back(v.asString());

      sql += fmt::format(" AND {} = ANY(${}{})", f.column, idx++, f.castSuffix);
      // bind the whole list as a single Postgres array literal
      params.push_back(fmt::format("{{{}}}", fmt::join(elems, ",")));
      break;
    }
  }
}

void QuestionController::restSearchQuestions(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback) {
  auto json = req->getJsonObject();
  if (!json) {
    const std::string& body = std::string(req->getBody());

    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    Json::Value root;
    std::string errs;

    bool ok =
        reader->parse(body.data(), body.data() + body.size(), &root, &errs);

    if (!ok) {
      LOG_WARN << "Invalid JSON in request body: " << errs;
      LOG_WARN << "Raw body was: " << body;
    } else {
      // rare: Drogon's own parser rejected it but JsonCpp directly succeeds —
      // usually means Content-Type wasn't application/json
      LOG_WARN
          << "Body parsed independently but Drogon didn't recognize it as JSON "
             "(check Content-Type header: "
          << req->getHeader("Content-Type") << ")";
    }

    auto resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }

  // Extract pagination parameters with defaults
  int offset = 0;
  int limit = 50;

  if (json->isMember("offset")) {
    try {
      offset = (*json)["offset"].asInt();
      if (offset < 0) {
        offset = 0;
      }
    } catch (const std::exception& e) {
      LOG_WARN << "Invalid offset value, using default (0): " << e.what();
    }
  }

  if (json->isMember("limit")) {
    try {
      limit = (*json)["limit"].asInt();
      if (limit < 0) {
        limit = 50;
      }
      // Apply a reasonable maximum limit to prevent abuse
      if (limit > 1000) {
        limit = 1000;
      }
    } catch (const std::exception& e) {
      LOG_WARN << "Invalid limit value, using default (50): " << e.what();
    }
  }

  std::string sql =
      "SELECT q.id, q.text, q.language, q.category_id, "
      "c.name AS category_name FROM questions q "
      "JOIN categories c ON q.category_id = c.id WHERE 1=1";
  std::vector<std::string> params;
  int idx = 1;

  for (const auto& f : filters) {
    Json::Value value;
    if (json->isMember(f.jsonKey)) {
      value = (*json)[f.jsonKey];
    } else if (f.kind == FilterKind::InArray) {
      // Default for array filters is an empty list; appendFilter() will skip
      // it.
      value = Json::Value(Json::arrayValue);
    } else {
      // Default for scalar filters is the empty value; appendFilter() skips it.
      value = Json::Value(f.nullValue);
    }
    appendFilter(f, value, sql, params, idx);
  }

  // Add ORDER BY to ensure consistent pagination
  sql += " ORDER BY q.created_at DESC";

  // Add LIMIT and OFFSET for pagination
  if (limit > 0) {
    sql += fmt::format(" LIMIT ${}", idx++);
    params.push_back(std::to_string(limit));
  }
  if (offset > 0) {
    sql += fmt::format(" OFFSET ${}", idx++);
    params.push_back(std::to_string(offset));
  }

  LOG_DEBUG << fmt::format("SQL: {} | params: [{}]", sql,
                           fmt::join(params, ", "));

  auto dbClientPtr = drogon::app().getDbClient();
  auto binder = *dbClientPtr << sql;

  for (const auto& p : params) binder << p;

  binder >> [callback](const drogon::orm::Result& result) {
    Json::Value ret;
    for (const auto& row : result) {
      Json::Value q;
      q["id"] =
          Json::Value(static_cast<Json::Int64>(row.at("id").as<long long>()));
      q["text"] = row.at("text").as<std::string>();
      q["language"] = row.at("language").as<std::string>();
      q["category_id"] = Json::Value(
          static_cast<Json::Int64>(row.at("category_id").as<long long>()));
      q["category_name"] = row.at("category_name").as<std::string>();
      ret.append(q);
    }
    callback(HttpResponse::newHttpJsonResponse(ret));
  } >> [callback](const drogon::orm::DrogonDbException& e) {
    Json::Value err;
    err["error"] = e.base().what();
    auto resp = HttpResponse::newHttpJsonResponse(err);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  };
}
