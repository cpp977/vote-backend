/**
 *
 *  AuthController.h
 *  Controller for authentication endpoints: register, login, logout, refresh.
 */

#pragma once

#include <drogon/HttpController.h>

class AuthController : public drogon::HttpController<AuthController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthController::register_user, "/register", drogon::Post);
    ADD_METHOD_TO(AuthController::login,         "/login",    drogon::Post);
    ADD_METHOD_TO(AuthController::logout,        "/logout",   drogon::Post);
    ADD_METHOD_TO(AuthController::refresh,       "/refresh",  drogon::Post);
    ADD_METHOD_TO(AuthController::me,            "/me",       drogon::Get, drogon::Options, "JwtAuthFilter");
    ADD_METHOD_TO(AuthController::update_me,     "/me",       drogon::Patch, drogon::Options, "JwtAuthFilter");
    ADD_METHOD_TO(AuthController::update_me,     "/me",       drogon::Put, drogon::Options, "JwtAuthFilter");
    ADD_METHOD_TO(AuthController::forgot_password, "/user/password/forgot", drogon::Post);
    ADD_METHOD_TO(AuthController::reset_password, "/user/password/reset", drogon::Post);
    METHOD_LIST_END

    void register_user(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&cb);

    void login(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&cb);

    void logout(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&cb);

    void refresh(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&cb);

    void me(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&cb);

    void update_me(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&cb);

    /**
     * @brief POST /user/password/forgot
     *
     * Request a password-reset link. The caller supplies an email address; if
     * an account exists for that email and the caller is not rate-limited, a
     * single-use token is generated, hashed, persisted, and used to build a
     * reset link that is sent via the configured email provider (Resend).
     *
     * The endpoint always returns the same generic success response regardless
     * of whether the email exists or was rate-limited, to prevent user
     * enumeration.
     */
    void forgot_password(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&cb);

    /**
     * @brief POST /user/password/reset
     *
     * Consume a password-reset token together with a new password. The supplied
     * token is SHA-256 hashed and looked up in the password_resets table. On
     * success the user's password_hash is updated, password_changed_at is set
     * to NOW(), the token is marked used, and all of the user's refresh
     * tokens are revoked. On failure the token's attempt_count is incremented
     * and, after a configurable number of failures, the token is killed.
     */
    void reset_password(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&cb);
};
