#include "vote-backend/controllers/QuestionController.hpp"

#include <drogon/drogon.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpAppFramework.h>
#include <json/json.h>

using drogon::orm::DrogonDbException;
using drogon::orm::Result;
using namespace drogon;

void QuestionController::getStats(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& cb,
    int questionId)
{
    // Optional tag filtering via query parameters ?tagKey=...&tagValue=...
    std::string tagKey   = req->getParameter("tagKey");
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
    if (filterTags)
    {
        // JSONB existence and equality operators.
        sql += " AND ua.tags ? $2::text AND ua.tags->>$2 = $3";
    }
    sql += " GROUP BY ao.id, ao.text";

    auto dbClient = app().getDbClient();
    if (filterTags)
    {
        dbClient->execSqlAsync(
            sql,
            [cb](const Result &r) {
                // Compute total votes.
                long long total = 0;
                for (size_t i = 0; i < r.size(); ++i)
                    total += r[i].at("cnt").as<long long>();

                Json::Value arr(Json::arrayValue);
                for (size_t i = 0; i < r.size(); ++i)
                {
                    Json::Value obj;
                    obj["answer_id"]   = Json::Value(static_cast<Json::Int64>(r[i].at("answer_id").as<long long>()));
                    obj["answer_text"] = Json::Value(r[i].at("answer_text").as<std::string>());
                    long long cnt = r[i].at("cnt").as<long long>();
                    obj["count"] = Json::Value(static_cast<Json::UInt64>(cnt));
                    double percent = total > 0 ? (static_cast<double>(cnt) * 100.0 / static_cast<double>(total)) : 0.0;
                    obj["percent"] = Json::Value(percent);
                    arr.append(obj);
                }
                auto resp = HttpResponse::newHttpJsonResponse(arr);
                cb(resp);
            },
            [cb](const DrogonDbException &e) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k500InternalServerError);
                resp->setBody(e.base().what());
                cb(resp);
            },
            static_cast<int64_t>(questionId), tagKey, tagValue);
    }
    else
    {
        dbClient->execSqlAsync(
            sql,
            [cb](const Result &r) {
                long long total = 0;
                for (size_t i = 0; i < r.size(); ++i)
                    total += r[i].at("cnt").as<long long>();

                Json::Value arr(Json::arrayValue);
                for (size_t i = 0; i < r.size(); ++i)
                {
                    Json::Value obj;
                    obj["answer_id"]   = Json::Value(Json::Int64(r[i].at("answer_id").as<long long>()));
                    obj["answer_text"] = Json::Value(r[i].at("answer_text").as<std::string>());
                    long long cnt = r[i].at("cnt").as<long long>();
                    obj["count"] = Json::Value(Json::UInt64(cnt));
                    double percent = total > 0 ? (static_cast<double>(cnt) * 100.0 / static_cast<double>(total)) : 0.0;
                    obj["percent"] = Json::Value(percent);
                    arr.append(obj);
                }
                auto resp = HttpResponse::newHttpJsonResponse(arr);
                cb(resp);
            },
            [cb](const DrogonDbException &e) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k500InternalServerError);
                resp->setBody(e.base().what());
                cb(resp);
            },
            static_cast<int64_t>(questionId));
    }
}
