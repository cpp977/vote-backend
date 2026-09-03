#include "vote-backend/utils/ErrorResponse.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <drogon/orm/Exception.h>
#include <fmt/format.h>
#include <trantor/utils/Logger.h>

using namespace drogon;
using drogon::orm::DrogonDbException;

/**
 * @brief Helper to send a JSON error response.
 */
void send_error(const std::function<void(const HttpResponsePtr&)>& cb,
                const std::string& msg, HttpStatusCode code) {
  Json::Value err;
  err["error"] = msg;
  auto resp = HttpResponse::newHttpJsonResponse(err);
  resp->setStatusCode(code);
  cb(resp);
}

/**
 * @brief Securely send an internal server error response.
 * Logs the detailed error message internally and returns a generic response
 * to avoid leaking internal details to clients.
 *
 * @param cb Callback function to send the response
 * @param errorMessage Detailed error message for logging only
 */
void send_internal_server_error(
    const std::function<void(const HttpResponsePtr&)>& cb,
    const std::string& errorMessage) {
  LOG_ERROR << fmt::format("Internal server error: {}", errorMessage);
  Json::Value err;
  err["error"] = "Internal server error";
  auto resp = HttpResponse::newHttpJsonResponse(err);
  resp->setStatusCode(k500InternalServerError);
  cb(resp);
}

/**
 * @brief Convenience function to safely handle database exceptions.
 * Extracts the error message from DrogonDbException, logs it, and sends a
 * generic internal server error response to avoid exposing database details.
 *
 * @param cb Callback function to send the response
 * @param e Database exception containing detailed error
 */
void send_db_internal_server_error(
    const std::function<void(const HttpResponsePtr&)>& cb,
    const DrogonDbException& e) {
  LOG_ERROR << fmt::format("Database error: {}", e.base().what());
  Json::Value err;
  err["error"] = "Internal server error";
  auto resp = HttpResponse::newHttpJsonResponse(err);
  resp->setStatusCode(k500InternalServerError);
  cb(resp);
}

/**
 * @brief Enhanced send_error that can optionally log errors for server errors.
 * For non-server errors (4xx status codes), uses the original behavior.
 * For server errors (5xx status codes), logs the message and sends a generic
 * response.
 *
 * @param cb Callback function to send the response
 * @param msg Error message
 * @param code HTTP status code
 */
void send_error_secure(const std::function<void(const HttpResponsePtr&)>& cb,
                       const std::string& msg, HttpStatusCode code) {
  if (code >= k500InternalServerError) {
    // For server errors, log the message and send generic response
    LOG_ERROR << fmt::format("Server error: {}", msg);
    Json::Value err;
    err["error"] = "Internal server error";
    auto resp = HttpResponse::newHttpJsonResponse(err);
    resp->setStatusCode(k500InternalServerError);
    cb(resp);
  } else {
    // For client errors, use original behavior
    send_error(cb, msg, code);
  }
}
