/**
 *
 *  UserIdHash.cpp
 *  Privacy-preserving hashing of user ids for anonymous answer tracking.
 */

#include "vote-backend/utils/UserIdHash.hpp"

#include <fmt/format.h>
#include <json/json.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <trantor/utils/Logger.h>

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace vote_backend::utils {

namespace {

/// Resolves the server-side secret used to key the user-id hash.
std::string load_secret() {
  char* conf_path_c = std::getenv("VOTE_BACKEND_CONFPATH");
  std::string config_path = "/etc/vote";
  if (conf_path_c != nullptr) {
    config_path = conf_path_c;
  }

  Json::Value cfg;
  std::ifstream ifs(fmt::format("{}/config.json", config_path));
  if (ifs.is_open()) {
    ifs >> cfg;
    ifs.close();
  }

  if (cfg.isMember("user_id_hash_salt") &&
      !cfg["user_id_hash_salt"].asString().empty()) {
    return cfg["user_id_hash_salt"].asString();
  }
  if (cfg.isMember("jwt_secret") && !cfg["jwt_secret"].asString().empty()) {
    return cfg["jwt_secret"].asString();
  }

  LOG_WARN << "[UserIdHasher] No user_id_hash_salt/jwt_secret configured; "
              "using built-in default secret";
  return "change-me-to-a-strong-random-secret-key-min-32-chars";
}

}  // namespace

UserIdHasher::UserIdHasher(std::string key) : key_(std::move(key)) {}

std::string UserIdHasher::hash(int64_t user_id) const {
  const std::string message = std::to_string(user_id);

  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digest_len = 0;
  const unsigned char* result =
      HMAC(EVP_sha256(), key_.data(), static_cast<int>(key_.size()),
           reinterpret_cast<const unsigned char*>(message.data()),
           static_cast<int>(message.size()), digest, &digest_len);
  if (result == nullptr) {
    throw std::runtime_error("UserIdHasher: HMAC-SHA256 computation failed");
  }

  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (unsigned int i = 0; i < digest_len; ++i) {
    oss << std::setw(2) << static_cast<int>(digest[i]);
  }
  return oss.str();
}

const UserIdHasher& user_id_hasher() {
  static const UserIdHasher instance(load_secret());
  return instance;
}

}  // namespace vote_backend::utils
