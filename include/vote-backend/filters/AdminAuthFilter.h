/**
 *
 *  AdminAuthFilter.h
 *  Drogon filter that requires the authenticated user to be an admin.
 *
 *  It performs the same JWT authentication as JwtAuthFilter and additionally
 *  asserts that the access token's `is_admin` claim is true. Requests without a
 *  valid token are rejected with 401, while authenticated but non-admin users
 *  are rejected with 403.
 */

#pragma once

#include <drogon/HttpFilter.h>

class AdminAuthFilter : public drogon::HttpFilter<AdminAuthFilter>
{
  public:
    void doFilter(const drogon::HttpRequestPtr &req,
                  drogon::FilterCallback &&filterCb,
                  drogon::FilterChainCallback &&nextCb) override;
};
