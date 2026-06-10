/**
 *
 *  JwtAuthFilter.h
 *  Drogon filter that validates JWT access tokens for protected routes.
 */

#pragma once

#include <drogon/HttpFilter.h>

class JwtAuthFilter : public drogon::HttpFilter<JwtAuthFilter>
{
public:
    void doFilter(const drogon::HttpRequestPtr &req,
                  drogon::FilterCallback &&filterCb,
                  drogon::FilterChainCallback &&nextCb) override;
};
