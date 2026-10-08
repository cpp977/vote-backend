#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "integration_helpers.hpp"

using namespace test_helpers;

// When VOTE_BACKEND_NATIVE=1 the environment is provided by NativeStack
// (throwaway PostgreSQL cluster plus a natively started backend) instead of the
// Ansible/quadlet deployment. The default stays on Ansible: CI runs
// `ctest --preset test-release` with no override and must keep exercising the
// real deployment.
static bool running_natively() {
  return get_env("VOTE_BACKEND_NATIVE", "") == "1";
}

namespace {
std::unique_ptr<test_helpers::NativeStack> native_stack;
}  // namespace

GlobalTestFixture::GlobalTestFixture() {
  if (running_natively()) {
    const std::string base = get_env("VOTE_BACKEND_SRC", ".");
    native_stack = std::make_unique<test_helpers::NativeStack>(base);
    native_stack->start();
  } else {
    // Resolve paths relative to the project source directory so it works
    // regardless of the working directory CTest sets.
    const char* src_dir = std::getenv("VOTE_BACKEND_SRC");
    std::string base = src_dir ? std::string(src_dir) : ".";
    std::string playbook = base + "/ansible/playbooks/deploy-local-test.yml";
    std::string roles_path = base + "/ansible/roles";
    run_ansible_playbook(playbook, roles_path);
    wait_for_http("127.0.0.1", 8848);
  }
  // Login as the pre-seeded admin user (Admin / 12345678) for testing admin
  // endpoints.
  access_token = login_only("127.0.0.1", 8848, "Admin", "12345678");
}

GlobalTestFixture::~GlobalTestFixture() {
  if (running_natively()) {
    // Tear the scratch cluster and the backend down. Done before the static
    // native_stack below would, so the ordering is explicit.
    native_stack.reset();
    return;
  }
  std::system(
      "systemctl --user stop "
      "vote-backend@$(cat version.txt).service 2>/dev/null");
  std::system("systemctl --user stop drogon-postgres.service 2>/dev/null");
}

GlobalTestFixture global_fixture;
