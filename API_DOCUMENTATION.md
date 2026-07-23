# Vote Backend API Documentation

## Overview

This C++ RESTful backend uses the **Drogon** framework and provides a set of HTTP endpoints for managing users, authentication, and voting functionality. All responses are JSON‑encoded. Authentication is based on JWT access and refresh tokens.

## Configuration

The backend reads configuration from `config.json` (e.g., JWT secret, token expiry). Ensure the file is present at the repository root.

## Endpoints

| Method | Path | Description | Authentication | Request Body | Response |
|--------|------|-------------|----------------|--------------|----------|
| **POST** | `/register` | Register a new user. Password is hashed with Argon2id. | No | ```json
{ "username": "string", "email": "string", "password": "string", "birth_year": 1990, "gender": "m", "nationality": "US" }
``` | *201 Created* – JSON with user id. Errors: 400 (invalid), 409 (user exists). |
| **POST** | `/login` | Authenticate a user and issue JWT access + refresh tokens. | No | ```json
{ "username": "string", "password": "string" }
``` | *200 OK* – `{ "access_token": "...", "refresh_token": "..." }`. Errors: 400, 401. |
| **POST** | `/logout` | Revoke a refresh token (adds its hash to the blocklist). | Bearer access token | ```json
{ "refresh_token": "string" }
``` | *200 OK* – `{ "message": "Logged out" }`. Errors: 400, 401. |
| **POST** | `/refresh` | Exchange a valid refresh token for a new token pair. | No (refresh token in body) | ```json
{ "refresh_token": "string" }
``` | *200 OK* – new `{ "access_token": "...", "refresh_token": "..." }`. Errors: 400, 401. |
| **PATCH** | `/me` | Update the authenticated user's own profile. Only `email`, `gender` and `password` are modifiable; `username` is the user's identity and is **never** modifiable. | Bearer access token | JSON object with any subset of `email` (string), `gender` (`"m"`/`"w"`/`"d"`), `password` (string, min 8 chars). | *200 OK* – updated user object (never includes `password_hash`). Errors: 400 (invalid/missing field, `username` not modifiable, no fields provided), 401 (missing/invalid token), 404, 409 (email already in use), 500. |
| **PUT** | `/me` | Same as `PATCH /me` – update the authenticated user's own profile (partial update accepted). | Bearer access token | JSON object with any subset of `email`, `gender`, `password` (see `PATCH /me`). | *200 OK* – updated user object. Errors: 400, 401, 404, 409, 500. |
| **GET** | `/admin/users` | List all users (admin-only). Returns usernames and IDs of all users in the system. | Bearer access token (admin required) | – | *200 OK* – JSON array of user objects with `id` and `username` fields. Errors: 401 (unauthorized), 403 (admin required), 500 (DB error). |
| **GET** | `/admin/users/{id}` | Get detailed information about a specific user (admin-only). Returns all user fields except `password_hash`. | Bearer access token (admin required) | – | *200 OK* – JSON user object with user details. Errors: 400 (invalid ID format), 401 (unauthorized), 403 (admin required), 404 (user not found), 500 (DB error). |
| **POST** | `/admin/users/{id}/inactive` | Deactivate a user account (admin-only). Sets the user's `is_active` flag to false and updates the `updated_at` timestamp. | Bearer access token (admin required) | – | *200 OK* – JSON object confirming deactivation with `id`, `is_active`, and `message`. Errors: 400 (invalid ID format), 401 (unauthorized), 403 (admin required), 404 (user not found), 500 (DB error). |
| **POST** | `/admin/users/{id}/active` | Activate a user account (admin-only). Sets the user's `is_active` flag to true and updates the `updated_at` timestamp. | Bearer access token (admin required) | – | *200 OK* – JSON object confirming activation with `id`, `is_active`, and `message`. Errors: 400 (invalid ID format), 401 (unauthorized), 403 (admin required), 404 (user not found), 500 (DB error). |
| **GET** | `/questions` | Retrieve all voting questions. | Bearer access token | – | *200 OK* – array of question objects. |
| **GET** | `/questions/{id}` | Retrieve a single question by its UUID. | Bearer access token | – | *200 OK* – question object or 404. |
| **POST** | `/questions/submissions` | Submit a new question **together with its answer options** for review. The question and all its `answer_options` are inserted atomically inside a single transaction, so a failure never leaves a partial record. Creates a *pending* submission owned by the caller; an admin must approve it (`POST /admin/questions/{id}/approve`) before it becomes publicly visible. | Bearer access token | ```json
{ "text": "string", "category_id": 1, "language": "en", "min_age": 0, "answer_options": [ "Option A", "Option B", { "text": "Option C" } ] }
``` | *201 Created* – created submission (`submission_status: "pending"`) plus its `answer_options` array (`[{ "id": <int>, "question_id": <int>, "text": "string" }, ...]`). `answer_options` is **required** and must contain at least one entry (max 50); each entry is a string or an object with a non‑empty `text`. Errors: 400 (missing/invalid fields, or missing/empty/too many `answer_options`), 401 (unauthenticated), 500 (DB error). |
| **POST** | `/questions/{id}/answers` | Submit an answer for a question. | Bearer access token | ```json
{ "answer": "string" }
``` | *201 Created* – answer record. |
| **POST** | `/questions/{id}/answer` | Submit an answer for a question. Enforces that a user may answer a question **only once**: the server records an anonymous, non‑reversible hash of the user id (never the raw id) together with the `question_id`, and a duplicate attempt is rejected. The insert is performed inside a transaction, so a failure never leaves a partial record. | Bearer access token | ```json
{ "answer_id": 1, "tags": { "gender": "m", "age": 30 } }
``` | *201 Created* – `{ "id": <int>, "question_id": <int>, "answer_id": <int> }`. Errors: 400 (missing/invalid `answer_id`, or `answer_id` does not belong to this question), 409 (user already answered this question), 500 (DB error). |
| **GET** | `/questions/{id}/results` | Get aggregated results for a question. | Bearer access token | – | *200 OK* – tally per answer. |
| **POST** | `/questions/restSearch` | Search/filter questions via a JSON body. Filters: `language` (exact match), `search` (case‑insensitive substring on the question text), `categoryIds` (match any of the given category ids), `age` (question `min_age` >= value). Supports pagination via `offset` (default 0) and `limit` (default 50, max 1000). | Bearer access token | ```json
{ "language": "string", "search": "string", "categoryIds": [1, 2, 3], "age": 0, "offset": 0, "limit": 50 }
``` | *200 OK* – array of question objects (`id`, `text`, `language`, `category_id`, `category_name`). Errors: 400 (invalid JSON body), 500 (DB error). |
| **GET** | `/categories/lang/{lang}` | Retrieve all categories for a given language code (e.g. `en`, `de`). Returns only categories whose `language` column matches the path parameter. | Bearer access token | – | *200 OK* – array of category objects (`id`, `name`, `language`). Returns an empty array for an unknown language code. |

