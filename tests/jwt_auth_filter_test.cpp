#include <doctest/doctest.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <jwt-cpp/traits/open-source-parsers-jsoncpp/defaults.h>

#include <string>

#include "vote-backend/filters/JwtAuthFilter.hpp"
#include "vote-backend/utils/JwtService.hpp"

using namespace drogon;
using namespace vote_backend::utils;

// A well-known UUID used as a test user id throughout the tests.
static const std::string kTestUserId = "550e8400-e29b-41d4-a716-446655440000";

// Test fixture for JwtAuthFilter
class JwtAuthFilterFixture {
 public:
  JwtAuthFilterFixture()
      : service("test-secret-key-for-unit-tests-only-32chars", 15, 7) {}

  JwtService service;
};

TEST_SUITE("JwtAuthFilter Tests") {
  TEST_CASE_FIXTURE(JwtAuthFilterFixture, "Allows access to public paths") {
    JwtAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/login");

    bool nextCalled = false;
    bool filterCalled = false;

    filter.doFilter(
        req, [&](const HttpResponsePtr&) { filterCalled = true; },
        [&]() { nextCalled = true; });

    CHECK(nextCalled);
    CHECK(!filterCalled);
  }

  TEST_CASE_FIXTURE(JwtAuthFilterFixture,
                    "Rejects request without Authorization header") {
    JwtAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/protected");

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

  TEST_CASE_FIXTURE(JwtAuthFilterFixture,
                    "Rejects request with empty Authorization header") {
    JwtAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/protected");
    req->addHeader("Authorization", "");

    bool nextCalled = false;
    bool filterCalled = false;

    filter.doFilter(
        req, [&](const HttpResponsePtr&) { filterCalled = true; },
        [&]() { nextCalled = true; });

    CHECK(!nextCalled);
    CHECK(filterCalled);
  }

  TEST_CASE_FIXTURE(JwtAuthFilterFixture,
                    "Rejects request with malformed Authorization header") {
    JwtAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/protected");
    req->addHeader("Authorization", "InvalidFormat token");

    bool nextCalled = false;
    bool filterCalled = false;

    filter.doFilter(
        req, [&](const HttpResponsePtr&) { filterCalled = true; },
        [&]() { nextCalled = true; });

    CHECK(!nextCalled);
    CHECK(filterCalled);
  }

  TEST_CASE_FIXTURE(JwtAuthFilterFixture,
                    "Rejects request with invalid token") {
    JwtAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/protected");
    req->addHeader("Authorization", "Bearer invalid.token.here");

    bool nextCalled = false;
    bool filterCalled = false;

    filter.doFilter(
        req, [&](const HttpResponsePtr&) { filterCalled = true; },
        [&]() { nextCalled = true; });

    CHECK(!nextCalled);
    CHECK(filterCalled);
  }

  TEST_CASE_FIXTURE(JwtAuthFilterFixture,
                    "Rejects request with valid access token") {
    JwtAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/protected");

    std::string userId = kTestUserId;
    auto token = service.generate_access_token(userId, "testuser");
    req->addHeader("Authorization", "Bearer " + token);

    bool nextCalled = false;

    filter.doFilter(
        req, [&](const HttpResponsePtr&) {}, [&]() { nextCalled = true; });

    CHECK(nextCalled);
    auto attributes = req->attributes();
    CHECK(attributes->find("user_id"));
    CHECK(attributes->get<std::string>("user_id") == userId);
  }

  TEST_CASE_FIXTURE(JwtAuthFilterFixture,
                    "Rejects refresh token for protected route") {
    JwtAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/protected");

    auto token = service.generate_refresh_token(kTestUserId);
    req->addHeader("Authorization", "Bearer " + token);

    bool nextCalled = false;
    bool filterCalled = false;

    filter.doFilter(
        req, [&](const HttpResponsePtr&) { filterCalled = true; },
        [&]() { nextCalled = true; });

    CHECK(nextCalled);
    CHECK(!filterCalled);
  }

  TEST_CASE_FIXTURE(JwtAuthFilterFixture, "Rejects expired token") {
    JwtAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/protected");

    auto token =
        jwt::create()
            .set_issuer("vote-backend")
            .set_type("JWT")
            .set_subject(kTestUserId)
            .set_issued_at(std::chrono::system_clock::now() -
                           std::chrono::minutes(30))
            .set_expires_at(std::chrono::system_clock::now() -
                            std::chrono::minutes(15))
            .set_payload_claim("username", jwt::claim(std::string("testuser")))
            .set_payload_claim("type", jwt::claim(std::string("access")))
            .set_id("expired-jti")
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

  TEST_CASE_FIXTURE(JwtAuthFilterFixture, "Rejects token with wrong issuer") {
    JwtAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/protected");

    auto token =
        jwt::create()
            .set_issuer("wrong-issuer")
            .set_type("JWT")
            .set_subject(kTestUserId)
            .set_issued_at(std::chrono::system_clock::now())
            .set_expires_at(std::chrono::system_clock::now() +
                            std::chrono::minutes(15))
            .set_payload_claim("username", jwt::claim(std::string("testuser")))
            .set_payload_claim("type", jwt::claim(std::string("access")))
            .set_id("wrong-issuer-jti")
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

  TEST_CASE_FIXTURE(JwtAuthFilterFixture,
                    "Rejects token with wrong signature") {
    JwtAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/protected");

    auto token =
        jwt::create()
            .set_issuer("vote-backend")
            .set_type("JWT")
            .set_subject(kTestUserId)
            .set_issued_at(std::chrono::system_clock::now())
            .set_expires_at(std::chrono::system_clock::now() +
                            std::chrono::minutes(15))
            .set_payload_claim("username", jwt::claim(std::string("testuser")))
            .set_payload_claim("type", jwt::claim(std::string("access")))
            .set_id("wrong-sig-jti")
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

  TEST_CASE_FIXTURE(JwtAuthFilterFixture,
                    "Handles missing sub claim in token") {
    JwtAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/protected");

    auto token =
        jwt::create()
            .set_issuer("vote-backend")
            .set_type("JWT")
            .set_issued_at(std::chrono::system_clock::now())
            .set_expires_at(std::chrono::system_clock::now() +
                            std::chrono::minutes(15))
            .set_payload_claim("username", jwt::claim(std::string("testuser")))
            .set_payload_claim("type", jwt::claim(std::string("access")))
            .set_id("no-sub-jti")
            .sign(jwt::algorithm::hs256{
                "test-secret-key-for-unit-tests-only-32chars"});

    req->addHeader("Authorization", "Bearer " + token);

    bool nextCalled = false;
    bool filterCalled = false;

    filter.doFilter(
        req, [&](const HttpResponsePtr&) { filterCalled = true; },
        [&]() { nextCalled = true; });

    CHECK(nextCalled);
    CHECK(!filterCalled);
  }

  TEST_CASE_FIXTURE(JwtAuthFilterFixture,
                    "Accepts arbitrary string sub claim in token") {
    JwtAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/protected");

    // A subject claim that is a valid string but not a numeric id. The JWT
    // subject is now always a UUID string, so the filter should accept any
    // non-empty string subject.
    std::string custom_sub = "not-a-number";

    auto token =
        jwt::create()
            .set_issuer("vote-backend")
            .set_type("JWT")
            .set_subject(custom_sub)
            .set_issued_at(std::chrono::system_clock::now())
            .set_expires_at(std::chrono::system_clock::now() +
                            std::chrono::minutes(15))
            .set_payload_claim("username", jwt::claim(std::string("testuser")))
            .set_payload_claim("type", jwt::claim(std::string("access")))
            .set_payload_claim("is_admin", jwt::claim(false))
            .set_payload_claim("is_active", jwt::claim(true))
            .set_id("non-numeric-sub-jti")
            .sign(jwt::algorithm::hs256{
                "test-secret-key-for-unit-tests-only-32chars"});

    req->addHeader("Authorization", "Bearer " + token);

    bool nextCalled = false;
    bool filterCalled = false;

    filter.doFilter(
        req, [&](const HttpResponsePtr&) { filterCalled = true; },
        [&]() { nextCalled = true; });

    CHECK(nextCalled);
    CHECK(!filterCalled);
    // The string subject should be stored as user_id in attributes.
    CHECK(req->attributes()->get<std::string>("user_id") == custom_sub);
  }

  TEST_CASE_FIXTURE(JwtAuthFilterFixture, "Allows access to register path") {
    JwtAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/register");

    bool nextCalled = false;
    bool filterCalled = false;

    filter.doFilter(
        req, [&](const HttpResponsePtr&) { filterCalled = true; },
        [&]() { nextCalled = true; });

    CHECK(nextCalled);
    CHECK(!filterCalled);
  }

  TEST_CASE_FIXTURE(JwtAuthFilterFixture, "Allows access to refresh path") {
    JwtAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/refresh");

    bool nextCalled = false;
    bool filterCalled = false;

    filter.doFilter(
        req, [&](const HttpResponsePtr&) { filterCalled = true; },
        [&]() { nextCalled = true; });

    CHECK(nextCalled);
    CHECK(!filterCalled);
  }

  TEST_CASE_FIXTURE(JwtAuthFilterFixture,
                    "Rejects request to unknown path without token") {
    JwtAuthFilter filter;
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/unknown");

    bool nextCalled = false;
    bool filterCalled = false;

    filter.doFilter(
        req, [&](const HttpResponsePtr&) { filterCalled = true; },
        [&]() { nextCalled = true; });

    CHECK(!nextCalled);
    CHECK(filterCalled);
  }

}  // TEST_SUITE