#include <drogon/HttpResponse.h>

void send_error(const std::function<void(const drogon::HttpResponsePtr&)>& cb,
                const std::string& msg, drogon::HttpStatusCode code);
