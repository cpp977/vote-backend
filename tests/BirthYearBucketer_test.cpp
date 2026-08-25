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

TEST_CASE("ageBucketLabels generates ordered decade labels by default") {
  using vote_backend::utils::age_bucket_labels;

  const auto labels = age_bucket_labels(10);
  REQUIRE(labels.size() == 12);  // ages 0..119 -> starts 0,10,...,110
  CHECK(labels.front() == "0-9");
  CHECK(labels[1] == "10-19");
  CHECK(labels[6] == "60-69");
  CHECK(labels.back() == "110-119");
}

TEST_CASE("ageBucketLabels honours custom sizes and bounds") {
  using vote_backend::utils::age_bucket_labels;

  const auto five = age_bucket_labels(5, 14);
  REQUIRE(five.size() == 3);
  CHECK(five.front() == "0-4");
  CHECK(five[1] == "5-9");
  CHECK(five.back() == "10-14");  // last label clipped to max_age

  // Labels are consistent with what bucketBirthYear produces.
  const int cy = currentYear();
  const auto labels = age_bucket_labels(10);
  for (int age = 0; age <= 119; ++age) {
    const int birth_year = cy - age;
    const std::string expected = bucketBirthYear(birth_year, 10, cy);
    REQUIRE(expected == labels[static_cast<size_t>(age / 10)]);
  }

  CHECK(age_bucket_labels(0).empty());
  CHECK(age_bucket_labels(-3).empty());
  CHECK(age_bucket_labels(10, -1).empty());
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
