#pragma once

#include <doctest/doctest.h>
#include <fmt/format.h>

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace test_helpers {

// ---------------------------------------------------------------------------
// Environment helpers
// ---------------------------------------------------------------------------
inline std::string get_env(const std::string& key,
                           const std::string& default_val) {
  const char* val = std::getenv(key.c_str());
  return val ? std::string(val) : default_val;
}

// ---------------------------------------------------------------------------
// Ansible + service readiness
// ---------------------------------------------------------------------------
inline void run_ansible_playbook(const std::string& playbook,
                                 const std::string& roles_path = "") {
  // Set ANSIBLE_ROLES_PATH so Ansible can find roles regardless of cwd.
  std::string env_setup;
  if (!roles_path.empty()) {
    env_setup = "ANSIBLE_ROLES_PATH=" + roles_path + " ";
  }
  // Use the project's inventory file which defines the 'dev' group.
  std::string inventory =
      fmt::format(" -i {}/ansible/inventory/hosts.yml",
                  std::string(get_env("VOTE_BACKEND_SRC", ".")));
  std::string ansible_opts =
      fmt::format(" --vault-id dev@~/.ansible_vault_pass_dev");
  std::string cmd = fmt::format("{} ansible-playbook {} {} {}", env_setup,
                                playbook, inventory, ansible_opts);
  int rc = std::system(cmd.c_str());
  if (rc != 0) {
    throw std::runtime_error("Ansible playbook failed: " + playbook);
  }
}

inline void wait_for_http(const std::string& host, unsigned short port,
                          std::chrono::seconds timeout = std::chrono::seconds{
                              30}) {
  using boost::asio::ip::tcp;
  boost::asio::io_context ioc;
  tcp::resolver resolver(ioc);
  auto const results = resolver.resolve(host, std::to_string(port));

  auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    boost::system::error_code ec;
    tcp::socket socket(ioc);
    socket.connect(*results.begin(), ec);
    if (!ec) {
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{200});
  }
  throw std::runtime_error("Timeout while waiting for HTTP service");
}

// ---------------------------------------------------------------------------
// HTTP client
// ---------------------------------------------------------------------------
struct HttpResponse {
  int status;
  nlohmann::json json_body;
  std::string raw_body;
};

inline HttpResponse http_request(
    const std::string& method, const std::string& host, unsigned short port,
    const std::string& target, const std::string& body = "",
    const std::string& content_type = "application/json",
    const std::string& bearer_token = "") {
  namespace beast = boost::beast;
  namespace http = beast::http;
  using tcp = boost::asio::ip::tcp;

  boost::asio::io_context ioc;
  tcp::resolver resolver(ioc);
  auto const results = resolver.resolve(host, std::to_string(port));

  beast::tcp_stream stream(ioc);
  stream.connect(results);

  http::request<http::string_body> req;
  // Accept arbitrary HTTP verbs (GET, POST, PUT, PATCH, DELETE, OPTIONS, ...)
  // so that tests can exercise methods beyond the two that were hard-coded
  // previously.
  req.method(http::string_to_verb(method));
  req.target(target);
  req.version(11);
  req.set(http::field::host, host);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  req.set(http::field::content_type, content_type);
  if (!bearer_token.empty()) {
    req.set(http::field::authorization, "Bearer " + bearer_token);
  }
  if (!body.empty()) {
    req.body() = body;
    req.prepare_payload();
  }

  http::write(stream, req);

  beast::flat_buffer buffer;
  http::response<http::string_body> res;
  http::read(stream, buffer, res);

  beast::error_code ec;
  stream.socket().shutdown(tcp::socket::shutdown_both, ec);

  HttpResponse out;
  out.status = static_cast<int>(res.result_int());
  out.raw_body = res.body();
  try {
    out.json_body = nlohmann::json::parse(res.body());
  } catch (...) {
  }
  return out;
}

// ---------------------------------------------------------------------------
// JSON helpers
// ---------------------------------------------------------------------------
inline void check_json_has_keys(const nlohmann::json& j,
                                const std::vector<std::string>& keys,
                                const std::string& context) {
  for (const auto& key : keys) {
    INFO(context << ": checking key \"" << key << "\"");
    CHECK(j.contains(key));
  }
}

