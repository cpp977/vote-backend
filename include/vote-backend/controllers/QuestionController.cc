#include "vote-backend/controllers/QuestionController.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>
#include <drogon/orm/Exception.h>
#include <drogon/orm/Mapper.h>
#include <drogon/utils/Utilities.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <json/json.h>
#include <trantor/utils/Logger.h>

#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "vote-backend/models/AnswerOptions.h"
#include "vote-backend/models/Questions.h"
#include "vote-backend/utils/UserIdHash.h"

using drogon::orm::DrogonDbException;
using drogon::orm::Result;
using namespace drogon;
using namespace drogon_model::vote;

namespace {
// Columns allowed in an ORDER BY clause (whitelisted to prevent SQL injection
// via the `sort` query parameter).
const std::set<std::string> kSortableColumns = {
    "id", "text", "category_id", "language", "min_age", "created_at"};

// Serialize a questions row (with the submission columns) to JSON.
Json::Value questionToJson(const drogon::orm::Row& row) {
  Json::Value q;
  q["id"] = Json::Value(static_cast<Json::Int64>(row["id"].as<long long>()));
  q["text"] = row["text"].as<std::string>();
  q["category_id"] =
      Json::Value(static_cast<Json::Int64>(row["category_id"].as<long long>()));
  q["language"] = row["language"].as<std::string>();
  q["min_age"] =
      Json::Value(static_cast<Json::Int64>(row["min_age"].as<long long>()));
  q["created_at"] = row["created_at"].as<std::string>();
  if (!row["submission_status"].isNull()) {
    q["submission_status"] = row["submission_status"].as<std::string>();
  }
  if (!row["submitted_by"].isNull()) {
    q["submitted_by"] = Json::Value(
        static_cast<Json::Int64>(row["submitted_by"].as<long long>()));
  }
  return q;
}
}  // namespace

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
      "JOIN categories c ON q.category_id = c.id "
      "WHERE q.submission_status = 'approved'";

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
      "WHERE q.text ILIKE $1 AND q.submission_status = 'approved' "
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

  int64_t user_id = req->attributes()->get<int64_t>("user_id");

  // Resolve the question and its visibility first. Unapproved questions are
  // only visible to their submitter; everyone else (including non-owner
  // regular users) gets 404 so pending content never leaks.
  *dbClient << "SELECT id, submission_status, submitted_by "
               "FROM questions WHERE id = $1::bigint"
            << static_cast<int64_t>(questionId) >>
      [dbClient, callbackPtr, user_id](const Result& r) {
        if (r.empty()) {
          auto resp = HttpResponse::newHttpResponse();
          resp->setStatusCode(k404NotFound);
          (*callbackPtr)(resp);
          return;
        }
        const std::string status = r[0]["submission_status"].as<std::string>();
        bool owner = (!r[0]["submitted_by"].isNull() &&
                      r[0]["submitted_by"].as<long long>() == user_id);
        if (status != "approved" && !owner) {
          auto resp = HttpResponse::newHttpResponse();
          resp->setStatusCode(k404NotFound);
          (*callbackPtr)(resp);
          return;
        }
        // Visible: load the answer options.
        *dbClient << "SELECT id, question_id, text FROM answer_options "
                     "WHERE question_id = $1::bigint ORDER BY id"
                  << static_cast<int64_t>(r[0]["id"].as<long long>()) >>
            [callbackPtr](const Result& opts) {
              Json::Value arr(Json::arrayValue);
              for (const auto& row : opts) {
                Json::Value o;
                o["id"] = Json::Value(
                    static_cast<Json::Int64>(row["id"].as<long long>()));
                o["question_id"] = Json::Value(static_cast<Json::Int64>(
                    row["question_id"].as<long long>()));
                o["text"] = row["text"].as<std::string>();
                arr.append(o);
              }
              (*callbackPtr)(HttpResponse::newHttpJsonResponse(arr));
            } >>
            [callbackPtr](const DrogonDbException& e) {
              LOG_ERROR << "getAnswerOptions DB error: " << e.base().what();
              auto resp = HttpResponse::newHttpResponse();
              resp->setStatusCode(k500InternalServerError);
              resp->setBody(e.base().what());
              (*callbackPtr)(resp);
            };
      } >>
      [callbackPtr](const DrogonDbException& e) {
        LOG_ERROR << "getAnswerOptions DB error: " << e.base().what();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody(e.base().what());
        (*callbackPtr)(resp);
      };
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
      "WHERE q.language = $1 AND q.submission_status = 'approved'";

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
      "WHERE ua.question_id = $1 "
      "  AND (SELECT submission_status FROM questions WHERE id = $1) = "
      "'approved'";
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
     .column = "q.language",
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
     .kind = FilterKind::LessEq,
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
      "LEFT JOIN categories c ON c.id = q.category_id AND c.language = "
      "q.language "
      "WHERE 1=1 AND q.submission_status = 'approved'";
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
      const auto& category_name = row.at("category_name");
      // The JOIN matches the category on language, so a NULL category_name
      // means the question is linked to a category that has no translation in
      // the question's language (data inconsistency). Surface it instead of
      // returning a question with a missing or foreign-language category name.
      if (category_name.isNull()) {
        LOG_ERROR << "Question " << row.at("id").as<long long>()
                  << " (language " << row.at("language").as<std::string>()
                  << ") references category "
                  << row.at("category_id").as<long long>()
                  << " which has no translation in that language";
        Json::Value err;
        err["error"] =
            "Data inconsistency: a question references a category that is "
            "not available in the question's language";
        auto resp = HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
        return;
      }
      Json::Value q;
      q["id"] =
          Json::Value(static_cast<Json::Int64>(row.at("id").as<long long>()));
      q["text"] = row.at("text").as<std::string>();
      q["language"] = row.at("language").as<std::string>();
      q["category_id"] = Json::Value(
          static_cast<Json::Int64>(row.at("category_id").as<long long>()));
      q["category_name"] = category_name.as<std::string>();
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

