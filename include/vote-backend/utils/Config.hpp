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

/// Configuration for normalizing `birth_year` tags into anonymized age-range
/// buckets (e.g. "25-29") inside the answer-submission endpoint.
struct AgeBucketConfig {
  /// Width of each bucket in years (default 5).
  int bucket_size{};
};

AgeBucketConfig load_age_bucket_config();
