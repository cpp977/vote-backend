/**
 *  Region_test.cpp
 *  Unit tests for the region -> ISO 3166-2 subdivision normalization used by
 *  registration, profile updates and the stats tag filter.
 */

#include "vote-backend/utils/Region.hpp"

#include <doctest/doctest.h>

using vote_backend::utils::is_valid_region;
using vote_backend::utils::normalize_region;

TEST_CASE("valid subdivision codes normalize to uppercase") {
  CHECK(normalize_region("de-be") == "DE-BE");
  CHECK(normalize_region("DE-BE") == "DE-BE");
  CHECK(normalize_region("De-Be") == "DE-BE");
  CHECK(normalize_region(" us-ca ") == "US-CA");
  CHECK(normalize_region("\tin-up\n") == "IN-UP");
  // Single-letter and three-letter suffixes occur in the standard.
  CHECK(normalize_region("ar-b") == "AR-B");
  CHECK(normalize_region("eg-pts") == "EG-PTS");
  // Numeric suffixes are part of the standard (e.g. CN-11 = Beijing).
  CHECK(normalize_region("cn-11") == "CN-11");
}

TEST_CASE("invalid inputs normalize to empty") {
  CHECK(normalize_region("").empty());
  CHECK(normalize_region("   ").empty());
  CHECK(normalize_region("DE").empty());
  CHECK(normalize_region("DEBE").empty());
  CHECK(normalize_region("DE_BE").empty());
  CHECK(normalize_region("-BE").empty());
  CHECK(normalize_region("DE-").empty());
  CHECK(normalize_region("DE-BE-X").empty());
  CHECK(normalize_region("D3-AB").empty());
  CHECK(normalize_region("1E-AB").empty());
  CHECK(normalize_region("DE-B!").empty());
  CHECK(normalize_region("ÄÖ-XX").empty());
}

TEST_CASE("validity check mirrors normalization") {
  CHECK(is_valid_region("DE-BE"));
  CHECK(is_valid_region("de-be"));
  CHECK_FALSE(is_valid_region(""));
  CHECK_FALSE(is_valid_region("DE"));
  CHECK_FALSE(is_valid_region("DE-"));
  CHECK_FALSE(is_valid_region("DE-BE-X"));
  CHECK_FALSE(is_valid_region("DE B E"));
}
