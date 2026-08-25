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
/// buckets (e.g. "25-34") inside the answer-submission endpoint.
struct AgeBucketConfig {
  /// Width of each bucket in years (default 10).
  int bucket_size{};
};

AgeBucketConfig load_age_bucket_config();

/// Configuration for the stats endpoint: aggregated results are only returned
/// when at least `min_answers` matching answers exist (privacy threshold).
struct StatsConfig {
  /// Minimum number of matching answers required to return statistics
  /// (default 5).
  int min_answers{};
};

StatsConfig load_stats_config();
