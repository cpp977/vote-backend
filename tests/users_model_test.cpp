#include <doctest/doctest.h>
#include <string>
#include <memory>

// Simple test to verify doctest is working
TEST_CASE("basic sanity check")
{
    CHECK(1 + 1 == 2);
    CHECK(true);
}

TEST_CASE("string operations")
{
    std::string str = "Hello, World!";
    CHECK(str.length() == 13);
    CHECK(str.substr(0, 5) == "Hello");
}

TEST_CASE("smart pointer basics")
{
    auto ptr = std::make_shared<int>(42);
    CHECK(ptr != nullptr);
    CHECK(*ptr == 42);
}