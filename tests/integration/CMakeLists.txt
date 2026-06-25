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
)

set_property(TARGET integration_test PROPERTY CXX_STANDARD 23)

add_test(NAME integration COMMAND $<TARGET_FILE:integration_test>)
set_property(TEST integration PROPERTY ENVIRONMENT "VOTE_BACKEND_SRC=${CMAKE_SOURCE_DIR}")
