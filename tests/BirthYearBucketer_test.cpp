/**
 *  BirthYearBucketer_test.cpp
 *  Unit tests for the birth-year -> age-bucket normalization used by the
 *  answer-submission endpoint.
 */

#include "vote-backend/utils/BirthYearBucketer.hpp"

#include <doctest/doctest.h>

#include <string>

using vote_backend::utils::bucketBirthYear;
using vote_backend::utils::currentYear;

TEST_CASE("bucketBirthYear buckets ages with a 5-year bucket size") {
  // Fixed reference year so the expectations stay stable over time.
  const int cy = 2025;

  // age = 2025 - birth_year; bucket start = (age / 5) * 5.
  CHECK(bucketBirthYear(1998, 5, cy) == "25-29");  // age 27
  CHECK(bucketBirthYear(1990, 5, cy) == "35-39");  // age 35 (bucket start)
  CHECK(bucketBirthYear(1989, 5, cy) == "35-39");  // age 36
  CHECK(bucketBirthYear(2000, 5, cy) == "25-29");  // age 25 (bucket start)
  CHECK(bucketBirthYear(1999, 5, cy) == "25-29");  // age 26
  CHECK(bucketBirthYear(2015, 5, cy) == "10-14");  // age 10 (bucket start)
  CHECK(bucketBirthYear(2019, 5, cy) == "5-9");    // age 6
  CHECK(bucketBirthYear(2025, 5, cy) == "0-4");    // age 0
}

TEST_CASE("bucketBirthYear honours different bucket sizes") {
  const int cy = 2025;

  // 10-year buckets.
  CHECK(bucketBirthYear(1990, 10, cy) == "30-39");  // age 35
  CHECK(bucketBirthYear(2000, 10, cy) == "20-29");  // age 25

  // 1-year buckets: every age is its own bucket.
  CHECK(bucketBirthYear(1990, 1, cy) == "35-35");

  // 15-year buckets.
  CHECK(bucketBirthYear(1990, 15, cy) == "30-44");  // age 35
}

TEST_CASE("bucketBirthYear rejects invalid bucket sizes") {
  CHECK(bucketBirthYear(1990, 0, 2025).empty());
  CHECK(bucketBirthYear(1990, -1, 2025).empty());
}

TEST_CASE("bucketBirthYear rejects implausible birth years") {
  const int cy = 2025;

  // Too old to be plausible.
  CHECK(bucketBirthYear(1900, 5, cy).empty());
  CHECK(bucketBirthYear(1800, 5, cy).empty());
  CHECK(bucketBirthYear(0, 5, cy).empty());

  // In the future relative to the reference year.
  CHECK(bucketBirthYear(2026, 5, cy).empty());

  // The earliest accepted year is 1901.
  CHECK_FALSE(bucketBirthYear(1901, 5, cy).empty());
}

TEST_CASE("currentYear returns a plausible calendar year") {
  // Sanity check only: the wall-clock year must be recent. The lower bound is
  // deliberately generous so the test does not rot.
  CHECK(currentYear() >= 2024);
}
