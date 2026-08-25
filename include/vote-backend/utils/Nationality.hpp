#pragma once

#include <string>

namespace vote_backend::utils {

/// True when @p value is a syntactically valid ISO 3166-1 alpha-2 country
/// code: exactly two ASCII letters (case-insensitive).
bool is_valid_nationality(const std::string& value);

/// Normalizes free-text input into the canonical form stored in the database
/// and used inside `user_answers.tags`: an uppercase ISO 3166-1 alpha-2
/// country code (e.g. "de" -> "DE"). Leading/trailing whitespace is ignored.
///
/// Returns an empty string when the input is not a valid alpha-2 code,
/// signalling to the caller that the value must be rejected.
std::string normalize_nationality(const std::string& value);

}  // namespace vote_backend::utils
