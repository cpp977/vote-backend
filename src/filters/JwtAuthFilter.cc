/**
 *
 *  JwtAuthFilter.cc
 *  Implementation of the JWT authentication filter.
 */

#include "vote-backend/filters/JwtAuthFilter.h"
#include "vote-backend/utils/JwtService.h"

#include <drogon/HttpResponse.h>
#include <drogon/HttpAppFramework.h>
#include <json/json.h>
#include <unordered_set>

using namespace drogon;
using namespace vote_backend::utils;

namespace
{
// Publicly accessible paths – we skip auth for these.
static const std::unordered_set<std::string> kPublicPaths = {
    "/login",
    "/register",
    "/refresh"
};
}

void JwtAuthFilter::doFilter(const HttpRequestPtr &req,
                             FilterCallback &&filterCb,
                             FilterChainCallback &&nextCb)
{
    // Skip authentication for public endpoints.
    if (kPublicPaths.count(req->path()))
    {
        nextCb();
        return;
    }

    auto authHeader = req->getHeader("Authorization");
    if (authHeader.empty() || authHeader.find("Bearer ") != 0)
    {
        Json::Value err;
        err["error"] = "Missing or malformed Authorization header";
        auto resp = HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(k401Unauthorized);
        filterCb(resp);
        return;
    }

    std::string token = authHeader.substr(7); // strip "Bearer "

    // Build JwtService from app config (make_jwt_service is in AuthController.cc anonymous namespace)
    auto &custom_cfg = drogon::app().getCustomConfig();
    vote_backend::utils::JwtService jwt_svc(
        custom_cfg["jwt_secret"].asString(),
        custom_cfg["jwt_access_token_expiry_minutes"].asInt(),
        custom_cfg["jwt_refresh_token_expiry_days"].asInt());

    Json::Value claims = jwt_svc.verify_token(token);

    if (claims.isNull() || claims.empty())
    {
        Json::Value err;
        err["error"] = "Invalid or expired token";
        auto resp = HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(k401Unauthorized);
        filterCb(resp);
        return;
    }

    // Store user_id in request attributes for downstream handlers.
    int64_t user_id = std::stoll(claims["sub"].asString());
    req->attributes()->insert("user_id", user_id);

    // Continue processing.
    nextCb();
}
