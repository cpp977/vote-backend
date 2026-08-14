#include <fmt/core.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <cstdlib>
#include <string>

#include "drogon/HttpAppFramework.h"

#include "vote-backend/utils/Config.hpp"

int main() {
  char* conf_path_c = nullptr;
  conf_path_c = std::getenv("VOTE_BACKEND_CONFPATH");
  std::string conf_path = "/etc/vote";
  if (conf_path_c != nullptr) {
    conf_path = conf_path_c;
  }
  if (!trantor::Logger::getSpdLogger()) {
    trantor::Logger::enableSpdLog();
    auto logger = trantor::Logger::getSpdLogger();
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stderr_color_sink_mt>());
    for (auto& sink : sinks)
      sink->set_pattern("%Y%m%d %T.%f %6t %^%=8l%$ [%!] %v - %s:%#");
    logger->sinks() = sinks;
  }
  spdlog::flush_every(std::chrono::seconds(1));

  drogon::app().loadConfigFile(fmt::format("{}/config.json", conf_path));

  auto cfg = load_cors_config();
  
  drogon::app().registerPreRoutingAdvice(
      [cfg](const drogon::HttpRequestPtr& req, drogon::AdviceCallback&& acb,
            drogon::AdviceChainCallback&& accb) {
        if (req->method() == drogon::Options) {
          auto resp = drogon::HttpResponse::newHttpResponse();
          resp->addHeader("Access-Control-Allow-Origin", cfg.allowed_origin);
          resp->addHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
          resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
          resp->setStatusCode(drogon::k204NoContent);
          acb(resp);
          return;
        }
        accb();
      });

  drogon::app().registerPostHandlingAdvice(
      [cfg](const drogon::HttpRequestPtr& req,
            const drogon::HttpResponsePtr& resp) {
        resp->addHeader("Access-Control-Allow-Origin", cfg.allowed_origin);
      });
  drogon::app().run();
  return 0;
}