void QuestionController::answerQuestion(
    const drogon::HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback, int questionId) {
  // 1. Authentication: the JWT filter stores the numeric user id in the
  //    request attributes. A missing/zero id means the request was not
  //    authenticated with a usable subject.
  int64_t user_id = req->attributes()->get<int64_t>("user_id");
  if (user_id == 0) {
    Json::Value err;
    err["error"] = "Unauthenticated";
    auto resp = HttpResponse::newHttpJsonResponse(err);
    resp->setStatusCode(k401Unauthorized);
    callback(resp);
    return;
  }

  // 2. Request body must contain a numeric answer_id.
  auto jsonPtr = req->jsonObject();
  if (!jsonPtr) {
    Json::Value err;
    err["error"] = "No json object is found in the request";
    auto resp = HttpResponse::newHttpJsonResponse(err);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }
  if (!jsonPtr->isMember("answer_id") ||
      !(*jsonPtr)["answer_id"].isIntegral()) {
    Json::Value err;
    err["error"] = "Field 'answer_id' (integer) is required";
    auto resp = HttpResponse::newHttpJsonResponse(err);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }
  int64_t answer_id = (*jsonPtr)["answer_id"].asInt64();

  // Optional tags object, persisted as jsonb.
  std::string tags_json = "{}";
  if (jsonPtr->isMember("tags") && (*jsonPtr)["tags"].isObject()) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    tags_json = Json::writeString(builder, (*jsonPtr)["tags"]);
  }

  // Opaque, non-reversible hash of the user id (privacy-preserving): the
  // database stores this instead of the raw id.
  std::string hash_user_id =
      vote_backend::utils::user_id_hasher().hash(user_id);

  auto dbClient = app().getDbClient();
  auto callbackPtr =
      std::make_shared<std::function<void(const HttpResponsePtr&)>>(
          std::move(callback));
  int64_t qid = static_cast<int64_t>(questionId);

  // Only *approved* questions can be answered. Pending/rejected submissions
  // must not be reachable for answering; 404 keeps their existence hidden.
  *dbClient << "SELECT submission_status FROM questions WHERE id = $1::bigint"
            << qid >>
      [=](const Result& r) {
        if (r.empty() ||
            r[0]["submission_status"].as<std::string>() != "approved") {
          Json::Value err;
          err["error"] = "Question not found";
          auto resp = HttpResponse::newHttpJsonResponse(err);
          resp->setStatusCode(k404NotFound);
          (*callbackPtr)(resp);
          return;
        }

        // Perform both inserts atomically so a failure leaves no half-written
        // state:
        //   - question_user insert enforces "one answer per user". We use
        //     ON CONFLICT DO NOTHING and inspect the affected row count: a
        //     count of 0 means the (question_id, hash_user_id) pair already
        //     exists, i.e. the user has already answered.
        //   - user_answers insert only happens if the answer option actually
        //     belongs to the question.
        dbClient->newTransactionAsync([=](const std::shared_ptr<
                                          drogon::orm::Transaction>& trans) {
          if (!trans) {
            Json::Value err;
            err["error"] = "database timeout";
            auto resp = HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(k500InternalServerError);
            (*callbackPtr)(resp);
            return;
          }

          *trans << "INSERT INTO question_user (question_id, hash_user_id) "
                    "VALUES ($1::bigint, $2::text) "
                    "ON CONFLICT (question_id, hash_user_id) DO NOTHING"
                 << qid << hash_user_id >>
              [=](const Result& r) {
                if (r.affectedRows() == 0) {
                  // The user has already answered this question.
                  trans->rollback();
                  Json::Value err;
                  err["error"] = "You have already answered this question";
                  auto resp = HttpResponse::newHttpJsonResponse(err);
                  resp->setStatusCode(k409Conflict);
                  (*callbackPtr)(resp);
                  return;
                }
                *trans << "INSERT INTO user_answers (question_id, answer_id, "
                          "tags) "
                          "SELECT $1::bigint, $2::bigint, "
                          "COALESCE($3::jsonb, '{}'::jsonb) "
                          "WHERE EXISTS (SELECT 1 FROM answer_options ao "
                          "WHERE ao.id = $2::bigint AND ao.question_id = "
                          "$1::bigint) "
                          "RETURNING id"
                       << qid << answer_id << tags_json >>
                    [=](const Result& r) {
                      if (r.size() == 0) {
                        // The answer option does not belong to this question.
                        trans->rollback();
                        Json::Value err;
                        err["error"] =
                            "answer_id does not belong to the given question";
                        auto resp = HttpResponse::newHttpJsonResponse(err);
                        resp->setStatusCode(k400BadRequest);
                        (*callbackPtr)(resp);
                        return;
                      }
                      Json::Value ret;
                      ret["id"] =
                          static_cast<Json::Int64>(r[0]["id"].as<long long>());
                      ret["question_id"] = static_cast<Json::Int64>(qid);
                      ret["answer_id"] = static_cast<Json::Int64>(answer_id);
                      auto resp = HttpResponse::newHttpJsonResponse(ret);
                      resp->setStatusCode(k201Created);
                      trans->setCommitCallback(
                          [=](bool) { (*callbackPtr)(resp); });
                    } >>
                    [=](const DrogonDbException& e) {
                      trans->rollback();
                      LOG_ERROR << "answerQuestion user_answers insert failed: "
                                << e.base().what();
                      Json::Value err;
                      err["error"] = "database error";
                      auto resp = HttpResponse::newHttpJsonResponse(err);
                      resp->setStatusCode(k500InternalServerError);
                      (*callbackPtr)(resp);
                    };
              } >>
              [=](const DrogonDbException& e) {
                trans->rollback();
                LOG_ERROR << "answerQuestion question_user insert failed: "
                          << e.base().what();
                Json::Value err;
                err["error"] = "database error";
                auto resp = HttpResponse::newHttpJsonResponse(err);
                resp->setStatusCode(k500InternalServerError);
                (*callbackPtr)(resp);
              };
        });  // end newTransactionAsync
      }  // end status-check success lambda
      >>
      [=](const DrogonDbException& e) {
        LOG_ERROR << "answerQuestion status check failed: " << e.base().what();
        Json::Value err;
        err["error"] = "database error";
        auto resp = HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(k500InternalServerError);
        (*callbackPtr)(resp);
      };
}

