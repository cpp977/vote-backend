#include <doctest/doctest.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <jwt-cpp/traits/open-source-parsers-jsoncpp/defaults.h>

#include <chrono>
#include <string>

#include "vote-backend/filters/AdminAuthFilter.hpp"
#include "vote-backend/utils/JwtService.hpp"

using namespace drogon;
using namespace vote_backend::utils;

// Test fixture for AdminAuthFilter
class AdminAuthFilterFixture {
 public:
  AdminAuthFilterFixture()
      : service("test-secret-key-for-unit-tests-only-32chars", 15, 7) {}

  JwtService service;
};

TEST_SUITE("AdminAuthFilter Tests") {
  TEST_CASE_FIXTURE(AdminAuthFilterFixture,
                    "Rejects request without Authorization header") {
    AdminAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/admin/things");

    bool nextCalled = false;
    bool filterCalled = false;
    HttpResponsePtr resp;

    filter.doFilter(
        req,
        [&](const HttpResponsePtr& r) {
          filterCalled = true;
          resp = r;
        },
        [&]() { nextCalled = true; });

    CHECK(!nextCalled);
    CHECK(filterCalled);
    CHECK(resp->statusCode() == k401Unauthorized);
  }

  TEST_CASE_FIXTURE(AdminAuthFilterFixture,
                    "Rejects request with empty Authorization header") {
    AdminAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/admin/things");
    req->addHeader("Authorization", "");

    bool nextCalled = false;
    bool filterCalled = false;

    filter.doFilter(
        req, [&](const HttpResponsePtr&) { filterCalled = true; },
        [&]() { nextCalled = true; });

    CHECK(!nextCalled);
    CHECK(filterCalled);
  }

  TEST_CASE_FIXTURE(AdminAuthFilterFixture,
                    "Rejects request with malformed Authorization header") {
    AdminAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/admin/things");
    req->addHeader("Authorization", "InvalidFormat token");

    bool nextCalled = false;
    bool filterCalled = false;

    filter.doFilter(
        req, [&](const HttpResponsePtr&) { filterCalled = true; },
        [&]() { nextCalled = true; });

    CHECK(!nextCalled);
    CHECK(filterCalled);
  }

  TEST_CASE_FIXTURE(AdminAuthFilterFixture,
                    "Rejects request with invalid token") {
    AdminAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/admin/things");
    req->addHeader("Authorization", "Bearer invalid.token.here");

    bool nextCalled = false;
    bool filterCalled = false;

    filter.doFilter(
        req, [&](const HttpResponsePtr&) { filterCalled = true; },
        [&]() { nextCalled = true; });

    CHECK(!nextCalled);
    CHECK(filterCalled);
  }

  TEST_CASE_FIXTURE(AdminAuthFilterFixture, "Rejects non-admin user with 403") {
    AdminAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/admin/things");

    // A valid token, but issued for a regular (non-admin) user.
    auto token = service.generate_access_token(123, "regularuser", false);
    req->addHeader("Authorization", "Bearer " + token);

    bool nextCalled = false;
    bool filterCalled = false;
    HttpResponsePtr resp;

    filter.doFilter(
        req,
        [&](const HttpResponsePtr& r) {
          filterCalled = true;
          resp = r;
        },
        [&]() { nextCalled = true; });

    CHECK(!nextCalled);
    CHECK(filterCalled);
    CHECK(resp->statusCode() == k403Forbidden);
  }

  TEST_CASE_FIXTURE(AdminAuthFilterFixture,
                    "Rejects token without is_admin claim with 403") {
    AdminAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/admin/things");

    // Build a perfectly valid token that simply lacks the is_admin claim.
    auto now = std::chrono::system_clock::now();
    auto token =
        jwt::create()
            .set_issuer("vote-backend")
            .set_type("JWT")
            .set_subject("1")
            .set_issued_at(now)
            .set_expires_at(now + std::chrono::minutes(15))
            .set_payload_claim("username", jwt::claim(std::string("admin")))
            .set_payload_claim("type", jwt::claim(std::string("access")))
            .set_id("no-is-admin-jti")
            .sign(jwt::algorithm::hs256{
                "test-secret-key-for-unit-tests-only-32chars"});
    req->addHeader("Authorization", "Bearer " + token);

    bool nextCalled = false;
    bool filterCalled = false;
    HttpResponsePtr resp;

    filter.doFilter(
        req,
        [&](const HttpResponsePtr& r) {
          filterCalled = true;
          resp = r;
        },
        [&]() { nextCalled = true; });

    CHECK(!nextCalled);
    CHECK(filterCalled);
    CHECK(resp->statusCode() == k403Forbidden);
  }

  TEST_CASE_FIXTURE(AdminAuthFilterFixture, "Allows admin user through") {
    AdminAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/admin/things");

    int64_t userId = 123;
    auto token = service.generate_access_token(userId, "adminuser", true);
    req->addHeader("Authorization", "Bearer " + token);

    bool nextCalled = false;
    bool filterCalled = false;

    filter.doFilter(
        req, [&](const HttpResponsePtr&) {}, [&]() { nextCalled = true; });

    CHECK(nextCalled);
    CHECK(!filterCalled);
  }

  TEST_CASE_FIXTURE(AdminAuthFilterFixture,
                    "Sets user_id and is_admin attributes for admin") {
    AdminAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/admin/things");

    int64_t userId = 123;
    auto token = service.generate_access_token(userId, "adminuser", true);
    req->addHeader("Authorization", "Bearer " + token);

    bool nextCalled = false;

    filter.doFilter(
        req, [&](const HttpResponsePtr&) {}, [&]() { nextCalled = true; });

    CHECK(nextCalled);
    auto attributes = req->attributes();
    CHECK(attributes->find("user_id"));
    CHECK(attributes->get<int64_t>("user_id") == userId);
    CHECK(attributes->find("is_admin"));
    CHECK(attributes->get<bool>("is_admin") == true);
  }

  TEST_CASE_FIXTURE(AdminAuthFilterFixture, "Rejects expired token") {
    AdminAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/admin/things");

    auto token =
        jwt::create()
            .set_issuer("vote-backend")
            .set_type("JWT")
            .set_subject("1")
            .set_issued_at(std::chrono::system_clock::now() -
                           std::chrono::minutes(30))
            .set_expires_at(std::chrono::system_clock::now() -
                            std::chrono::minutes(15))
            .set_payload_claim("username", jwt::claim(std::string("admin")))
            .set_payload_claim("type", jwt::claim(std::string("access")))
            .set_payload_claim("is_admin", jwt::claim(Json::Value(true)))
            .set_id("expired-admin-jti")
            .sign(jwt::algorithm::hs256{
                "test-secret-key-for-unit-tests-only-32chars"});

    req->addHeader("Authorization", "Bearer " + token);

    bool nextCalled = false;
    bool filterCalled = false;

    filter.doFilter(
        req, [&](const HttpResponsePtr&) { filterCalled = true; },
        [&]() { nextCalled = true; });

    CHECK(!nextCalled);
    CHECK(filterCalled);
  }

  TEST_CASE_FIXTURE(AdminAuthFilterFixture, "Rejects token with wrong issuer") {
    AdminAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/admin/things");

    auto token =
        jwt::create()
            .set_issuer("wrong-issuer")
            .set_type("JWT")
            .set_subject("1")
            .set_issued_at(std::chrono::system_clock::now())
            .set_expires_at(std::chrono::system_clock::now() +
                            std::chrono::minutes(15))
            .set_payload_claim("username", jwt::claim(std::string("admin")))
            .set_payload_claim("type", jwt::claim(std::string("access")))
            .set_payload_claim("is_admin", jwt::claim(Json::Value(true)))
            .set_id("wrong-issuer-admin-jti")
            .sign(jwt::algorithm::hs256{
                "test-secret-key-for-unit-tests-only-32chars"});

    req->addHeader("Authorization", "Bearer " + token);

    bool nextCalled = false;
    bool filterCalled = false;

    filter.doFilter(
        req, [&](const HttpResponsePtr&) { filterCalled = true; },
        [&]() { nextCalled = true; });

    CHECK(!nextCalled);
    CHECK(filterCalled);
  }

  TEST_CASE_FIXTURE(AdminAuthFilterFixture,
                    "Rejects token with wrong signature") {
    AdminAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/admin/things");

    auto token =
        jwt::create()
            .set_issuer("vote-backend")
            .set_type("JWT")
            .set_subject("1")
            .set_issued_at(std::chrono::system_clock::now())
            .set_expires_at(std::chrono::system_clock::now() +
                            std::chrono::minutes(15))
            .set_payload_claim("username", jwt::claim(std::string("admin")))
            .set_payload_claim("type", jwt::claim(std::string("access")))
            .set_payload_claim("is_admin", jwt::claim(Json::Value(true)))
            .set_id("wrong-sig-admin-jti")
            .sign(jwt::algorithm::hs256{
                "wrong-secret-key-for-unit-tests-only-32chars"});

    req->addHeader("Authorization", "Bearer " + token);

    bool nextCalled = false;
    bool filterCalled = false;

    filter.doFilter(
        req, [&](const HttpResponsePtr&) { filterCalled = true; },
        [&]() { nextCalled = true; });

    CHECK(!nextCalled);
    CHECK(filterCalled);
  }

  TEST_CASE_FIXTURE(AdminAuthFilterFixture,
                    "Generated admin token carries is_admin claim") {
    int64_t user_id = 42;
    auto token = service.generate_access_token(user_id, "admin", true);
    auto claims = service.verify_token(token);

    CHECK(!claims.isNull());
    CHECK(claims["is_admin"].asBool() == true);
    CHECK(claims["sub"].asString() == std::to_string(user_id));
  }

}  // TEST_SUITE
