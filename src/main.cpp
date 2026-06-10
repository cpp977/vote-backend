#include <fmt/core.h>

#include <cstdlib>
#include <string>

#include "drogon/HttpAppFramework.h"

int main() {
  char* conf_path_c = nullptr;
  conf_path_c = std::getenv("VOTE_BACKEND_CONFPATH");
  std::string conf_path = "/etc/vote";
  if (conf_path_c != nullptr) {
    conf_path = conf_path_c;
  }
  drogon::app().loadConfigFile(fmt::format("{}/config.json", conf_path));
  drogon::app().run();
  return 0;
}
