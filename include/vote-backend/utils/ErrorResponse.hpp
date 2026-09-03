#ifndef ERROR_RESPONSE_HPP
#define ERROR_RESPONSE_HPP

#include <drogon/HttpResponse.h>
#include <drogon/orm/Exception.h>
#include <string>
#include <functional>

void send_error(const std::function<void(const drogon::HttpResponsePtr&)>& cb,
                const std::string& msg, drogon::HttpStatusCode code);

/**
 * @brief Securely send an internal server error response.
 * Logs the detailed error message internally and returns a generic response
 * to avoid leaking internal details to clients.
 *
 * @param cb Callback function to send the response
 * @param errorMessage Detailed error message for logging only
 */
void send_internal_server_error(
    const std::function<void(const drogon::HttpResponsePtr&)>& cb,
    const std::string& errorMessage);

/**
 * @brief Convenience function to safely handle database exceptions.
 * Extracts the error message from DrogonDbException, logs it, and sends a generic
 * internal server error response to avoid exposing database details.
 *
 * @param cb Callback function to send the response
 * @param e Database exception containing detailed error
 */
void send_db_internal_server_error(
    const std::function<void(const drogon::HttpResponsePtr&)>& cb,
    const drogon::orm::DrogonDbException& e);

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
void send_error_secure(const std::function<void(const drogon::HttpResponsePtr&)>& cb,
                       const std::string& msg, drogon::HttpStatusCode code);

#endif  // ERROR_RESPONSE_HPP
