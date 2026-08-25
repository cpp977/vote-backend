/**
 *  BirthYearBucketer.cpp
 *  Anonymizes a submitted birth year by normalizing it into a
 *  configurable age-range bucket before the value is persisted.
 */

#include "vote-backend/utils/BirthYearBucketer.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <vector>

namespace vote_backend::utils {

int currentYear() {
  using namespace std::chrono;
  auto now = system_clock::now();
  std::time_t now_time = system_clock::to_time_t(now);
  std::tm tm_buf;
  localtime_r(&now_time, &tm_buf);
  return tm_buf.tm_year + 1900;
}

std::string bucketBirthYear(int birth_year, int bucket_size, int current_year) {
  if (bucket_size <= 0) {
    return "";
  }
  if (birth_year <= 1900 || birth_year > current_year) {
    return "";
  }
  int age = current_year - birth_year;
  int bucket_start = (age / bucket_size) * bucket_size;
  int bucket_end = bucket_start + bucket_size - 1;
  return fmt::format("{}-{}", bucket_start, bucket_end);
}

std::vector<std::string> age_bucket_labels(int bucket_size, int max_age) {
  std::vector<std::string> labels;
  if (bucket_size <= 0 || max_age < 0) {
    return labels;
  }
  for (int start = 0; start <= max_age; start += bucket_size) {
    const int end = std::min(start + bucket_size - 1, max_age);
    labels.push_back(fmt::format("{}-{}", start, end));
  }
  return labels;
}

}  // namespace vote_backend::utils