## Authentication Details

- **Access Token**: Short‑lived JWT (default 15 min) used in the `Authorization: Bearer <token>` header.
- **Refresh Token**: Long‑lived JWT (default 7 days). Stored only as a SHA‑256 hash in the DB; the raw token is never persisted.
- Tokens are generated by `vote_backend::utils::JwtService` (see `src/utils/JwtService.h`).

## Error Handling

All error responses follow the format:

```json
{ "error": "Human‑readable message" }
```

Appropriate HTTP status codes are used (`400`, `401`, `403`, `404`, `409`, `500`).

## Project Structure

- **src/controllers** – HTTP endpoint implementations (e.g., `AuthController.cpp`).
- **src/models** – Database ORM models generated by Drogon (`vote_model::vote::*`).
- **src/utils** – Helper utilities such as `JwtService` and hashing functions.
- **config.json** – Runtime configuration (JWT secret, expiry, DB connection).
- **sql/** – Database schema and migration scripts.

## Building & Testing

```bash
# Configure
CXX=clang++ cmake --preset ninja-multi-vcpkg
# Build Debug
CXX=clang++ cmake --build --preset ninja-vcpkg-debug
# Run tests
CXX=clang++ ctest --preset test-debug
```

Ensure the PostgreSQL container (`drogon-postgres`) is running before starting the service.

---
*Generated by ECA.*