/**
 *  Nationality.cpp
 *  Normalizes user-supplied nationality strings into canonical
 *  ISO 3166-1 alpha-2 country codes so statistics filters match
 *  deterministically instead of fragmenting over spelling variants.
 */

#include "vote-backend/utils/Nationality.hpp"

#include <cctype>

namespace vote_backend::utils {

namespace {

std::string trim(const std::string& value) {
  const auto begin = value.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) {
    return "";
  }
  const auto end = value.find_last_not_of(" \t\r\n");
  return value.substr(begin, end - begin + 1);
}

char upper_ascii(char c) {
  return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
}

bool is_ascii_alpha(char c) {
  const unsigned char u = static_cast<unsigned char>(c);
  return (u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z');
}

}  // namespace

bool is_valid_nationality(const std::string& value) {
  if (value.size() != 2) {
    return false;
  }
  return is_ascii_alpha(value[0]) && is_ascii_alpha(value[1]);
}

std::string normalize_nationality(const std::string& value) {
  const std::string trimmed = trim(value);
  if (!is_valid_nationality(trimmed)) {
    return "";
  }
  std::string normalized;
  normalized.reserve(2);
  normalized.push_back(upper_ascii(trimmed[0]));
  normalized.push_back(upper_ascii(trimmed[1]));
  return normalized;
}

}  // namespace vote_backend::utils