namespace {
// Serialize a questions row (including the submission-workflow columns) to
// JSON for the "my submissions" and admin review-queue endpoints.
Json::Value submissionToJson(const drogon::orm::Row& row) {
  Json::Value q;
  q["id"] = Json::Value(static_cast<Json::Int64>(row["id"].as<long long>()));
  q["text"] = row["text"].as<std::string>();
  q["category_id"] =
      Json::Value(static_cast<Json::Int64>(row["category_id"].as<long long>()));
  q["language"] = row["language"].as<std::string>();
  q["min_age"] =
      Json::Value(static_cast<Json::Int64>(row["min_age"].as<long long>()));
  q["created_at"] = row["created_at"].as<std::string>();
  if (!row["submission_status"].isNull()) {
    q["submission_status"] = row["submission_status"].as<std::string>();
  }
  if (!row["submitted_by"].isNull()) {
    q["submitted_by"] = Json::Value(
        static_cast<Json::Int64>(row["submitted_by"].as<long long>()));
  }
  if (!row["reviewed_by"].isNull()) {
    q["reviewed_by"] = Json::Value(
        static_cast<Json::Int64>(row["reviewed_by"].as<long long>()));
  }
  return q;
}
}  // namespace

void QuestionController::getMySubmissions(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  int64_t user_id = req->attributes()->get<int64_t>("user_id");
  if (user_id == 0) {
    Json::Value err;
    err["error"] = "Unauthenticated";
    auto resp = HttpResponse::newHttpJsonResponse(err);
    resp->setStatusCode(k401Unauthorized);
    callback(resp);
    return;
  }

  auto dbClient = app().getDbClient();
  auto callbackPtr =
      std::make_shared<std::function<void(const HttpResponsePtr&)>>(
          std::move(callback));

  *dbClient << "SELECT id, text, category_id, language, min_age, created_at, "
               "submission_status, submitted_by, reviewed_by "
               "FROM questions "
               "WHERE submitted_by = $1::bigint "
               "ORDER BY created_at DESC"
            << user_id >>
      [callbackPtr](const Result& r) {
        Json::Value arr(Json::arrayValue);
        for (const auto& row : r) {
          arr.append(submissionToJson(row));
        }
        (*callbackPtr)(HttpResponse::newHttpJsonResponse(arr));
      } >>
      [callbackPtr](const DrogonDbException& e) {
        LOG_ERROR << "getMySubmissions DB error: " << e.base().what();
        Json::Value err;
        err["error"] = "database error";
        auto resp = HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(k500InternalServerError);
        (*callbackPtr)(resp);
      };
}

