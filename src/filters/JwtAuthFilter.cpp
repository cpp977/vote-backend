/**
 *
 *  JwtAuthFilter.cpp
 *  Implementation of the JWT authentication filter.
 */

#include "vote-backend/filters/JwtAuthFilter.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <json/json.h>

#include <unordered_set>

#include "vote-backend/utils/JwtService.hpp"

using namespace drogon;
using namespace vote_backend::utils;

namespace {
// Publicly accessible paths – we skip auth for these.
static const std::unordered_set<std::string> kPublicPaths = {
    "/login", "/register", "/refresh"};
}  // namespace

void JwtAuthFilter::doFilter(const HttpRequestPtr& req,
                             FilterCallback&& filterCb,
                             FilterChainCallback&& nextCb) {
  // Skip authentication for public endpoints.
  if (kPublicPaths.count(req->path())) {
    nextCb();
    return;
  }

  auto authHeader = req->getHeader("Authorization");
  if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
    Json::Value err;
    err["error"] = "Missing or malformed Authorization header";
    auto resp = HttpResponse::newHttpJsonResponse(err);
    resp->setStatusCode(k401Unauthorized);
    filterCb(resp);
    return;
  }

  std::string token = authHeader.substr(7);  // strip "Bearer "

  // Build JwtService using the same configuration loader as AuthController
  vote_backend::utils::JwtService jwt_svc = make_jwt_service();

  Json::Value claims = jwt_svc.verify_token(token);

  if (claims.isNull() || claims.empty()) {
    Json::Value err;
    err["error"] = "Invalid or expired token";
    auto resp = HttpResponse::newHttpJsonResponse(err);
    resp->setStatusCode(k401Unauthorized);
    filterCb(resp);
    return;
  }

  // Store user_id in request attributes for downstream handlers.
  // Handle missing or non-numeric sub claim gracefully.
  if (!claims.isMember("sub") || claims["sub"].isNull()) {
    // Token is valid but has no sub claim - still allow access
    // Downstream handlers can check for user_id attribute presence
    nextCb();
    return;
  }

  try {
    int64_t user_id = std::stoll(claims["sub"].asString());
    req->attributes()->insert("user_id", user_id);
  } catch (const std::exception&) {
    // Non-numeric sub claim - still allow access without user_id
    nextCb();
    return;
  }

  // Check if user is active
  if (!claims.isMember("is_active") || claims["is_active"].isNull() ||
      !claims["is_active"].asBool()) {
    Json::Value err;
    err["error"] = "User account is not active";
    auto resp = HttpResponse::newHttpJsonResponse(err);
    resp->setStatusCode(k423Locked);
    filterCb(resp);
    return;
  }

  // Continue processing.
  nextCb();
}
