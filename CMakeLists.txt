# CMakeLists.txt
cmake_minimum_required(VERSION 4.2)
project(vote-backend)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT APPLE)
    # set(CMAKE_CXX_CLANG_TIDY clang-tidy)
endif()

# Find dependencies provided by vcpkg (via vcpkg.cmake)
find_package(Drogon CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(unofficial-argon2 CONFIG REQUIRED)
find_package(jwt-cpp CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

set(
    VOTE_BACKEND_SOURCES
    src/main.cpp
    include/vote-backend/controllers/RestfulUserAnswersCtrlBase.cc
    include/vote-backend/controllers/RestfulUserAnswersCtrl.cc
    include/vote-backend/controllers/RestfulQuestionsCtrlBase.cc
    include/vote-backend/controllers/RestfulQuestionsCtrl.cc
    include/vote-backend/controllers/RestfulAnswerOptionsCtrlBase.cc
    include/vote-backend/controllers/RestfulAnswerOptionsCtrl.cc
    include/vote-backend/controllers/RestfulCategoriesCtrlBase.cc
    include/vote-backend/controllers/RestfulCategoriesCtrl.cc
    include/vote-backend/controllers/QuestionController.cc
    include/vote-backend/models/UserAnswers.cc
    include/vote-backend/models/Questions.cc
    include/vote-backend/models/AnswerOptions.cc
    include/vote-backend/models/Languages.cc
    include/vote-backend/models/Categories.cc
    include/vote-backend/models/Users.cc
    src/controllers/AuthController.cc
    src/utils/JwtService.cc
    src/filters/JwtAuthFilter.cc
)

# main target
add_executable(main ${VOTE_BACKEND_SOURCES})
target_link_libraries(main PRIVATE Drogon::Drogon fmt::fmt-header-only unofficial::argon2::libargon2 jwt-cpp::jwt-cpp OpenSSL::SSL OpenSSL::Crypto)
target_include_directories(main PRIVATE include)
set_property(TARGET main PROPERTY CXX_STANDARD 23)

add_custom_target(
    check-format
    COMMAND clang-format --dry-run ${VOTE_BACKEND_SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "checking format in src..."
)

add_custom_target(
    format
    COMMAND clang-format -i ${VOTE_BACKEND_SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "checking format in src..."
)
