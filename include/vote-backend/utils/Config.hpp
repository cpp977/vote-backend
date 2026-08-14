#include <string>

struct CORSConfig {
  std::string allowed_origin{};
};

CORSConfig load_cors_config();