// ---------------------------------------------------------------------------
// Authentication helper
// ---------------------------------------------------------------------------
inline std::string authenticate(const std::string& host, unsigned short port,
                                const std::string& username = "testuser",
                                const std::string& email = "test@example.com",
                                const std::string& password = "password123",
                                int birth_year = 1990,
                                const std::string& gender = "m",
                                const std::string& nationality = "US") {
  // 1. Register (tolerate 409 – user may already exist).
  nlohmann::json reg_payload = {
      {"username", username}, {"email", email},
      {"password", password}, {"birth_year", birth_year},
      {"gender", gender},     {"nationality", nationality}};
  auto reg_resp =
      http_request("POST", host, port, "/register", reg_payload.dump());
  if (reg_resp.status != 201 && reg_resp.status != 409) {
    throw std::runtime_error("Registration failed with status " +
                             std::to_string(reg_resp.status) + ": " +
                             reg_resp.raw_body);
  }

  // 2. Login – must succeed.
  nlohmann::json login_payload = {{"username", username},
                                  {"password", password}};
  auto login_resp =
      http_request("POST", host, port, "/login", login_payload.dump());
  if (login_resp.status != 200) {
    throw std::runtime_error("Login failed with status " +
                             std::to_string(login_resp.status) + ": " +
                             login_resp.raw_body);
  }

  std::string access_token =
      login_resp.json_body["access_token"].get<std::string>();
  if (access_token.empty()) {
    throw std::runtime_error("Login response missing access_token");
  }
  return access_token;
}

// ---------------------------------------------------------------------------
// Login-only helper for pre-seeded users (no registration attempt)
// ---------------------------------------------------------------------------
inline std::string login_only(const std::string& host, unsigned short port,
                              const std::string& username,
                              const std::string& password) {
  nlohmann::json login_payload = {{"username", username},
                                  {"password", password}};
  auto login_resp =
      http_request("POST", host, port, "/login", login_payload.dump());
  if (login_resp.status != 200) {
    throw std::runtime_error("Login failed with status " +
                             std::to_string(login_resp.status) + ": " +
                             login_resp.raw_body);
  }

  std::string access_token =
      login_resp.json_body["access_token"].get<std::string>();
  if (access_token.empty()) {
    throw std::runtime_error("Login response missing access_token");
  }
  return access_token;
}

// ---------------------------------------------------------------------------
// Native (container-free) stack
//
// Brings up everything the integration suite needs without podman, quadlets or
// Ansible: a throwaway PostgreSQL cluster in a scratch directory, a freshly
// seeded `vote` database, and the backend binary started against it.
//
// Used when VOTE_BACKEND_NATIVE=1. The default path stays on the Ansible
// deployment (see run_ansible_playbook) because CI relies on it.
//
// The database is rebuilt on every run, which is required: the suite is
// order-dependent and mutates state (it registers users, answers questions and
// deactivates accounts), so reusing a database makes later runs fail. This
// mirrors what vote_backend_postgres_persistent: false does for the container
// path.
// ---------------------------------------------------------------------------
class NativeStack {
 public:
  explicit NativeStack(std::string repo_root)
      : repo_root_(std::move(repo_root)),
        keep_(get_env("VOTE_BACKEND_NATIVE_KEEP", "") == "1") {}

