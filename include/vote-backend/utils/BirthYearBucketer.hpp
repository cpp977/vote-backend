#pragma once

#include <string>

namespace vote_backend::utils {

/// Returns the current calendar year derived from the system wall clock.
int currentYear();

/// Normalizes a birth year into a human-readable age-range bucket label.
///
/// The caller's age is computed relative to @p current_year (which defaults to
/// the real wall-clock year so callers do not need to pass it explicitly).
/// The age is then placed into a bucket of @p bucket_size years and returned
/// as a label of the form "<start>-<end>", e.g. "25-29" for a 5-year bucket
/// covering ages 25, 26, 27, 28 and 29.
///
/// Returns an empty string when the input is unreasonable, signalling to the
/// caller that the field should be left untouched rather than replaced with an
/// invalid bucket:
///   - @p bucket_size is <= 0
///   - @p birth_year is <= 1900 (implausibly old)
///   - @p birth_year is in the future (> @p current_year)
///
/// @param birth_year   The four-digit birth year submitted by the user.
/// @param bucket_size  Width of each age bucket in years (e.g. 5).
/// @param current_year The year to compute the age against (defaults to the
///                     real wall-clock year via currentYear()).
std::string bucketBirthYear(int birth_year, int bucket_size,
                            int current_year = currentYear());

}  // namespace vote_backend::utils
