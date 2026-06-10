# AGENTS.md

## Overview

This is a c++ RESTful backend which uses the drogon c++ library.
It uses a postgresql database as storage.

## Build, Lint & Test Commands
- **Configure** `CXX=clang++ cmake --preset ninja-multi-vcpkg`
- **Build (Debug)**: `CXX=clang++ cmake --build --preset ninja-vcpkg-debug`
- **Build (Release)**: `CXX=clang++ cmake --build --preset ninja-vcpkg-release`
- **Run all tests** (Debug): `CXX=clang++ ctest --preset test-debug`
- **Run a single test** (Debug): `CXX=clang++ ctest --preset test-debug -R <TestName>`
- **Run all tests** (Release): `CXX=clang++ ctest --preset test-release`
- **Run a single test** (Release): `CXX=clang++ ctest --preset test-release -R <TestName>`
- **Check formatting**: `CXX=clang++ cmake --workflow --preset check-format`
- **Auto‑format**: `CXX=clang++ cmake --workflow --preset check-format`

## Database

The postgresql database runs a container named `drogon-postgres`

Useful commands:

- **Check if running**: `podman ps --filter name=drogon-postgres --format "{{.Names}} {{.Status}}"`
- **Start postgresql container**: `podman run --name drogon-postgres --rm -e POSTGRES_PASSWORD=secret -e POSTGRES_DB=vote -v /home/peschke/code/private/Vote-Backend/sql:/docker-entrypoint-initdb.d -p 5432:5432 -d postgres:18
a69355ee5ca7b8cf7d591cca68faa57a6ac7d2deaee1caf83acf4ff0c5b`
- **Stop postgresql container**: `podman stop drogon-postgres`
- **Execute postgresql command**: `podman exec drogon-postgres psql -U postgres -d vote -c "<command>"`
- **Example command** (list all tables): `podman exec drogon-postgres psql -U postgres -d vote -c "SELECT * FROM information_schema.tables WHERE table_schema = 'public';"`
- **Example command** (list metadata of table `questions`): ` podman exec drogon-postgres psql -U postgres -d vote -c "\d questions"`

## Code Style Guidelines
- **Formatting**: Use `clang-format` (C++23) with the project's default style; run `CXX=clang++ cmake --workflow --preset check-format` before committing.
- **Imports**: Prefer angle‑bracket includes for external libs (`<drogon/...>`) and quotes for project headers (`"vote-backend/..."`).
- **Naming**:
  - Types & classes: `PascalCase` (e.g., `RestfulUserAnswersCtrl`).
  - Variables & functions: `snake_case`.
  - Constants: `UPPER_SNAKE_CASE`.
- **Types**: Enable `C++23`; avoid raw pointers, prefer `std::unique_ptr`/`std::shared_ptr` and `std::optional` where appropriate.
- **Error handling**: Throw exceptions for unrecoverable errors; use `try/catch` at request boundaries and return proper HTTP error codes.
- **Comments**: Doxygen‑compatible block comments for public APIs; line comments for implementation details.
- **Testing**: Write unit tests with GoogleTest; each test file mirrors the source file name with `_test.cpp` suffix.
- **Linting**: Run `clang-tidy <SOURCE>` for static analysis.
- **Commit**: Ensure `git diff --check` passes, format is clean, and all tests succeed before pushing.
