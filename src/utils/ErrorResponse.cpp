#include "vote-backend/utils/ErrorResponse.hpp"

#include <drogon/HttpResponse.h>

using namespace drogon;

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
