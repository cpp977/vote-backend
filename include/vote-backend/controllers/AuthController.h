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
};
