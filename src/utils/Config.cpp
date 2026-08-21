#include "vote-backend/utils/Config.hpp"

#include <cstdint>
#include <fstream>

#include "fmt/format.h"
#include "json/json.h"

AgeBucketConfig load_age_bucket_config() {
  AgeBucketConfig cfg;
  const char* conf_path_c = std::getenv("VOTE_BACKEND_CONFPATH");
  std::string config_path =
      conf_path_c ? std::string(conf_path_c) : "/etc/vote";

  Json::Value json_cfg;
  std::ifstream ifs(fmt::format("{}/config.json", config_path));
  if (ifs.is_open()) {
    ifs >> json_cfg;
    ifs.close();
  } else {
    // Fallback default
    json_cfg["age_bucket_size"] = 5;
  }

  cfg.bucket_size = 5;
  if (json_cfg.isMember("age_bucket_size")) {
    cfg.bucket_size = json_cfg["age_bucket_size"].asInt();
  }
  return cfg;
}

CORSConfig load_cors_config() {
  CORSConfig cfg;
  const char* conf_path_c = std::getenv("VOTE_BACKEND_CONFPATH");
  std::string config_path =
      conf_path_c ? std::string(conf_path_c) : "/etc/vote";

  Json::Value json_cfg;
  std::ifstream ifs(fmt::format("{}/config.json", config_path));
  if (ifs.is_open()) {
    ifs >> json_cfg;
    ifs.close();
  } else {
    // Fallback defaults
    json_cfg["allowed_origin"] = "";
  }

  cfg.allowed_origin = json_cfg["allowed_origin"].asString();
  return cfg;
}

HMACConfig load_hmac_config() {
  HMACConfig cfg;
  const char* conf_path_c = std::getenv("VOTE_BACKEND_CONFPATH");
  std::string config_path =
      conf_path_c ? std::string(conf_path_c) : "/etc/vote";

  Json::Value json_cfg;
  std::ifstream ifs(fmt::format("{}/config.json", config_path));
  if (ifs.is_open()) {
    ifs >> json_cfg;
    ifs.close();
  } else {
    // Fallback defaults
    json_cfg["hmac_key_version"] = 1;
  }

  cfg.hmac_key_version =
      static_cast<int16_t>(json_cfg["hmac_key_version"].asInt());
  return cfg;
}
