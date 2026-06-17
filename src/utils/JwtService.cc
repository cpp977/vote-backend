/**
 *
 *  JwtService.cc
 *  JWT token generation and verification implementation.
 *  Uses jwt-cpp (header-only) with OpenSSL for HS256 signing.
 */

#include "vote-backend/utils/JwtService.h"

#include <jwt-cpp/traits/open-source-parsers-jsoncpp/defaults.h>
#include <trantor/utils/Logger.h>
#include <fmt/core.h>

#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace vote_backend::utils;

namespace vote_backend::utils {

JwtService make_jwt_service() {
  // Load JWT configuration directly from the project's config.json file.
  // This avoids reliance on drogon's custom config, which does not contain the
  // top‑level JWT entries.
  char* conf_path_c = nullptr;
  conf_path_c = std::getenv("VOTE_BACKEND_CONFPATH");
  std::string config_path = "/etc/vote";
  if (conf_path_c != nullptr) {
    config_path = conf_path_c;
  }

  LOG_INFO << "[make_jwt_service] VOTE_BACKEND_CONFPATH=" << (conf_path_c ? conf_path_c : "(null)");
  LOG_INFO << "[make_jwt_service] config_path=" << config_path;

  Json::Value cfg;
  std::ifstream ifs(fmt::format("{}/config.json", config_path));
  if (ifs.is_open()) {
    ifs >> cfg;
    ifs.close();
    LOG_INFO << "[make_jwt_service] Successfully loaded config.json";
    LOG_INFO << "[make_jwt_service] jwt_secret length=" << cfg["jwt_secret"].asString().length();
  } else {
    // Fallback: use default values if the config cannot be read.
    LOG_ERROR << "[make_jwt_service] Failed to open config.json at " << config_path << ", using defaults";
    cfg["jwt_secret"] = "change-me-to-a-strong-random-secret-key-min-32-chars";
    cfg["jwt_access_token_expiry_minutes"] = 15;
    cfg["jwt_refresh_token_expiry_days"] = 7;
  }
  return JwtService(
      cfg["jwt_secret"].asString(),
      cfg["jwt_access_token_expiry_minutes"].asInt(),
      cfg["jwt_refresh_token_expiry_days"].asInt());
}

} // namespace vote_backend::utils

JwtService::JwtService(const std::string& secret, int access_expiry_minutes,
                       int refresh_expiry_days)
    : secret_(secret),
      access_expiry_minutes_(access_expiry_minutes),
      refresh_expiry_days_(refresh_expiry_days) {}

std::string JwtService::generate_access_token(
    int64_t user_id, const std::string& username) const {
  auto now = std::chrono::system_clock::now();
  auto exp = now + std::chrono::minutes(access_expiry_minutes_);
  LOG_INFO << "[JwtService] generate_access_token: access_expiry_minutes_="
            << access_expiry_minutes_
            << " now=" << std::chrono::system_clock::to_time_t(now)
            << " exp=" << std::chrono::system_clock::to_time_t(exp);
  auto token = jwt::create()
                   .set_issuer("vote-backend")
                   .set_type("JWT")
                   .set_subject(std::to_string(user_id))
                   .set_issued_at(now)
                   .set_expires_at(exp)
                   .set_payload_claim("username", jwt::claim(username))
                   .set_payload_claim("type", jwt::claim(std::string("access")))
                   .sign(jwt::algorithm::hs256{secret_});

  return token;
}

std::string JwtService::generate_refresh_token(int64_t user_id) const {
  auto now = std::chrono::system_clock::now();
  auto exp = now + std::chrono::hours(24 * refresh_expiry_days_);
  LOG_INFO << "[JwtService] generate_refresh_token: refresh_expiry_days_="
            << refresh_expiry_days_
            << " now=" << std::chrono::system_clock::to_time_t(now)
            << " exp=" << std::chrono::system_clock::to_time_t(exp);
  auto token =
      jwt::create()
          .set_issuer("vote-backend")
          .set_type("JWT")
          .set_subject(std::to_string(user_id))
          .set_issued_at(now)
          .set_expires_at(exp)
          .set_payload_claim("type", jwt::claim(std::string("refresh")))
          .sign(jwt::algorithm::hs256{secret_});

  return token;
}

Json::Value JwtService::verify_token(const std::string& token) const {
  try {
    auto decoded = jwt::decode(token);
    auto verifier = jwt::verify()
                        .allow_algorithm(jwt::algorithm::hs256{secret_})
                        .with_issuer("vote-backend");

    verifier.verify(decoded);

    // Convert claims to Json::Value
    // With jsoncpp traits, get_payload_json() returns a Json::Value object,
    // and iterating over it yields member names (strings).
    Json::Value claims;
    auto payload = decoded.get_payload_json();
    for (const auto& member : payload.getMemberNames()) {
      claims[member] = payload[member];
    }
    return claims;
  } catch (const std::exception&) {
    // Return null on any verification error.
    return Json::Value();
  }
}