  ~NativeStack() {
    // Stop the backend first so it cannot outlive the cluster it is attached
    // to and spam connection errors while postgres shuts down.
    if (backend_pid_ > 0) {
      ::kill(backend_pid_, SIGTERM);
      int status = 0;
      // Give it a moment to exit cleanly before forcing the issue.
      for (int i = 0; i < 50; ++i) {
        if (::waitpid(backend_pid_, &status, WNOHANG) == backend_pid_) {
          backend_pid_ = -1;
          break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
      }
      if (backend_pid_ > 0) {
        ::kill(backend_pid_, SIGKILL);
        ::waitpid(backend_pid_, &status, 0);
        backend_pid_ = -1;
      }
    }
    if (!pgdata_.empty()) {
      run_quiet(fmt::format("pg_ctl -D {} -m immediate stop", pgdata_));
    }
    if (!scratch_.empty()) {
      if (keep_) {
        std::cerr << "[native] scratch kept for inspection: " << scratch_
                  << std::endl;
      } else {
        run_quiet(fmt::format("rm -rf {}", scratch_));
      }
    }
  }

  NativeStack(const NativeStack&) = delete;
  NativeStack& operator=(const NativeStack&) = delete;

  void start() {
    require_tool("initdb");
    require_tool("pg_ctl");
    require_tool("psql");
    require_tool("createdb");

    scratch_ = make_scratch_dir();
    pgdata_ = fmt::format("{}/pgdata", scratch_);
    conf_dir_ = fmt::format("{}/conf", scratch_);
    sock_dir_ = fmt::format("{}/sock", scratch_);
    for (const auto& d : {pgdata_, conf_dir_, sock_dir_}) {
      run(fmt::format("mkdir -p {}", d));
    }

    // trust auth on the unix socket only: this cluster is scratch, lives for a
    // single test run, and is not listening on any TCP port.
    run(fmt::format("initdb -D {} -U postgres --auth-local=trust "
                    "--auth-host=reject",
                    pgdata_));
    run(fmt::format("pg_ctl -D {} -l {}/pg.log -o \"-p {} -k {} "
                    "-c listen_addresses=''\" start",
                    pgdata_, scratch_, pg_port_, sock_dir_));
    wait_for_postgres();

    run(fmt::format("psql -h {} -p {} -U postgres -d postgres -c "
                    "\"DROP DATABASE IF EXISTS vote\"",
                    sock_dir_, pg_port_));
    run(fmt::format("psql -h {} -p {} -U postgres -d postgres -c "
                    "\"CREATE DATABASE vote\"",
                    sock_dir_, pg_port_));

    // Each file runs in a single psql session on purpose: 003_seed_data.sql
    // relies on currval('questions_id_seq') across statements.
    for (const char* f : {"001_init", "002_auth", "003_seed_data",
                          "004_test_data", "005_regions"}) {
      run(fmt::format("psql -h {} -p {} -U postgres -d vote "
                      "-v ON_ERROR_STOP=1 -f {}/sql/{}.sql",
                      sock_dir_, pg_port_, repo_root_, f));
    }

    write_backend_config();
    start_backend();
    try {
      wait_for_http("127.0.0.1", 8848, std::chrono::seconds{60});
    } catch (const std::exception& e) {
      // The scratch dir is about to be destroyed, so surface the backend's own
      // log while it still exists; otherwise a startup failure is invisible.
      std::cerr << "[native] backend failed to start: " << e.what() << "\n"
                << "[native] backend log tail:\n"
                << tail_of(fmt::format("{}/backend.log", scratch_), 40)
                << std::endl;
      throw;
    }
  }

  static std::string tail_of(const std::string& path, size_t lines) {
    std::ifstream f(path);
    if (!f) return "  (no log file at " + path + ")";
    std::vector<std::string> all;
    std::string line;
    while (std::getline(f, line)) all.push_back(line);
    std::string out;
    for (size_t i = all.size() > lines ? all.size() - lines : 0; i < all.size();
         ++i) {
      out += "  " + all[i] + "\n";
    }
    return out;
  }

 private:
  static void require_tool(const char* tool) {
    if (std::system(fmt::format("command -v {} >/dev/null 2>&1", tool).c_str()) !=
        0) {
      throw std::runtime_error(
          fmt::format("'{}' not found on PATH. Install the PostgreSQL server "
                      "package to run the integration tests natively.",
                      tool));
    }
  }

  static std::string make_scratch_dir() {
    std::string tmpl = "/tmp/vote-backend-native-XXXXXX";
    std::vector<char> buf(tmpl.begin(), tmpl.end());
    buf.push_back('\0');
    if (::mkdtemp(buf.data()) == nullptr) {
      throw std::runtime_error("mkdtemp failed: " +
                               std::string(std::strerror(errno)));
    }
    return std::string(buf.data());
  }

  void run(const std::string& cmd) const {
    if (std::system((cmd + " 2>&1").c_str()) != 0) {
      throw std::runtime_error("command failed: " + cmd);
    }
  }

  static void run_quiet(const std::string& cmd) {
    std::system((cmd + " >/dev/null 2>&1").c_str());
  }

  void wait_for_postgres() const {
    const auto probe = fmt::format(
        "psql -h {} -p {} -U postgres -d postgres -c \"SELECT 1\"",
        sock_dir_, pg_port_);
    for (int i = 0; i < 60; ++i) {
      if (std::system((probe + " >/dev/null 2>&1").c_str()) == 0) return;
      std::this_thread::sleep_for(std::chrono::milliseconds{250});
    }
    throw std::runtime_error("postgres did not become ready in time");
  }

  void write_backend_config() const {
    // Start from the project's own config.json rather than assembling one from
    // scratch: drogon reads a number of optional keys (plugins, custom_config,
    // app defaults, ...) and hand-rolling the document is easy to get subtly
    // wrong in a way that only shows up as a Json::LogicError at startup.
    // Only the connection details and the test-relevant values are overridden.
    nlohmann::json cfg;
    {
      std::ifstream in(fmt::format("{}/config.json", repo_root_));
      if (!in) {
        throw std::runtime_error("cannot read " + repo_root_ + "/config.json");
      }
      in >> cfg;
    }

    auto& listener = cfg["listeners"][0];
    listener["address"] = "127.0.0.1";
    listener["port"] = 8848;
    listener["https"] = false;

    auto& db = cfg["db_clients"][0];
    db["name"] = "default";
    db["rdbms"] = "postgresql";
    db["host"] = sock_dir_;  // unix socket directory of the scratch cluster
    db["port"] = pg_port_;
    db["dbname"] = "vote";
    db["user"] = "postgres";
    db["passwd"] = "";
    db["is_fast"] = false;
    db["number_of_connections"] = 4;
    db["timeout"] = -1.0;
    db["auto_batch"] = false;

    cfg["app"]["document_root"] = fmt::format("{}/static", scratch_);

    // stats_min_answers must be 1: the suite seeds only a handful of answers
    // per question, so the statistics endpoints would otherwise answer
    // insufficient_data. age_bucket_size 10 is hardcoded by several tests.
    cfg["stats_min_answers"] = 1;
    cfg["age_bucket_size"] = 10;
    cfg["password_reset_max_attempts"] = 50;
    cfg["password_reset_rate_limit_window_minutes"] = 15;
    cfg["password_reset_rate_limit_max_requests"] = 50;

    run(fmt::format("mkdir -p {}/static", scratch_));
    std::ofstream out(fmt::format("{}/config.json", conf_dir_));
    if (!out) {
      throw std::runtime_error("cannot write " + conf_dir_ + "/config.json");
    }
    out << cfg.dump(2);
    if (!out) {
      throw std::runtime_error("failed while writing config.json");
    }
  }

  void start_backend() {
    const std::string bin = locate_backend_binary();
    const std::string log = fmt::format("{}/backend.log", scratch_);

    // Fork + exec rather than system("... &") so the child is not reaped by an
    // intermediate shell and we keep a pid we can actually signal in the
    // destructor.
    const pid_t pid = ::fork();
    if (pid < 0) {
      throw std::runtime_error("fork failed while starting the backend");
    }
    if (pid == 0) {
      // Child: point the backend at our config and hand stderr to the log.
      ::setenv("VOTE_BACKEND_CONFPATH", conf_dir_.c_str(), 1);
      FILE* f = std::fopen(log.c_str(), "a");
      if (f != nullptr) {
        ::dup2(::fileno(f), STDOUT_FILENO);
        ::dup2(::fileno(f), STDERR_FILENO);
      }
      ::execl(bin.c_str(), bin.c_str(), static_cast<char*>(nullptr));
      ::_exit(127);  // exec failed
    }
    backend_pid_ = pid;
  }

  std::string locate_backend_binary() const {
    const std::string from_env = get_env("VOTE_BACKEND_MAIN_BIN", "");
    if (!from_env.empty()) {
      if (::access(from_env.c_str(), X_OK) != 0) {
        throw std::runtime_error("VOTE_BACKEND_MAIN_BIN is not executable: " +
                                 from_env);
      }
      return from_env;
    }
    for (const char* rel : {"builds/ninja-multi-vcpkg/Debug/main",
                            "builds/ninja-multi-vcpkg/Release/main",
                            "builds/ninja-multi-vcpkg/Debug/main"}) {
      const std::string cand = fmt::format("{}/{}", repo_root_, rel);
      if (::access(cand.c_str(), X_OK) == 0) return cand;
    }
    throw std::runtime_error(
        "backend binary not found. Build it, or set VOTE_BACKEND_MAIN_BIN.");
  }

  std::string repo_root_;
  std::string scratch_;
  std::string pgdata_;
  std::string conf_dir_;
  std::string sock_dir_;
  int pg_port_ = 55432;
  bool keep_ = false;
  pid_t backend_pid_ = -1;
};

// ---------------------------------------------------------------------------
// Convenience macro for simple endpoint tests
// ---------------------------------------------------------------------------
#define ENDPOINT_TEST(NAME, METHOD, URL, EXPECTED_STATUS, EXPECTED_JSON)    \
  TEST_CASE(#NAME) {                                                        \
    auto resp = test_helpers::http_request(METHOD, "127.0.0.1", 8848, URL); \
    CHECK(resp.status == EXPECTED_STATUS);                                  \
    CHECK(resp.json_body == nlohmann::json(EXPECTED_JSON));                 \
  }

}  // namespace test_helpers

// ---------------------------------------------------------------------------
// Global fixture – declared here, defined in integration_test.cpp
// ---------------------------------------------------------------------------
struct GlobalTestFixture {
  std::string access_token;
  GlobalTestFixture();
  ~GlobalTestFixture();
};

extern GlobalTestFixture global_fixture;

// ---------------------------------------------------------------------------
// Convenience helper: resolve a seed user's UUID by username
// ---------------------------------------------------------------------------
namespace test_helpers {
inline std::string get_userid(const std::string& username) {
  // Query the admin list endpoint using the global fixture's access token.
  auto resp = http_request("GET", "127.0.0.1", 8848, "/admin/users", "",
                           "application/json", global_fixture.access_token);
  if (resp.status != 200 || !resp.json_body.is_array()) {
    throw std::runtime_error(
        "Failed to list users for get_userid lookup: status " +
        std::to_string(resp.status) + ", body: " + resp.raw_body);
  }
  for (const auto& user : resp.json_body) {
    if (user.contains("username") &&
        user["username"].get<std::string>() == username) {
      return user["id"].get<std::string>();
    }
  }
  throw std::runtime_error("User not found in get_userid lookup: " + username);
}
}  // namespace test_helpers