void QuestionController::listSubmissions(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  auto dbClient = app().getDbClient();
  auto callbackPtr =
      std::make_shared<std::function<void(const HttpResponsePtr&)>>(
          std::move(callback));

  auto handle = [callbackPtr](const Result& r) {
    Json::Value arr(Json::arrayValue);
    for (const auto& row : r) {
      arr.append(submissionToJson(row));
    }
    (*callbackPtr)(HttpResponse::newHttpJsonResponse(arr));
  };
  auto onError = [callbackPtr](const DrogonDbException& e) {
    LOG_ERROR << "listSubmissions DB error: " << e.base().what();
    Json::Value err;
    err["error"] = "database error";
    auto resp = HttpResponse::newHttpJsonResponse(err);
    resp->setStatusCode(k500InternalServerError);
    (*callbackPtr)(resp);
  };

  // Optional ?status=pending|rejected filter; otherwise the full review queue
  // (everything that is not yet approved) is returned.
  std::string status = req->getParameter("status");
  if (!status.empty() && (status == "pending" || status == "rejected")) {
    *dbClient << "SELECT id, text, category_id, language, min_age, created_at, "
                 "submission_status, submitted_by, reviewed_by "
                 "FROM questions "
                 "WHERE submission_status = $1::text "
                 "ORDER BY created_at DESC"
              << status >>
        handle >> onError;
  } else {
    *dbClient << "SELECT id, text, category_id, language, min_age, created_at, "
                 "submission_status, submitted_by, reviewed_by "
                 "FROM questions "
                 "WHERE submission_status <> 'approved' "
                 "ORDER BY created_at DESC" >>
        handle >> onError;
  }
}

void QuestionController::approveQuestion(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    int questionId) {
  // Reviewed-by is taken from the verified admin JWT; clients cannot forge it.
  int64_t admin_id = req->attributes()->get<int64_t>("user_id");

  auto dbClient = app().getDbClient();
  auto callbackPtr =
      std::make_shared<std::function<void(const HttpResponsePtr&)>>(
          std::move(callback));

  *dbClient << "UPDATE questions "
               "SET submission_status = 'approved', "
               "    reviewed_by = $2::bigint, "
               "    reviewed_at = NOW() "
               "WHERE id = $1::bigint "
               "RETURNING id, text, category_id, language, min_age, "
               "created_at, submission_status, submitted_by, reviewed_by"
            << static_cast<int64_t>(questionId) << admin_id >>
      [callbackPtr](const Result& r) {
        if (r.empty()) {
          auto resp = HttpResponse::newHttpResponse();
          resp->setStatusCode(k404NotFound);
          (*callbackPtr)(resp);
          return;
        }
        (*callbackPtr)(
            HttpResponse::newHttpJsonResponse(submissionToJson(r[0])));
      } >>
      [callbackPtr](const DrogonDbException& e) {
        LOG_ERROR << "approveQuestion DB error: " << e.base().what();
        Json::Value err;
        err["error"] = "database error";
        auto resp = HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(k500InternalServerError);
        (*callbackPtr)(resp);
      };
}

