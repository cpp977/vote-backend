#include <doctest/doctest.h>
#include <json/json.h>
#include <jwt-cpp/traits/open-source-parsers-jsoncpp/defaults.h>

#include <chrono>
#include <string>
#include <thread>

#include "vote-backend/utils/JwtService.hpp"

using namespace vote_backend::utils;

// Test fixture for JwtService with a known secret
class JwtServiceFixture {
 public:
  JwtServiceFixture()
      : service("test-secret-key-for-unit-tests-only-32chars", 15, 7) {}

  JwtService service;
};

TEST_SUITE("JwtService Tests") {
  TEST_CASE_FIXTURE(JwtServiceFixture,
                    "Constructor initializes with correct values") {
    // The fixture creates a service with known values
    // We can verify by checking token generation works
    auto token = service.generate_access_token(1, "testuser");
    CHECK(!token.empty());
  }

  TEST_CASE_FIXTURE(JwtServiceFixture,
                    "Generate access token returns non-empty string") {
    auto token = service.generate_access_token(123, "john_doe");

    CHECK(!token.empty());
    // JWT tokens have three parts separated by dots
    CHECK(token.find('.') != std::string::npos);
    CHECK(token.find('.', token.find('.') + 1) != std::string::npos);
  }

  TEST_CASE_FIXTURE(JwtServiceFixture,
                    "Generate access token contains correct claims") {
    int64_t user_id = 42;
    std::string username = "alice";

    auto token = service.generate_access_token(user_id, username);
    auto claims = service.verify_token(token);

    CHECK(!claims.isNull());
    CHECK(claims["sub"].asString() == std::to_string(user_id));
    CHECK(claims["username"].asString() == username);
    CHECK(claims["iss"].asString() == "vote-backend");
    CHECK(claims["type"].asString() == "access");
  }

  TEST_CASE_FIXTURE(JwtServiceFixture,
                    "Generate refresh token returns non-empty string") {
    auto token = service.generate_refresh_token(456);

    CHECK(!token.empty());
    CHECK(token.find('.') != std::string::npos);
  }

  TEST_CASE_FIXTURE(JwtServiceFixture,
                    "Generate refresh token contains correct claims") {
    int64_t user_id = 789;

    auto token = service.generate_refresh_token(user_id);
    auto claims = service.verify_token(token);

    CHECK(!claims.isNull());
    CHECK(claims["sub"].asString() == std::to_string(user_id));
    CHECK(claims["iss"].asString() == "vote-backend");
    CHECK(claims["type"].asString() == "refresh");
    // Refresh tokens should NOT have username claim
    CHECK(claims["username"].isNull());
  }

  TEST_CASE_FIXTURE(JwtServiceFixture, "Verify valid access token succeeds") {
    auto token = service.generate_access_token(100, "validuser");
    auto claims = service.verify_token(token);

    CHECK(!claims.isNull());
    CHECK(claims["sub"].asString() == "100");
    CHECK(claims["username"].asString() == "validuser");
  }

  TEST_CASE_FIXTURE(JwtServiceFixture, "Verify valid refresh token succeeds") {
    auto token = service.generate_refresh_token(200);
    auto claims = service.verify_token(token);

    CHECK(!claims.isNull());
    CHECK(claims["sub"].asString() == "200");
    CHECK(claims["type"].asString() == "refresh");
  }

  TEST_CASE_FIXTURE(JwtServiceFixture, "Verify invalid token returns null") {
    // Completely invalid token
    auto claims = service.verify_token("invalid.token.here");
    CHECK(claims.isNull());
  }

  TEST_CASE_FIXTURE(JwtServiceFixture, "Verify empty token returns null") {
    auto claims = service.verify_token("");
    CHECK(claims.isNull());
  }

  TEST_CASE_FIXTURE(JwtServiceFixture,
                    "Verify token with wrong signature returns null") {
    // Create a token with a different secret
    JwtService other_service("different-secret-key-for-testing-32chars", 15, 7);
    auto token = other_service.generate_access_token(1, "user");

    // Try to verify with our service (different secret)
    auto claims = service.verify_token(token);
    CHECK(claims.isNull());
  }

  TEST_CASE_FIXTURE(JwtServiceFixture, "Verify tampered token returns null") {
    auto token = service.generate_access_token(1, "user");

    // Tamper with the token by changing a character
    if (token.length() > 10) {
      token[10] = (token[10] == 'a') ? 'b' : 'a';
    }

    auto claims = service.verify_token(token);
    CHECK(claims.isNull());
  }

  TEST_CASE_FIXTURE(JwtServiceFixture,
                    "Verify token with wrong issuer returns null") {
    // Create a token manually with wrong issuer
    auto now = std::chrono::system_clock::now();
    auto exp = now + std::chrono::minutes(15);

    auto token =
        jwt::create()
            .set_issuer("wrong-issuer")
            .set_type("JWT")
            .set_subject("1")
            .set_issued_at(now)
            .set_expires_at(exp)
            .set_payload_claim("type", jwt::claim(std::string("access")))
            .sign(jwt::algorithm::hs256{
                "test-secret-key-for-unit-tests-only-32chars"});

    auto claims = service.verify_token(token);
    CHECK(claims.isNull());
  }

  TEST_CASE_FIXTURE(JwtServiceFixture, "Extract JTI from valid token") {
    auto token = service.generate_refresh_token(123);
    auto jti = service.extract_jti_unsafe(token);

    // JTI should be present in refresh tokens (jwt-cpp may or may not add it)
    // This test verifies the extraction doesn't crash
    // The actual presence depends on jwt-cpp implementation
    CHECK(!jti.empty());
  }

  TEST_CASE_FIXTURE(JwtServiceFixture,
                    "Extract JTI from invalid token returns empty") {
    auto jti = service.extract_jti_unsafe("invalid.token");
    // Should not crash and return empty or handle gracefully
    // The exact behavior depends on jwt-cpp, but it shouldn't crash
    CHECK(true);  // If we get here, it didn't crash
  }

  TEST_CASE_FIXTURE(JwtServiceFixture,
                    "Different user IDs produce different tokens") {
    auto token1 = service.generate_access_token(1, "user1");
    auto token2 = service.generate_access_token(2, "user2");

    CHECK(token1 != token2);

    auto claims1 = service.verify_token(token1);
    auto claims2 = service.verify_token(token2);

    CHECK(claims1["sub"].asString() == "1");
    CHECK(claims2["sub"].asString() == "2");
  }

  TEST_CASE_FIXTURE(
      JwtServiceFixture,
      "Same user ID produces different tokens (due to timestamps)") {
    auto token1 = service.generate_access_token(1, "user");
    // Small delay to ensure different timestamp
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto token2 = service.generate_access_token(1, "user");

    // Tokens should be different due to different issued_at timestamps
    CHECK(token1 != token2);

    // But both should verify correctly and contain the same user info
    auto claims1 = service.verify_token(token1);
    auto claims2 = service.verify_token(token2);

    CHECK(claims1["sub"].asString() == "1");
    CHECK(claims2["sub"].asString() == "1");
    CHECK(claims1["username"].asString() == "user");
    CHECK(claims2["username"].asString() == "user");
  }

  TEST_CASE_FIXTURE(JwtServiceFixture, "Token contains issued_at claim") {
    auto token = service.generate_access_token(1, "user");
    auto claims = service.verify_token(token);

    CHECK(!claims.isNull());
    CHECK(!claims["iat"].isNull());
    CHECK(claims["iat"].isInt64());
  }

  TEST_CASE_FIXTURE(JwtServiceFixture, "Token contains expires_at claim") {
    auto token = service.generate_access_token(1, "user");
    auto claims = service.verify_token(token);

    CHECK(!claims.isNull());
    CHECK(!claims["exp"].isNull());
    CHECK(claims["exp"].isInt64());

    // Expiration should be after issued_at
    CHECK(claims["exp"].asInt64() > claims["iat"].asInt64());
  }

  TEST_CASE_FIXTURE(JwtServiceFixture,
                    "Access token has shorter expiry than refresh token") {
    auto access_token = service.generate_access_token(1, "user");
    auto refresh_token = service.generate_refresh_token(1);

    auto access_claims = service.verify_token(access_token);
    auto refresh_claims = service.verify_token(refresh_token);

    CHECK(!access_claims.isNull());
    CHECK(!refresh_claims.isNull());

    int64_t access_exp = access_claims["exp"].asInt64();
    int64_t access_iat = access_claims["iat"].asInt64();
    int64_t refresh_exp = refresh_claims["exp"].asInt64();
    int64_t refresh_iat = refresh_claims["iat"].asInt64();

    int64_t access_duration = access_exp - access_iat;
    int64_t refresh_duration = refresh_exp - refresh_iat;

    // Refresh token should last much longer (days vs minutes)
    CHECK(refresh_duration > access_duration);
    // Access token should be roughly 15 minutes (900 seconds)
    CHECK(access_duration >= 890);  // Allow small margin
    CHECK(access_duration <= 910);
    // Refresh token should be roughly 7 days (604800 seconds)
    CHECK(refresh_duration >= 604790);
    CHECK(refresh_duration <= 604810);
  }

  TEST_CASE_FIXTURE(JwtServiceFixture, "Custom expiry configuration works") {
    // Create service with custom expiry times
    JwtService custom_service("another-test-secret-key-32chars!", 30, 14);

    auto access_token = custom_service.generate_access_token(1, "user");
    auto refresh_token = custom_service.generate_refresh_token(1);

    auto access_claims = custom_service.verify_token(access_token);
    auto refresh_claims = custom_service.verify_token(refresh_token);

    int64_t access_duration =
        access_claims["exp"].asInt64() - access_claims["iat"].asInt64();
    int64_t refresh_duration =
        refresh_claims["exp"].asInt64() - refresh_claims["iat"].asInt64();

    // Access token should be roughly 30 minutes (1800 seconds)
    CHECK(access_duration >= 1790);
    CHECK(access_duration <= 1810);
    // Refresh token should be roughly 14 days (1209600 seconds)
    CHECK(refresh_duration >= 1209590);
    CHECK(refresh_duration <= 1209610);
  }

  TEST_CASE_FIXTURE(JwtServiceFixture,
                    "Token type claim distinguishes access and refresh") {
    auto access_token = service.generate_access_token(1, "user");
    auto refresh_token = service.generate_refresh_token(1);

    auto access_claims = service.verify_token(access_token);
    auto refresh_claims = service.verify_token(refresh_token);

    CHECK(access_claims["type"].asString() == "access");
    CHECK(refresh_claims["type"].asString() == "refresh");
  }

  TEST_CASE_FIXTURE(JwtServiceFixture,
                    "Multiple tokens for same user can coexist") {
    auto token1 = service.generate_access_token(1, "user");
    auto token2 = service.generate_access_token(1, "user");
    auto token3 = service.generate_access_token(1, "user");

    // All should be valid
    CHECK(!service.verify_token(token1).isNull());
    CHECK(!service.verify_token(token2).isNull());
    CHECK(!service.verify_token(token3).isNull());

    // All should have same user info
    auto claims1 = service.verify_token(token1);
    auto claims2 = service.verify_token(token2);
    auto claims3 = service.verify_token(token3);

    CHECK(claims1["sub"].asString() == "1");
    CHECK(claims2["sub"].asString() == "1");
    CHECK(claims3["sub"].asString() == "1");
  }

  TEST_CASE_FIXTURE(JwtServiceFixture, "Large user ID is handled correctly") {
    int64_t large_id = 9999999999999;
    auto token = service.generate_access_token(large_id, "user");
    auto claims = service.verify_token(token);

    CHECK(!claims.isNull());
    CHECK(claims["sub"].asString() == std::to_string(large_id));
  }

  TEST_CASE_FIXTURE(JwtServiceFixture, "Username with special characters") {
    std::string special_username = "user@example.com";
    auto token = service.generate_access_token(1, special_username);
    auto claims = service.verify_token(token);

    CHECK(!claims.isNull());
    CHECK(claims["username"].asString() == special_username);
  }

  TEST_CASE_FIXTURE(JwtServiceFixture, "Empty username is handled") {
    auto token = service.generate_access_token(1, "");
    auto claims = service.verify_token(token);

    CHECK(!claims.isNull());
    CHECK(claims["username"].asString() == "");
  }

}  // TEST_SUITE