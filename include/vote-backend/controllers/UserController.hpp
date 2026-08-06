/**
 *
 *  UserController.h
 *  Controller for user management endpoints.
 *
 *  Provides endpoints to list all users and get a specific user by ID (admin-only),
 *  activate/deactivate users (admin-only), and delete the authenticated user's own
 *  account (JwtAuthFilter-protected, user can only delete themselves).
 */

#pragma once

#include <drogon/HttpController.h>
#include <drogon/HttpRequest.h>

using namespace drogon;

class UserController : public drogon::HttpController<UserController> {
  public:
    METHOD_LIST_BEGIN
    // Admin-only endpoint to list all users by username only
    ADD_METHOD_TO(UserController::list_users, "/admin/users", drogon::Get,
                  drogon::Options, "AdminAuthFilter");
    // Admin-only endpoint to get a specific user by ID (all fields except password_hash)
    ADD_METHOD_TO(UserController::get_user_by_id, "/admin/users/{1}", drogon::Get,
                  drogon::Options, "AdminAuthFilter");
    // Admin-only endpoint to set a user with a given id inactive
    ADD_METHOD_TO(UserController::set_user_inactive, "/admin/users/{1}/inactive", drogon::Post,
                  drogon::Options, "AdminAuthFilter");
    // Admin-only endpoint to set a given user active
    ADD_METHOD_TO(UserController::set_user_active, "/admin/users/{1}/active", drogon::Post,
                  drogon::Options, "AdminAuthFilter");
    // User endpoint to delete the authenticated user's own account.
    // Protected by JwtAuthFilter; only the calling user can delete themselves.
    ADD_METHOD_TO(UserController::delete_user, "/users/{1}/delete", drogon::Post,
                  drogon::Options, "JwtAuthFilter");
    METHOD_LIST_END

    void list_users(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& cb);

    void get_user_by_id(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& cb,
        int64_t user_id);

    void set_user_inactive(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& cb,
        int64_t user_id);

    void set_user_active(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& cb,
        int64_t user_id);

    void delete_user(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& cb,
        int64_t user_id);
};