void QuestionController::rejectQuestion(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    int questionId) {
  int64_t admin_id = req->attributes()->get<int64_t>("user_id");

  auto dbClient = app().getDbClient();
  auto callbackPtr =
      std::make_shared<std::function<void(const HttpResponsePtr&)>>(
          std::move(callback));

  *dbClient << "UPDATE questions "
               "SET submission_status = 'rejected', "
               "    reviewed_by = $2::bigint, "
               "    reviewed_at = NOW() "
               "WHERE id = $1::bigint "
               "RETURNING id, text, category_id, language, min_age, "
               "created_at, submission_status, submitted_by, reviewed_by"
            << static_cast<int64_t>(questionId) << admin_id >>
      [callbackPtr](const Result& r) {
        if (r.empty()) {
          auto resp = HttpResponse::newHttpResponse();
          resp->setStatusCode(k404NotFound);
          (*callbackPtr)(resp);
          return;
        }
        (*callbackPtr)(
            HttpResponse::newHttpJsonResponse(submissionToJson(r[0])));
      } >>
      [callbackPtr](const DrogonDbException& e) {
        LOG_ERROR << "rejectQuestion DB error: " << e.base().what();
        Json::Value err;
        err["error"] = "database error";
        auto resp = HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(k500InternalServerError);
        (*callbackPtr)(resp);
      };
}

void QuestionController::getOne(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback, int questionId) {
  auto dbClient = app().getDbClient();
  auto callbackPtr =
      std::make_shared<std::function<void(const HttpResponsePtr&)>>(
          std::move(callback));

  // Public endpoint: only approved questions are visible.
  *dbClient << "SELECT id, text, category_id, language, min_age, created_at, "
               "submission_status, submitted_by "
               "FROM questions "
               "WHERE id = $1::bigint AND submission_status = 'approved'"
            << static_cast<int64_t>(questionId) >>
      [callbackPtr](const Result& r) {
        if (r.empty()) {
          auto resp = HttpResponse::newHttpResponse();
          resp->setStatusCode(k404NotFound);
          (*callbackPtr)(resp);
          return;
        }
        (*callbackPtr)(HttpResponse::newHttpJsonResponse(questionToJson(r[0])));
      } >>
      [callbackPtr](const DrogonDbException& e) {
        LOG_ERROR << "QuestionController::getOne DB error: " << e.base().what();
        Json::Value err;
        err["error"] = "database error";
        auto resp = HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(k500InternalServerError);
        (*callbackPtr)(resp);
      };
}

void QuestionController::get(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback) {
  auto dbClient = app().getDbClient();
  auto callbackPtr =
      std::make_shared<std::function<void(const HttpResponsePtr&)>>(
          std::move(callback));

  // Public endpoint: only approved questions are visible.
  std::string sql =
      "SELECT id, text, category_id, language, min_age, created_at, "
      "submission_status, submitted_by "
      "FROM questions "
      "WHERE submission_status = 'approved'";

  // ORDER BY (whitelisted columns only).
  std::string orderBy = " ORDER BY created_at DESC";
  auto& parameters = req->parameters();
  auto it = parameters.find("sort");
  if (it != parameters.end() && !it->second.empty()) {
    auto fields = drogon::utils::splitString(it->second, ",");
    std::string ob;
    for (auto& field : fields) {
      if (field.empty()) continue;
      std::string dir = "ASC";
      std::string col = field;
      if (col[0] == '+') {
        col = col.substr(1);
      } else if (col[0] == '-') {
        col = col.substr(1);
        dir = "DESC";
      }
      if (kSortableColumns.count(col)) {
        if (!ob.empty()) ob += ", ";
        ob += col + " " + dir;
      }
    }
    if (!ob.empty()) orderBy = " ORDER BY " + ob;
  }
  sql += orderBy;

  // LIMIT / OFFSET (integers parsed from query parameters, safe to inline).
  long long limit = -1;
  long long offset = 0;
  it = parameters.find("limit");
  if (it != parameters.end()) {
    try {
      limit = std::stoll(it->second);
    } catch (...) {
    }
  }
  it = parameters.find("offset");
  if (it != parameters.end()) {
    try {
      offset = std::stoll(it->second);
    } catch (...) {
    }
  }
  if (offset > 0) sql += " OFFSET " + std::to_string(offset);
  if (limit > 0) sql += " LIMIT " + std::to_string(limit);

  *dbClient << sql >> [callbackPtr](const Result& r) {
    Json::Value arr(Json::arrayValue);
    for (const auto& row : r) {
      arr.append(questionToJson(row));
    }
    (*callbackPtr)(HttpResponse::newHttpJsonResponse(arr));
  } >> [callbackPtr](const DrogonDbException& e) {
    LOG_ERROR << "QuestionController::get DB error: " << e.base().what();
    Json::Value err;
    err["error"] = "database error";
    auto resp = HttpResponse::newHttpJsonResponse(err);
    resp->setStatusCode(k500InternalServerError);
    (*callbackPtr)(resp);
  };
}

