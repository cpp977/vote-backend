# CMakeLists.txt
cmake_minimum_required(VERSION 4.2)
project(vote-backend VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Generate version.txt for external tooling (Ansible, etc.)
file(GENERATE OUTPUT "${CMAKE_SOURCE_DIR}/version.txt" CONTENT "${PROJECT_VERSION}\n")
if(NOT APPLE)
    # set(CMAKE_CXX_CLANG_TIDY clang-tidy)
endif()

# Find dependencies provided by vcpkg (via vcpkg.cmake)
find_package(Drogon CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(unofficial-argon2 CONFIG REQUIRED)
find_package(jwt-cpp CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
# Boost is needed for integration tests (Beast & system)
find_package(Boost COMPONENTS system REQUIRED)

set(
    VOTE_BACKEND_SOURCES
    src/controllers/AuthController.cpp
    src/controllers/CategoryController.cpp
    src/controllers/QuestionController.cpp
    src/controllers/UserController.cpp
    src/filters/JwtAuthFilter.cpp
    src/filters/AdminAuthFilter.cpp
    src/models/AnswerOptions.cpp
    src/models/Categories.cpp
    src/models/Languages.cpp
    src/models/Questions.cpp
    src/models/UserAnswers.cpp
    src/models/Users.cpp
    src/utils/JwtService.cpp
    src/utils/UserIdHash.cpp
    src/utils/ErrorResponse.cpp
)

# main target
add_executable(main src/main.cpp ${VOTE_BACKEND_SOURCES})
target_link_libraries(main PRIVATE Drogon::Drogon fmt::fmt-header-only unofficial::argon2::libargon2 jwt-cpp::jwt-cpp OpenSSL::SSL OpenSSL::Crypto)
target_include_directories(main PRIVATE include)
set_property(TARGET main PROPERTY CXX_STANDARD 23)

# Testing configuration
enable_testing()
find_package(doctest CONFIG REQUIRED)

# Test sources (include source files needed for testing)
set(
    VOTE_BACKEND_TEST_SOURCES
    tests/main_test.cpp
    tests/users_model_test.cpp
    tests/jwt_service_test.cpp
    tests/jwt_auth_filter_test.cpp
    tests/admin_auth_filter_test.cpp
)

set(
    VOTE_BACKEND_INTEGRATION_TEST_SOURCES
    tests/integration/integration_test.cpp
    tests/integration/test_endpoints.cpp
)

# Test executable
add_executable(vote_backend_tests ${VOTE_BACKEND_TEST_SOURCES} ${VOTE_BACKEND_SOURCES})
target_link_libraries(vote_backend_tests PRIVATE
    doctest::doctest
    Drogon::Drogon
    jwt-cpp::jwt-cpp
    fmt::fmt-header-only
    unofficial::argon2::libargon2
)
target_include_directories(vote_backend_tests PRIVATE include)
set_property(TARGET vote_backend_tests PROPERTY CXX_STANDARD 23)

# Register unit tests with ctest
include(doctest)
doctest_discover_tests(vote_backend_tests
    PROPERTIES ENVIRONMENT "VOTE_BACKEND_CONFPATH=${CMAKE_SOURCE_DIR}/tests"
)

# Add integration test subdirectory
add_subdirectory(tests/integration)

add_custom_target(
    check-format
    COMMAND clang-format --dry-run ${VOTE_BACKEND_SOURCES} ${VOTE_BACKEND_TEST_SOURCES} ${VOTE_BACKEND_INTEGRATION_TEST_SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "checking format in src..."
)

add_custom_target(
    format
    COMMAND clang-format -i ${VOTE_BACKEND_SOURCES} ${VOTE_BACKEND_TEST_SOURCES} ${VOTE_BACKEND_INTEGRATION_TEST_SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "checking format in src..."
)
