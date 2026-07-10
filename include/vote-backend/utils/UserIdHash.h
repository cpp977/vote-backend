#pragma once

#include <cstdint>
#include <string>

namespace vote_backend::utils {

/// Hashes a numeric user id into an opaque, non-reversible token that is safe
/// to persist in the database without disclosing which concrete user answered
/// a given question.
///
/// The hash is an HMAC-SHA256 (hex-encoded) of the decimal user-id string,
/// keyed with a server-side secret. Because the key never leaves the server,
/// the stored value cannot be linked back to a specific user.
class UserIdHasher {
 public:
  explicit UserIdHasher(std::string key);

  /// Returns the hex-encoded HMAC-SHA256 of the decimal representation of
  /// @p user_id.
  std::string hash(int64_t user_id) const;

 private:
  std::string key_;
};

/// Returns a process-wide hasher built from the server configuration.
///
/// The secret is resolved as follows:
///   1. `user_id_hash_salt` from config.json (preferred, dedicated salt),
///   2. falls back to `jwt_secret`,
///   3. falls back to a built-in default (with a warning) if neither is set.
const UserIdHasher& user_id_hasher();

}  // namespace vote_backend::utils
