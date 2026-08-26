#pragma once

#include <string>

namespace vote_backend::utils {

/// True when @p value is a syntactically valid ISO 3166-2 subdivision code:
/// an alpha-2 country code, a hyphen, and a 1-3 character alphanumeric
/// subdivision suffix (case-insensitive).
bool is_valid_region(const std::string& value);

/// Normalizes free-text input into the canonical form stored in the database
/// and used inside `user_answers.tags`: an uppercase ISO 3166-2 subdivision
/// code (e.g. "de-be" -> "DE-BE"). Leading/trailing whitespace is ignored.
///
/// Returns an empty string when the input is not a syntactically valid
/// subdivision code, signalling to the caller that the value must be
/// rejected. Whether the code is actually assigned is decided by the
/// `regions` reference table at the database layer.
std::string normalize_region(const std::string& value);

}  // namespace vote_backend::utils