void QuestionController::submitQuestion(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback) {
  auto jsonPtr = req->jsonObject();
  if (!jsonPtr) {
    Json::Value err;
    err["error"] = "No json object is found in the request";
    auto resp = HttpResponse::newHttpJsonResponse(err);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }

  // --- Validation (mirrors the generated model rules) -----------------------
  if (!(*jsonPtr).isMember("text") || !(*jsonPtr)["text"].isString() ||
      (*jsonPtr)["text"].asString().empty()) {
    Json::Value err;
    err["error"] = "Field 'text' (string, non-empty) is required";
    auto resp = HttpResponse::newHttpJsonResponse(err);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }
  if (!(*jsonPtr).isMember("category_id") ||
      !(*jsonPtr)["category_id"].isIntegral()) {
    Json::Value err;
    err["error"] = "Field 'category_id' (integer) is required";
    auto resp = HttpResponse::newHttpJsonResponse(err);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }
  if (!(*jsonPtr).isMember("language") || !(*jsonPtr)["language"].isString() ||
      (*jsonPtr)["language"].asString().size() != 2) {
    Json::Value err;
    err["error"] = "Field 'language' (2-char code) is required";
    auto resp = HttpResponse::newHttpJsonResponse(err);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }

  int min_age = 0;
  if ((*jsonPtr).isMember("min_age") && (*jsonPtr)["min_age"].isIntegral()) {
    min_age = (*jsonPtr)["min_age"].asInt();
  }
  if (min_age < 0) min_age = 0;

  // --- Answer options (required, non-empty) ---------------------------------
  // Each option is either a plain string or an object carrying a non-empty
  // "text". We normalize them to a list of texts and persist them in the
  // answer_options table pointing at the newly created question, so a question
  // is always submitted together with the options it can be voted on.
  if (!(*jsonPtr).isMember("answer_options") ||
      !(*jsonPtr)["answer_options"].isArray()) {
    Json::Value err;
    err["error"] =
        "Field 'answer_options' (array of strings or objects) is required";
    auto resp = HttpResponse::newHttpJsonResponse(err);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }
  const auto& answerOptions = (*jsonPtr)["answer_options"];
  // A question without any answer option cannot be voted on, so require at
  // least one. The upper bound guards against unreasonably large payloads.
  constexpr size_t kMaxAnswerOptions = 50;
  if (answerOptions.empty()) {
    Json::Value err;
    err["error"] = "Field 'answer_options' must contain at least one entry";
    auto resp = HttpResponse::newHttpJsonResponse(err);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }
  if (answerOptions.size() > kMaxAnswerOptions) {
    Json::Value err;
    err["error"] = "Field 'answer_options' must not contain more than " +
                   std::to_string(kMaxAnswerOptions) + " entries";
    auto resp = HttpResponse::newHttpJsonResponse(err);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }
  std::vector<std::string> optionTexts;
  optionTexts.reserve(answerOptions.size());
  for (const auto& opt : answerOptions) {
    std::string text;
    if (opt.isString()) {
      text = opt.asString();
    } else if (opt.isObject() && opt.isMember("text") &&
               opt["text"].isString()) {
      text = opt["text"].asString();
    } else {
      Json::Value err;
      err["error"] =
          "Each answer option must be a string or an object with a "
          "non-empty 'text' field";
      auto resp = HttpResponse::newHttpJsonResponse(err);
      resp->setStatusCode(k400BadRequest);
      callback(resp);
      return;
    }
    if (text.empty()) {
      Json::Value err;
      err["error"] = "answer option text must not be empty";
      auto resp = HttpResponse::newHttpJsonResponse(err);
      resp->setStatusCode(k400BadRequest);
      callback(resp);
      return;
    }
    optionTexts.push_back(std::move(text));
  }

  // The submitter is taken from the verified JWT, never from the request body,
  // so a user cannot forge ownership or self-approve.
  int64_t user_id = req->attributes()->get<int64_t>("user_id");
  if (user_id == 0) {
    Json::Value err;
    err["error"] = "Unauthenticated";
    auto resp = HttpResponse::newHttpJsonResponse(err);
    resp->setStatusCode(k401Unauthorized);
    callback(resp);
    return;
  }

  const std::string text = (*jsonPtr)["text"].asString();
  const int64_t category_id = (*jsonPtr)["category_id"].as<int64_t>();
  const std::string language = (*jsonPtr)["language"].asString();

  auto dbClient = app().getDbClient();
  auto callbackPtr =
      std::make_shared<std::function<void(const HttpResponsePtr&)>>(
          std::move(callback));

  // Insert the question AND its answer options inside a single transaction so
  // the submission is atomic: if any insert fails the whole submission is
  // rolled back and no partial record is left behind.
  dbClient->newTransactionAsync([=](const std::shared_ptr<
                                    drogon::orm::Transaction>& trans) {
    if (!trans) {
      Json::Value err;
      err["error"] = "database timeout";
      auto resp = HttpResponse::newHttpJsonResponse(err);
      resp->setStatusCode(k500InternalServerError);
      (*callbackPtr)(resp);
      return;
    }

    *trans << "INSERT INTO questions (text, category_id, language, min_age, "
              "submission_status, submitted_by) "
              "VALUES ($1::text, $2::bigint, $3::char(2), $4::int, "
              "'pending', $5::bigint) "
              "RETURNING id, text, category_id, language, min_age, "
              "created_at, submission_status, submitted_by"
           << text << category_id << language << min_age << user_id >>
        [=](const Result& r) {
          const int64_t new_qid = r[0]["id"].as<long long>();

          // Build a single multi-row INSERT for all answer options. Every
          // text is bound as its own parameter (no string interpolation) so
          // the statement is injection-safe and the option order is
          // preserved (placeholder $1 is the question id).
          std::string optSql =
              "INSERT INTO answer_options (question_id, text) VALUES ";
          for (size_t i = 0; i < optionTexts.size(); ++i) {
            if (i) optSql += ", ";
            optSql += fmt::format("($1::bigint, ${}::text)", i + 2);
          }
          optSql += " RETURNING id, text";

          auto binder = *trans << optSql;
          binder << new_qid;
          for (const auto& t : optionTexts) binder << t;
          binder >> [=](const Result& opts) {
            Json::Value answers(Json::arrayValue);
            for (const auto& row : opts) {
              Json::Value o;
              o["id"] = Json::Value(
                  static_cast<Json::Int64>(row["id"].as<long long>()));
              o["question_id"] = Json::Value(static_cast<Json::Int64>(new_qid));
              o["text"] = row["text"].as<std::string>();
              answers.append(o);
            }

            Json::Value out = questionToJson(r[0]);
            out["answer_options"] = answers;
            auto resp = HttpResponse::newHttpJsonResponse(out);
            resp->setStatusCode(k201Created);
            // The answer_options INSERT is the last statement of the
            // transaction; commit it and respond once the commit lands.
            trans->setCommitCallback([=](bool) { (*callbackPtr)(resp); });
          } >> [=](const DrogonDbException& e) {
            trans->rollback();
            LOG_ERROR << "QuestionController::submitQuestion answer_options "
                         "insert failed: "
                      << e.base().what();
            Json::Value err;
            err["error"] = "database error";
            auto resp = HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(k500InternalServerError);
            (*callbackPtr)(resp);
          };
        }  // end question insert success lambda
        >> [=](const DrogonDbException& e) {
            trans->rollback();
            LOG_ERROR << "QuestionController::submitQuestion question insert "
                         "failed: "
                      << e.base().what();
            Json::Value err;
            err["error"] = "database error";
            auto resp = HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(k500InternalServerError);
            (*callbackPtr)(resp);
          };  // end question insert error lambda
  });  // end newTransactionAsync
}
