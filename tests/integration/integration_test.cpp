#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "integration_helpers.hpp"

using namespace test_helpers;

GlobalTestFixture::GlobalTestFixture() {
  // Resolve paths relative to the project source directory so it works
  // regardless of the working directory CTest sets.
  const char* src_dir = std::getenv("VOTE_BACKEND_SRC");
  std::string base = src_dir ? std::string(src_dir) : ".";
  std::string playbook = base + "/ansible/playbooks/deploy-local.yml";
  std::string roles_path = base + "/ansible/roles";
  run_ansible_playbook(playbook, roles_path);
  wait_for_http("127.0.0.1", 8848);
  // Login as the pre-seeded user (Jim / 12345678) — no registration needed.
  access_token = login_only("127.0.0.1", 8848, "Jim", "12345678");
}

GlobalTestFixture::~GlobalTestFixture() {
  std::system(
      "systemctl --user stop "
      "vote-backend@$(cat version.txt).service 2>/dev/null");
  std::system("systemctl --user stop drogon-postgres.service 2>/dev/null");
}

GlobalTestFixture global_fixture;
