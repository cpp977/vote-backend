/**
 *
 *  UserController.h
 *  Controller for admin-only user management endpoints.
 *
 *  Provides endpoints to list all users and get a specific user by ID.
 *  Returns user objects with id and username (for listing) or all fields except password_hash (for single user).
 *  Requires the AdminAuthFilter to access.
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
    METHOD_LIST_END

    void list_users(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& cb);
    
    void get_user_by_id(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& cb,
        int64_t user_id);
};