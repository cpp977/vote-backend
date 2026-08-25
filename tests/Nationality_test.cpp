/**
 *  Nationality_test.cpp
 *  Unit tests for the nationality -> ISO 3166-1 alpha-2 normalization used by
 *  registration and the stats tag filter.
 */

#include "vote-backend/utils/Nationality.hpp"

#include <doctest/doctest.h>

using vote_backend::utils::is_valid_nationality;
using vote_backend::utils::normalize_nationality;

TEST_CASE("valid alpha-2 codes normalize to uppercase") {
  CHECK(normalize_nationality("de") == "DE");
  CHECK(normalize_nationality("DE") == "DE");
  CHECK(normalize_nationality("De") == "DE");
  CHECK(normalize_nationality(" dE ") == "DE");
  CHECK(normalize_nationality("at") == "AT");
  CHECK(normalize_nationality("CH") == "CH");
  CHECK(normalize_nationality("\tfr\n") == "FR");
}

TEST_CASE("invalid inputs normalize to empty") {
  CHECK(normalize_nationality("").empty());
  CHECK(normalize_nationality("   ").empty());
  CHECK(normalize_nationality("G").empty());
  CHECK(normalize_nationality("GER").empty());
  CHECK(normalize_nationality("German").empty());
  CHECK(normalize_nationality("D3").empty());
  CHECK(normalize_nationality("1A").empty());
  CHECK(normalize_nationality("Ö").empty());
  // Non-ASCII letters are rejected even when they look alphabetic.
  CHECK(normalize_nationality("ÄÖ").empty());
}

TEST_CASE("validity check mirrors normalization") {
  CHECK(is_valid_nationality("DE"));
  CHECK(is_valid_nationality("de"));
  CHECK_FALSE(is_valid_nationality(""));
  CHECK_FALSE(is_valid_nationality("D"));
  CHECK_FALSE(is_valid_nationality("DES"));
  CHECK_FALSE(is_valid_nationality("D "));
}
