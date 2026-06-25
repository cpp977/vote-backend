#pragma once

#include <doctest/doctest.h>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <nlohmann/json.hpp>

#include <chrono>
#include <thread>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
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
    std::string inventory = " -i " + std::string(get_env("VOTE_BACKEND_SRC", ".")) + "/ansible/inventory/hosts.yml";
    std::string cmd = env_setup + "ansible-playbook " + playbook + inventory;
    int rc = std::system(cmd.c_str());
    if (rc != 0) {
        throw std::runtime_error("Ansible playbook failed: " + playbook);
    }
}

inline void wait_for_http(const std::string& host, unsigned short port,
                          std::chrono::seconds timeout =
                              std::chrono::seconds{30}) {
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

inline HttpResponse http_request(const std::string& method,
                                 const std::string& host,
                                 unsigned short port,
                                 const std::string& target,
                                 const std::string& body = "",
                                 const std::string& content_type =
                                     "application/json",
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
    req.method(method == "POST" ? http::verb::post : http::verb::get);
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
inline std::string
authenticate(const std::string& host, unsigned short port,
              const std::string& username = "testuser",
              const std::string& email = "test@example.com",
              const std::string& password = "password123") {
    // 1. Register (tolerate 409 – user may already exist).
    nlohmann::json reg_payload = {{"username", username},
                                  {"email", email},
                                  {"password", password}};
    auto reg_resp = http_request("POST", host, port, "/register",
                                  reg_payload.dump());
    if (reg_resp.status != 201 && reg_resp.status != 409) {
        throw std::runtime_error(
            "Registration failed with status " +
            std::to_string(reg_resp.status) + ": " + reg_resp.raw_body);
    }

    // 2. Login – must succeed.
    nlohmann::json login_payload = {{"username", username},
                                     {"password", password}};
    auto login_resp = http_request("POST", host, port, "/login",
                                    login_payload.dump());
    if (login_resp.status != 200) {
        throw std::runtime_error(
            "Login failed with status " + std::to_string(login_resp.status) +
            ": " + login_resp.raw_body);
    }

    std::string access_token =
        login_resp.json_body["access_token"].get<std::string>();
    if (access_token.empty()) {
        throw std::runtime_error("Login response missing access_token");
    }
    return access_token;
}

// ---------------------------------------------------------------------------
// Convenience macro for simple endpoint tests
// ---------------------------------------------------------------------------
#define ENDPOINT_TEST(NAME, METHOD, URL, EXPECTED_STATUS, EXPECTED_JSON)      \
    TEST_CASE(#NAME) {                                                        \
        auto resp = test_helpers::http_request(METHOD, "127.0.0.1", 8848,      \
                                               URL);                          \
        CHECK(resp.status == EXPECTED_STATUS);                                 \
        CHECK(resp.json_body == nlohmann::json(EXPECTED_JSON));                 \
    }

} // namespace test_helpers

// ---------------------------------------------------------------------------
// Global fixture – declared here, defined in integration_test.cpp
// ---------------------------------------------------------------------------
struct GlobalTestFixture {
    std::string access_token;
    GlobalTestFixture();
    ~GlobalTestFixture();
};

extern GlobalTestFixture global_fixture;
