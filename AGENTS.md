# AGENTS.md

## Overview

This is a c++ RESTful backend which uses the drogon c++ library.
It uses a postgresql database as storage.
Both the backend and the database are running in rootless podman containers using quadlets and systemd for orchestraction.

For the **integration tests** there is also a container-free path that runs PostgreSQL and the backend natively, see [Run integration tests without containers](#run-integration-tests-without-containers). Use it whenever rootless podman is unavailable, for example inside a hardened agent sandbox where every mount is `nosuid` and `/run` is read-only: there `newuidmap` cannot gain `CAP_SETUID`, and the quadlet units have no user session to attach to.

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
is recreated from `migrations/` (plus the `seed/` fixtures) on every (re)deploy:
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

## Run integration tests without containers

By default `ctest` runs the integration suite the long-standing way: the test
binary's fixture invokes `ansible/playbooks/deploy-local-test.yml`, which brings
up the podman/quadlet stack. CI depends on this and it is unchanged.

Where rootless podman is unavailable -- for example inside a hardened agent
sandbox where every mount is `nosuid` and `/run` is read-only, so `newuidmap`
cannot gain `CAP_SETUID` and the quadlet units have no user session -- the same
suite can run natively. Select it with the `test-native-*` presets:

```sh
CXX=clang++ cmake --build --preset ninja-vcpkg-debug
CXX=clang++ ctest --preset test-native-debug     # Debug
CXX=clang++ ctest --preset test-native-release   # Release
CXX=clang++ ctest --preset test-native-debug -R integration   # integration only
```

`VOTE_BACKEND_NATIVE=1` makes the fixture build the environment itself
(`NativeStack` in `tests/integration/integration_helpers.hpp`): a throwaway
PostgreSQL cluster via `initdb` in a `mktemp` directory, a `vote` database built
from `migrations/` and seeded from `seed/`, and the backend binary started
against it with `fork`/`exec`. Everything is torn down in the fixture
destructor. No wrapper script and no manual steps.

Prerequisites and gotchas:

- The PostgreSQL **server** binaries must be on `PATH` (`initdb`, `pg_ctl`,
  `psql`, `createdb`). The fixture checks and fails with a clear message.
- **The database is rebuilt on every run, and that is required.** The suite is
  order-dependent and mutates state (it registers users, answers questions,
  deactivates accounts), so reusing a database makes later runs fail. This
  mirrors `vote_backend_postgres_persistent: false` in the container path.
- The backend config is derived from the project's own `config.json` with the
  connection details overridden. Do not hand-assemble it: drogon reads several
  optional keys and a missing one surfaces only as a `Json::LogicError` at
  startup.
- `stats_min_answers` is forced to `1` and `age_bucket_size` to `10`; the
  statistics endpoints would otherwise answer `insufficient_data`, and several
  tests hardcode a bucket width of 10.
- Set `VOTE_BACKEND_NATIVE_KEEP=1` to keep the scratch directory (cluster,
  generated `config.json`, `backend.log`, `pg.log`) for inspection. On a
  startup failure the fixture already prints the backend log tail.
- If the build fails with `unable to read $HOME`, the environment is missing
  `HOME`; prefix the command with `HOME=/home/<user>`.

## Version

The project version is defined **once** in `CMakeLists.txt` via `project(vote-backend VERSION ...)`.
CMake generates `version.txt` at configure time; the Ansible deployment and quadlet template
(`quadlets/vote-backend@.container.j2`) derive their image tag from it. The same value is baked into
the binary as the `VOTE_BACKEND_VERSION` compile definition, which is what the unrestricted
`GET /version` endpoint reports — the container image ships no `version.txt`, so the endpoint must not
read one at runtime.

## Build container (compilation included)
- `CXX=clang++ cmake --preset ninja-multi-vcpkg` (generates `version.txt`)
- `podman build -f Containerfile --tag vote-backend:"$(cat version.txt)" .`

## Start/Stop the backend
- **Start** `systemctl --user start vote-backend@"$(cat /path/to/version.txt)".service`
- **Restart** `systemctl --user restart vote-backend@"$(cat /path/to/version.txt)".service`
- **Stop** `systemctl --user stop vote-backend@"$(cat /path/to/version.txt)".service`

## Database migrations

The schema is owned by **golang-migrate**. `migrations/` is the only definition
of it; nothing else creates tables. The PostgreSQL container deliberately has
nothing mounted into `/docker-entrypoint-initdb.d`, because init scripts only
run while a cluster is being created and therefore can never upgrade an existing
database.

```
migrations/
  000001_init.up.sql        / .down.sql    core schema (questions, answers, votes)
  000002_auth.up.sql        / .down.sql    users, tokens, countries reference data
  000005_regions.up.sql     / .down.sql    ISO 3166-2 reference data
seed/
  seed_data.sql                            demo categories/questions
  test_data.sql                            users and votes the tests log in with
```

- **migrations/** holds schema *and* the reference data the application cannot
  work without (`countries`, `regions`). It runs once per database, in version
  order, and each file plus its version bump is one transaction.
- **seed/** holds fixture data. It is not part of the schema, is applied by the
  integration-test playbook and the container-free test path only, and must
  never reach a persistent deployment (the seeded users have well-known
  passwords).

### Rules

1. A schema change is always a **new** migration. Never edit a migration that
   has been applied anywhere - fix it forward.
2. Write the `.down.sql` too, but treat deployed databases as forward-only: the
   rollback of a live deployment is a `pg_dump` restore, not `migrate down`.
3. Make migrations **expand/contract** so the currently running binary keeps
   working: add the nullable column first, backfill, drop the old one in a later
   migration.
4. New migrations are numbered sequentially:
   `migrate create -seq -ext sql -dir migrations add_<thing>`. Zero-padded
   prefixes keep a plain sort equal to the version order.

### Applying

The deploy playbooks do it for you: `roles/db-migrate` copies `migrations/` to
`~/.local/share/vote-backend/migrations`, runs the pinned `migrate/migrate`
container on the `vote` network as the `vote_migration` role, and restarts
`vote-backend` only when the version actually moved. To run just that step:

```sh
ansible-playbook ansible/playbooks/deploy-local.yml --tags migrate
```

Handy equivalents against the running container (note `-path` and that
golang-migrate's subcommand comes last):

```sh
# current version
podman run --rm --network vote \
  -v "$HOME/.local/share/vote-backend/migrations:/migrations:ro,Z" \
  docker.io/migrate/migrate:v4.19.0 \
  -path /migrations -database 'postgres://vote_migration:<pw>@drogon-postgres:5432/vote?sslmode=disable' version

# create a new migration
podman run --rm -v "$PWD:/src" docker.io/migrate/migrate:v4.19.0 \
  create -seq -ext sql -dir /src/migrations add_<thing>
```

`vote_backend_db_migration_password` is vaulted; use `--ask-vault-pass` and take
the password from the vault rather than pasting it into shell history.

### Databases that predate the framework

A cluster created from the old `sql/` init scripts has the schema but no
`schema_migrations` table. The db-migrate role detects that by asking the
database directly (marker tables, not the tool's exit status) and **baselines**
it at `vote_backend_migrate_baseline_version` (5 - the last migration those
scripts contained) by creating golang-migrate's own bookkeeping table
(`schema_migrations(version bigint PRIMARY KEY, dirty boolean)`) with that single
row, then applies only the newer migrations. Writing the row directly rather
than with `migrate force` is deliberate: `force` updates a table it expects to
exist, so it fails on a database that has never been tracked - which is exactly
the case it is meant for. Nothing is replayed into a populated database. Verify
the assumption before trusting it:

```sh
podman exec drogon-postgres psql -U postgres -d vote -tAc \
  "SELECT to_regclass('public.users') IS NOT NULL,
          EXISTS (SELECT 1 FROM regions WHERE code = 'DE-BE')"
```

Those init scripts ran as the cluster's bootstrap superuser, so every table on
such a database belongs to `postgres` -- and only the owner may `ALTER TABLE`.
The same adoption step therefore hands the objects in `public` (tables, and with
them their indexes and serial sequences, plus the enum types) over to the
migration role before the migrations run. It is idempotent: a database built by
the migration role has nothing to hand over.

A migration that fails leaves the database **dirty** (`schema_migrations.dirty =
t` at the version it attempted) and every later run is refused with "Dirty
database version N". Recovery is to fix the cause, clear the flag at the last
version that really is in the database, and re-run with `--tags migrate`:

```sh
podman exec -i drogon-postgres \
  psql -U postgres -d vote -v ON_ERROR_STOP=1 -c \
  "UPDATE schema_migrations SET dirty = false WHERE version = 5"
```

Check `SELECT version, dirty FROM schema_migrations` first: the version to keep
is the one that actually applied, which is not necessarily the version the failed
run recorded.

A fresh persistent deployment ends up with an empty database and no users: the
fixtures are not loaded there. Create the first account through
`POST /register`, then promote it:

```sh
podman exec drogon-postgres psql -U postgres -d vote \
  -c "UPDATE users SET is_admin = TRUE WHERE username = '<name>'"
```

## Database

The postgresql database runs in a container named `drogon-postgres`

Useful commands:

- **Check if running**: `podman ps --filter name=drogon-postgres --format "{{.Names}} {{.Status}}"`
- **Start postgresql container**: `podman run --name drogon-postgres --rm -e POSTGRES_PASSWORD=secret -e POSTGRES_DB=vote -p 5432:5432 -d postgres:18`
  (the schema is applied afterwards by `roles/db-migrate`, not by this command)
- **Stop postgresql container**: `podman stop drogon-postgres`
- **Execute postgresql command**: `podman exec drogon-postgres psql -U postgres -d vote -c "<command>"`
- **Example command** (list all tables): `podman exec drogon-postgres psql -U postgres -d vote -c "SELECT * FROM information_schema.tables WHERE table_schema = 'public';"`
- **Example command** (list metadata of table `questions`): ` podman exec drogon-postgres psql -U postgres -d vote -c "\d questions"`

## Code Style Guidelines
- **Formatting**: Use `clang-format` (C++23) with the project's default style; run `CXX=clang++ cmake --workflow --preset check-format` before committing.
- **Imports**: Prefer angle‑bracket includes for external libs (`<drogon/...>`) and quotes for project headers (`"vote-backend/..."`).
- **Naming**:
  - Types & classes: `PascalCase` (e.g., `QuestionController`).
  - Variables & functions: `snake_case`.
  - Constants: `UPPER_SNAKE_CASE`.
- **Types**: Enable `C++23`; avoid raw pointers, prefer `std::unique_ptr`/`std::shared_ptr` and `std::optional` where appropriate.
- **Error handling**: Throw exceptions for unrecoverable errors; use `try/catch` at request boundaries and return proper HTTP error codes.
- **Comments**: Doxygen‑compatible block comments for public APIs; line comments for implementation details.
- **Testing**: Write unit tests with doctest; each test file mirrors the source file name with `_test.cpp` suffix.
- **Linting**: Run `clang-tidy <SOURCE>` for static analysis.
- **Commit**: Ensure `git diff --check` passes, format is clean, and all tests succeed before pushing.
