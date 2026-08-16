#include <cstdint>
#include <string>

struct CORSConfig {
  std::string allowed_origin{};
};

CORSConfig load_cors_config();

struct HMACConfig {
  int16_t hmac_key_version{};
};

HMACConfig load_hmac_config();
