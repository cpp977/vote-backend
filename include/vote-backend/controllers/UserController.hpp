/**
 *
 *  UserController.h
 *  Controller for admin-only user management endpoints.
 *
 *  Provides an endpoint that lists all users by their id and username.
 *  Returns a JSON array of user objects containing id and username fields only.
 *  Requires the AdminAuthFilter to access.
 */

#pragma once

#include <drogon/HttpController.h>

class UserController : public drogon::HttpController<UserController> {
  public:
    METHOD_LIST_BEGIN
    // Admin-only endpoint to list all users by username only
    ADD_METHOD_TO(UserController::list_users, "/admin/users", drogon::Get,
                  drogon::Options, "AdminAuthFilter");
    METHOD_LIST_END

    void list_users(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& cb);
};