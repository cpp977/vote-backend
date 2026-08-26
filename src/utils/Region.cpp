/**
 *  Region.cpp
 *  Normalizes user-supplied region strings into canonical
 *  ISO 3166-2 subdivision codes so statistics filters match
 *  deterministically instead of fragmenting over spelling variants.
 */

#include "vote-backend/utils/Region.hpp"

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

bool is_ascii_digit(char c) {
  const unsigned char u = static_cast<unsigned char>(c);
  return u >= '0' && u <= '9';
}

}  // namespace

bool is_valid_region(const std::string& value) {
  // ISO 3166-2: "<alpha-2 country>-<1..3 alphanumeric>", i.e. 4-6 characters
  // total with the separator fixed at index 2.
  if (value.size() < 4 || value.size() > 6 || value[2] != '-') {
    return false;
  }
  if (!is_ascii_alpha(value[0]) || !is_ascii_alpha(value[1])) {
    return false;
  }
  for (std::size_t i = 3; i < value.size(); ++i) {
    if (!is_ascii_alpha(value[i]) && !is_ascii_digit(value[i])) {
      return false;
    }
  }
  return true;
}

std::string normalize_region(const std::string& value) {
  std::string normalized = trim(value);
  for (char& c : normalized) {
    c = upper_ascii(c);
  }
  if (!is_valid_region(normalized)) {
    return "";
  }
  return normalized;
}

}  // namespace vote_backend::utils
