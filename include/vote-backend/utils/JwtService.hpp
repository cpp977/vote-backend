/**
 *
 *  JwtService.h
 *  JWT token generation and verification utility.
 */

#pragma once

#include <json/json.h>
#include <string>
#include <optional>

namespace vote_backend::utils {

/**
 * @brief Service for creating and verifying JWT tokens.
 *
 * Uses HS256 (HMAC-SHA256) for signing.
 * Access tokens are short-lived; refresh tokens are long-lived.
 */
class JwtService {
public:
    JwtService() = default;

    /**
     * @brief Initialize the JWT service with configuration.
     * @param secret The secret key for signing tokens.
     * @param access_expiry_minutes Minutes until access token expires.
     * @param refresh_expiry_days Days until refresh token expires.
     */
    JwtService(const std::string &secret,
               int access_expiry_minutes = 15,
               int refresh_expiry_days = 7);

    /**
     * @brief Generate a short-lived access token.
     * @param user_id The user's database ID.
     * @param username The user's username.
     * @param is_admin Whether the user has admin privileges. Embedded as the
     *                 `is_admin` claim so the AdminAuthFilter can enforce it.
     * @param is_active Whether the user account is active. Embedded as the
     *                  `is_active` claim so the JwtAuthFilter can enforce it.
     * @return The encoded JWT string.
     */
    std::string generate_access_token(int64_t user_id,
                                      const std::string &username,
                                      bool is_admin = false,
                                      bool is_active = true) const;

    /**
     * @brief Generate a long-lived refresh token.
     * @param user_id The user's database ID.
     * @return The encoded JWT string (includes a unique jti claim).
     */
    std::string generate_refresh_token(int64_t user_id) const;

    /**
     * @brief Verify a JWT token and extract its claims.
     * @param token The JWT string to verify.
     * @return Json::Value containing the claims if valid, or nullptr/empty if invalid.
     */
    Json::Value verify_token(const std::string &token) const;

    /**
     * @brief Extract the jti (JWT ID) claim from a token without full verification.
     * @param token The JWT string.
     * @return The jti value, or empty string if absent.
     */
    std::string extract_jti_unsafe(const std::string &token) const;

private:
    std::string secret_;
    int access_expiry_minutes_ = 15;
    int refresh_expiry_days_ = 7;
};

/**
 * @brief Create a JwtService instance from the application configuration.
 * @return A configured JwtService instance.
 *
 * Reads configuration from the project's config.json file.
 */
JwtService make_jwt_service();

} // namespace vote_backend::utils
