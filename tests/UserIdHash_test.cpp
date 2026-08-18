#include "vote-backend/utils/UserIdHash.hpp"

#include <doctest/doctest.h>

using namespace vote_backend::utils;

TEST_CASE(
    "UserIdHasher produces different hashes for same user on different "
    "questions") {
  // Use a fixed key for deterministic testing
  UserIdHasher hasher("test-secret-key-min-32-chars-long");

  const std::string user_id = "550e8400-e29b-41d4-a716-446655440000";
  const int64_t question_id_1 = 100;
  const int64_t question_id_2 = 200;

  std::string hash1 = hasher.hash(user_id, question_id_1);
  std::string hash2 = hasher.hash(user_id, question_id_2);

  // Hashes should be different for different questions
  CHECK(hash1 != hash2);

  // Both should be valid hex strings of 64 characters (SHA256 = 32 bytes = 64
  // hex chars)
  CHECK(hash1.length() == 64);
  CHECK(hash2.length() == 64);

  // Verify they are valid hex
  for (char c : hash1) {
    CHECK(((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')));
  }
  for (char c : hash2) {
    CHECK(((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')));
  }
}

TEST_CASE("UserIdHasher produces same hash for same user and question") {
  UserIdHasher hasher("test-secret-key-min-32-chars-long");

  const std::string user_id = "550e8400-e29b-41d4-a716-446655440000";
  const int64_t question_id = 100;

  std::string hash1 = hasher.hash(user_id, question_id);
  std::string hash2 = hasher.hash(user_id, question_id);

  CHECK(hash1 == hash2);
}

TEST_CASE(
    "UserIdHasher produces different hashes for different users on same "
    "question") {
  UserIdHasher hasher("test-secret-key-min-32-chars-long");

  const std::string user_id_1 = "550e8400-e29b-41d4-a716-446655440000";
  const std::string user_id_2 = "6ba7b810-9dad-11d1-80b4-00c04fd430c8";
  const int64_t question_id = 100;

  std::string hash1 = hasher.hash(user_id_1, question_id);
  std::string hash2 = hasher.hash(user_id_2, question_id);

  CHECK(hash1 != hash2);
}

TEST_CASE("UserIdHasher produces different hashes with different keys") {
  UserIdHasher hasher1("key-one-min-32-characters-long");
  UserIdHasher hasher2("key-two-min-32-characters-long");

  const std::string user_id = "550e8400-e29b-41d4-a716-446655440000";
  const int64_t question_id = 100;

  std::string hash1 = hasher1.hash(user_id, question_id);
  std::string hash2 = hasher2.hash(user_id, question_id);

  CHECK(hash1 != hash2);
}