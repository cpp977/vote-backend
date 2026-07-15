# AGENTS.md

## Overview

This is a c++ RESTful backend which uses the drogon c++ library.
It uses a postgresql database as storage.
Both the backend and the database are running in rootless podman containers using quadlets and systemd for orchestraction.

## Ansible Playbooks

Three Ansible playbooks automate the main workflows. All target the `dev` group (localhost).

### Build & Test (native)

`ansible-playbook ansible/playbooks/build-and-test.yml`

This playbook handles the full native build and test pipeline:
1. Installs build prerequisites (clang, cmake, ninja, dev libraries) if missing
2. Configures the project with CMake (`ninja-multi-vcpkg` preset)
3. Builds both **Debug** and **Release** configurations
4. Runs all unit tests via ctest (Debug)
5. Checks code formatting with `clang-format`

### Deploy locally (persistent service)

`ansible-playbook ansible/playbooks/deploy-local.yml`

This playbook deploys the application into rootless Podman containers via quadlets for
running it as a real service. In addition to the database and backend it also stands up
an **nginx reverse proxy** that terminates TLS, so the backend is reachable over HTTPS:
- The PostgreSQL data directory is bind-mounted from `~/.local/share/vote-backend/postgres/18`
  (the `/18` suffix is required by PostgreSQL >= 18), so the database survives container restarts.
- A **self-signed TLS certificate** is generated locally by the role and stored under
  `~/.config/vote-backend/tls` (outside the git repository). Drop real CA-signed
  `fullchain.pem`/`privkey.pem` files there to replace it.
- nginx listens on host port **8443** (rootless) and proxies to the backend on the
  `vote.network`. The certificate is issued for `vote-backend.local`; add
  `vote-backend.local <host-ip>` to `/etc/hosts` so the name resolves. (A later `socat`
  forward from 443 → 8443 is not configured yet.)

Steps:
1. Builds the `Containerfile` image
2. Generates the self-signed certificate (persistent deployment only)
3. Installs quadlets (network, postgres, app, nginx) into the user systemd instance
4. Reloads the user systemd daemon
5. Starts and enables the database, backend, and nginx services

**Typical workflow**: run **build-and-test** first to validate changes natively, then run
**deploy-local** to (re)deploy the containerized application as a persistent service.

### Deploy locally (integration testing)

`ansible-playbook ansible/playbooks/deploy-local-test.yml`

This playbook is the old `deploy-local` deployment, kept for integration tests. It deploys
an ephemeral setup: the PostgreSQL data directory is **not** bind-mounted, so the database
is recreated from the SQL init scripts on every (re)deploy:
1. Builds the `Containerfile` image
2. Installs quadlets (network, postgres, app) into the user systemd instance
3. Reloads the user systemd daemon
4. Starts and enables the database and backend services

Use this for running the integration test suite (`ctest --preset test-debug`); the database
state is reset on each (re)deploy.

## Build, Lint & Test Commands on native hosts
- **Configure** `CXX=clang++ cmake --preset ninja-multi-vcpkg`
- **Build (Debug)**: `CXX=clang++ cmake --build --preset ninja-vcpkg-debug`
- **Build (Release)**: `CXX=clang++ cmake --build --preset ninja-vcpkg-release`
- **Run all tests** (Debug): `CXX=clang++ ctest --preset test-debug`
- **Run a single test** (Debug): `CXX=clang++ ctest --preset test-debug -R <TestName>`
- **Run all tests** (Release): `CXX=clang++ ctest --preset test-release`
- **Run a single test** (Release): `CXX=clang++ ctest --preset test-release -R <TestName>`
- **Check formatting**: `CXX=clang++ cmake --workflow --preset check-format`
- **Auto‑format**: `CXX=clang++ cmake --workflow --preset format`

## Version

The project version is defined **once** in `CMakeLists.txt` via `project(vote-backend VERSION ...)`.
CMake generates `version.txt` at configure time; the Ansible deployment and quadlet template
(`quadlets/vote-backend@.container.j2`) derive their image tag from it.

## Build container (compilation included)
- `CXX=clang++ cmake --preset ninja-multi-vcpkg` (generates `version.txt`)
- `podman build -f Containerfile --tag vote-backend:"$(cat version.txt)" .`

## Start/Stop the backend
- **Start** `systemctl --user start vote-backend@"$(cat /path/to/version.txt)".service`
- **Restart** `systemctl --user restart vote-backend@"$(cat /path/to/version.txt)".service`
- **Stop** `systemctl --user stop vote-backend@"$(cat /path/to/version.txt)".service`

## Database

The postgresql database runs in a container named `drogon-postgres`

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
- **Testing**: Write unit tests with doctest; each test file mirrors the source file name with `_test.cpp` suffix.
- **Linting**: Run `clang-tidy <SOURCE>` for static analysis.
- **Commit**: Ensure `git diff --check` passes, format is clean, and all tests succeed before pushing.
