/**
 *
 *  AdminAuthFilter.cpp
 *  Implementation of the admin-authorization filter.
 */

#include "vote-backend/filters/AdminAuthFilter.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <json/json.h>

#include <string>

#include "vote-backend/utils/JwtService.hpp"

using namespace drogon;
using namespace vote_backend::utils;

void AdminAuthFilter::doFilter(const HttpRequestPtr& req,
                               FilterCallback&& filterCb,
                               FilterChainCallback&& nextCb) {
  // 1. Require a Bearer token.
  auto authHeader = req->getHeader("Authorization");
  if (authHeader.empty() || !authHeader.starts_with("Bearer ")) {
    Json::Value err;
    err["error"] = "Missing or malformed Authorization header";
    auto resp = HttpResponse::newHttpJsonResponse(err);
    resp->setStatusCode(k401Unauthorized);
    filterCb(resp);
    return;
  }

  std::string token = authHeader.substr(7);  // strip "Bearer "

  // 2. Verify the token signature and expiry.
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

  // 3. Assert that the token belongs to an admin user.
  bool is_admin = false;
  if (claims.isMember("is_admin") && !claims["is_admin"].isNull()) {
    is_admin = claims["is_admin"].asBool();
  }
  if (!is_admin) {
    Json::Value err;
    err["error"] = "Admin privileges required";
    auto resp = HttpResponse::newHttpJsonResponse(err);
    resp->setStatusCode(k403Forbidden);
    filterCb(resp);
    return;
  }

  // 4. Expose identity to downstream handlers (mirrors JwtAuthFilter).
  if (claims.isMember("sub") && !claims["sub"].isNull()) {
    try {
      int64_t user_id = std::stoll(claims["sub"].asString());
      req->attributes()->insert("user_id", user_id);
    } catch (const std::exception&) {
      // Non-numeric sub claim - still allow access without user_id attribute.
    }
  }
  req->attributes()->insert("is_admin", true);

  // Continue processing.
  nextCb();
}
