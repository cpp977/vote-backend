find_package(Boost COMPONENTS system REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(integration_test
    integration_test.cpp
    test_endpoints.cpp
)

target_link_libraries(integration_test
    PRIVATE
        Boost::headers
        Boost::system
        nlohmann_json::nlohmann_json
        doctest::doctest
        fmt::fmt-header-only
)

set_property(TARGET integration_test PROPERTY CXX_STANDARD 23)

add_test(NAME integration COMMAND $<TARGET_FILE:integration_test>)
# VOTE_BACKEND_SRC  : repo root, used to locate the playbook and sql/ scripts
# VOTE_BACKEND_MAIN_BIN : the backend binary, for the native (VOTE_BACKEND_NATIVE=1)
#                         path, where CTest's working directory is the build tree
set_tests_properties(integration PROPERTIES
    ENVIRONMENT
        "VOTE_BACKEND_SRC=${CMAKE_SOURCE_DIR};VOTE_BACKEND_MAIN_BIN=$<TARGET_FILE:main>"
)
