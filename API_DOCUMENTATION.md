# Vote Backend API Documentation

## Overview

This C++ RESTful backend uses the **Drogon** framework and provides a set of HTTP endpoints for managing users, authentication, and voting functionality. All responses are JSON‑encoded. Authentication is based on JWT access and refresh tokens.

## Configuration

The backend reads configuration from `config.json` (e.g., JWT secret, token expiry). Ensure the file is present at the repository root.

## Endpoints

| Method | Path | Description | Authentication | Request Body | Response |
|--------|------|-------------|----------------|--------------|----------|
| **POST** | `/register` | Register a new user. Password is hashed with Argon2id. `nationality`, when provided, must be an ISO 3166‑1 alpha‑2 country code that exists in the `countries` reference table (case‑insensitive; it is normalized to uppercase before storage) — any other value is rejected with *400*. `region`, when provided, must likewise be an ISO 3166‑2 subdivision code that exists in the `regions` reference table (e.g. `"DE-BE"`; case‑insensitive, normalized to uppercase). | No | ```json
{ "username": "string", "email": "string", "password": "string", "birth_year": 1990, "gender": "m", "nationality": "DE", "region": "DE-BE" }
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
| **PATCH** | `/me` | Update the authenticated user's own profile. Only `email`, `gender`, `password`, `nationality` and `region` are modifiable; `username` is the user's identity and is **never** modifiable. The nullable reference fields `nationality` and `region` additionally accept an explicit JSON `null` to clear them. | Bearer access token | JSON object with any subset of `email` (string), `gender` (`"m"`/`"w"`/`"d"`), `password` (string, min 8 chars), `nationality` (ISO 3166‑1 alpha‑2 code or `null`; unknown codes yield *400*), `region` (ISO 3166‑2 code or `null`; same rules). | *200 OK* – updated user object (never includes `password_hash`). Errors: 400 (invalid/missing field, `username` not modifiable, no fields provided), 401 (missing/invalid token), 404, 409 (email already in use), 500. |
| **PUT** | `/me` | Same as `PATCH /me` – update the authenticated user's own profile (partial update accepted). | Bearer access token | JSON object with any subset of `email`, `gender`, `password`, `nationality`, `region` (see `PATCH /me`). | *200 OK* – updated user object. Errors: 400, 401, 404, 409, 500. |
| **POST** | `/user/password/forgot` | Request a password-reset link. If an account exists for the given email and the caller is not rate-limited, a single-use token is generated, hashed (SHA-256), persisted in the `password_resets` table, and a reset link is sent via the configured email provider (Resend). Always returns the same generic response regardless of whether the email exists, preventing user enumeration. | No | ```json
{ "email": "user@example.com" }
``` | *200 OK* – `{ "message": "If an account with that email exists, a password reset link has been sent." }`. Errors: 400 (missing/invalid `email`). |
| **POST** | `/user/password/reset` | Consume a password-reset token together with a new password. The supplied token is SHA-256 hashed and looked up in the `password_resets` table. On success, the user's `password_hash` is updated, `password_changed_at` is set to `NOW()`, the token is marked `used = TRUE`, and all of the user's refresh tokens are revoked — all inside a single database transaction. On failure, the token's `attempt_count` is incremented and, after a configurable number of failures, the token is killed. | No | ```json
{ "token": "reset-token-string", "password": "newpassword123" }
``` | *200 OK* – `{ "message": "Password reset successfully" }`. Errors: 400 (missing/invalid `token`, `password` too short (<8 chars), invalid/expired/used token, max attempts exceeded), 500 (DB error). |
| **GET** | `/admin/users` | List all users (admin-only). Returns usernames and IDs of all users in the system. | Bearer access token (admin required) | – | *200 OK* – JSON array of user objects with `id` and `username` fields. Errors: 401 (unauthorized), 403 (admin required), 500 (DB error). |
| **GET** | `/admin/users/{id}` | Get detailed information about a specific user (admin-only). Returns all user fields except `password_hash`. | Bearer access token (admin required) | – | *200 OK* – JSON user object with user details. Errors: 400 (invalid ID format), 401 (unauthorized), 403 (admin required), 404 (user not found), 500 (DB error). |
| **POST** | `/admin/users/{id}/inactive` | Deactivate a user account (admin-only). Sets the user's `is_active` flag to false and updates the `updated_at` timestamp. | Bearer access token (admin required) | – | *200 OK* – JSON object confirming deactivation with `id`, `is_active`, and `message`. Errors: 400 (invalid ID format), 401 (unauthorized), 403 (admin required), 404 (user not found), 500 (DB error). |
| **POST** | `/admin/users/{id}/active` | Activate a user account (admin-only). Sets the user's `is_active` flag to true and updates the `updated_at` timestamp. | Bearer access token (admin required) | – | *200 OK* – JSON object confirming activation with `id`, `is_active`, and `message`. Errors: 400 (invalid ID format), 401 (unauthorized), 403 (admin required), 404 (user not found), 500 (DB error). |
| **GET** | `/admin/questions/submissions` | List the submission review queue (admin-only). Returns all questions whose `submission_status` is not `'approved'` (i.e. `pending` or `rejected`), newest first. Optionally filter with `?status=pending` or `?status=rejected`. | Bearer access token (admin required) | – | *200 OK* – JSON array of question objects (`id`, `text`, `category_id`, `language`, `min_age`, `created_at`, `special_category`, `submission_status`, `submitted_by`, `reviewed_by`). Errors: 401, 403, 500. |
| **POST** | `/admin/questions/{id}/approve` | Approve a pending submission (admin-only). Sets `submission_status` to `'approved'` and records `reviewed_by`/`reviewed_at`. Accepts an **optional** JSON body: `{ "min_age": 18, "special_category": "health" }`. The submitting user no longer chooses a minimum age, so the approving admin sets it here (`min_age`, integer 0–120, default 0) together with the GDPR special category flag (`special_category`, one of the enum labels, default `'none'`). Invalid values yield *400* without changing the submission. | Bearer access token (admin required) | ```json
{ "min_age": 18, "special_category": "health" }
``` | *200 OK* – the updated question object. *404* if the question does not exist. Errors: *400* (invalid `min_age`/`special_category`), *401*, *403*, *500*. |
| **POST** | `/admin/questions/{id}/reject` | Reject a pending submission (admin-only). Sets `submission_status` to `'rejected'` and records `reviewed_by`/`reviewed_at`. | Bearer access token (admin required) | – | *200 OK* – the updated question object. *404* if the question does not exist. |
| **GET** | `/admin/questions/{id}/answers` | Retrieve answer options for any question, regardless of submission status (admin-only). Exposes the options of pending/rejected submissions that are hidden from the public `/questions/{id}/answers` endpoint. | Bearer access token (admin required) | – | *200 OK* – JSON array of answer option objects (`id`, `question_id`, `text`). *404* if the question does not exist. Errors: 401, 403, 500. |
| **PATCH** | `/admin/questions/{id}/change` | Update the `text` field of an existing question (admin-only). Only the `text` column is modified; `submission_status`, `category_id`, and all other fields remain unchanged. Dependent rows (answer options, user answers) are left untouched. | Bearer access token (admin required) | ```json
{ "text": "Updated question text" }
``` | *200 OK* – `{ "id": 1, "text": "Updated question text", "submission_status": "approved", "message": "Question text updated" }`. Errors: *400* (missing/invalid JSON body, `text` field is missing/empty), *401* (unauthorized), *403* (admin required), *404* (question not found), *500* (DB error). |
| **POST** | `/admin/questions/{id}/delete` | Permanently delete a question (admin-only). All dependent rows — `answer_options`, `user_answers`, and `question_user` — are removed automatically via `ON DELETE CASCADE` foreign-key rules. This operation is irreversible. | Bearer access token (admin required) | – | *200 OK* – `{ "id": 1, "text": "...", "message": "Question deleted" }`. *404* if the question does not exist. Errors: 401 (unauthorized), 403 (admin required), 500 (DB error). |
| **GET** | `/questions` | Retrieve all voting questions. Each question object includes `special_category` (the GDPR Art. 9 flag, `'none'` for regular questions) so clients can request consent before answering. | Bearer access token | – | *200 OK* – array of question objects. |
| **GET** | `/questions/{id}` | Retrieve a single question by its id, including its `special_category` flag (`'none'` for regular questions). | Bearer access token | – | *200 OK* – question object or 404. |
| **POST** | `/questions/submissions` | Submit a new question **together with its answer options** for review. The question and all its `answer_options` are inserted atomically inside a single transaction, so a failure never leaves a partial record. Creates a *pending* submission owned by the caller; an admin must approve it (`POST /admin/questions/{id}/approve`) before it becomes publicly visible. | Bearer access token | ```json
{ "text": "string", "category_id": 1, "language": "en", "min_age": 0, "answer_options": [ "Option A", "Option B", { "text": "Option C" } ] }
``` | *201 Created* – created submission (`submission_status: "pending"`) plus its `answer_options` array (`[{ "id": <int>, "question_id": <int>, "text": "string" }, ...]`). `answer_options` is **required** and must contain at least one entry (max 50); each entry is a string or an object with a non‑empty `text`. Errors: 400 (missing/invalid fields, or missing/empty/too many `answer_options`), 401 (unauthenticated), 500 (DB error). |
| **POST** | `/questions/{id}/answers` | Submit an answer for a question. | Bearer access token | ```json
{ "answer": "string" }
``` | *201 Created* – answer record. |
| **POST** | `/questions/{id}/answer` | Submit an answer for a question. Enforces that a user may answer a question **only once**: the server records an anonymous, non‑reversible hash of the user id (never the raw id) together with the `question_id`, and a duplicate attempt is rejected. The insert is performed inside a transaction, so a failure never leaves a partial record. **Tags are not accepted from the client**: they are derived server-side from the authenticated user's profile (`gender`, `nationality`, `region`) in the `users` table, and the profile's `birth_year` is normalized into an anonymized age-range bucket (`age_bucket`, e.g. `"20-29"`) whose width is configurable via `age_bucket_size` in `config.json` (default 10). The raw birth year is never stored. Questions flagged with a `special_category` other than `'none'` additionally require explicit user consent: the optional boolean parameter `special_category_consent` must be `true`, otherwise the request is rejected with *400*; on success the consent timestamp is recorded. For regular questions the parameter is ignored entirely. | Bearer access token | ```json
{ "answer_id": 1, "special_category_consent": true }
``` | *201 Created* – `{ "id": <int>, "question_id": <int>, "answer_id": <int> }`. Errors: 400 (missing/invalid `answer_id`, `special_category_consent` not a boolean, `answer_id` does not belong to this question, or consent required but not given), 401 (unauthenticated), 409 (user already answered this question), 500 (DB error). |
| **GET** | `/questions/{id}/results` | Get aggregated results for a question. | Bearer access token | – | *200 OK* – tally per answer. |
| **POST** | `/questions/restSearch` | Search/filter questions via a JSON body. Filters: `language` (exact match), `search` (case‑insensitive substring on the question text), `categoryIds` (match any of the given category ids), `age` (question `min_age` >= value). Supports pagination via `offset` (default 0) and `limit` (default 50, max 1000). | No | ```json
{ "language": "string", "search": "string", "categoryIds": [1, 2, 3], "age": 0, "offset": 0, "limit": 50 }
``` | *200 OK* – array of question objects (`id`, `text`, `language`, `category_id`, `category_name`, `special_category`). `special_category` is the GDPR Art. 9 flag (`'none'` for regular questions) so clients can request consent before answering. Errors: 400 (invalid JSON body), 500 (DB error). |
| **GET** | `/categories/lang/{lang}` | Retrieve all categories for a given language code (e.g. `en`, `de`). Returns only categories whose `language` column matches the path parameter. | No | – | *200 OK* – array of category objects (`id`, `name`, `language`). Returns an empty array for an unknown language code. |
| **GET** | `/questions/{id}/stats` | Get aggregated results (vote counts per answer option) for a question. Optionally filter by tag via individual query parameters — each is optional and maps to one of the tags derived from the user profile at answer time: `gender` (`"m"`, `"w"`, `"d"`), `nationality` (ISO 3166‑1 alpha‑2 country code, case‑insensitive and normalized to uppercase), `region` (ISO 3166‑2 subdivision code, case‑insensitive and normalized to uppercase) and `age_bucket` (`"<start>-<end>"`, e.g. `?gender=m&age_bucket=20-29`; bucket width defaults to 10 years and is configurable via `age_bucket_size` in config.json); multiple parameters combine with AND semantics using jsonb containment. Unknown query parameters are ignored; empty values are treated as absent; invalid values yield *400*. For privacy, statistics are only returned when at least `stats_min_answers` matching answers exist (config.json, default 5). Clients should discover the available dimensions and values from `GET /stats/meta`. | No | – | *200 OK* – envelope object: `{ "status": "ok", "message": "", "answers": [ { "answer_id": 1, "answer_text": "...", "count": 2, "percent": 66.67 } ] }`. When fewer than `stats_min_answers` matching answers exist: `{ "status": "insufficient_data", "message": "Not enough responses to display statistics for this filter.", "answers": [] }`. Errors: 400 (invalid tag value), 500 (DB error). |
| **GET** | `/stats/meta` | Describes the statistics tag contract so clients can render filter UIs without hardcoding it: `age_bucket_size` (configured bucket width in years), `min_answers` (privacy threshold), and `dimensions` — the allowed wire values per dimension. `gender` values are fixed (`m`/`w`/`d`), `age_bucket` labels are generated from the configured bucket width, and `nationality` / `region` list the normalized codes actually present among users with at least `min_answers` occurrences (so guaranteed-viable segments only). | No | – | *200 OK* – `{ "age_bucket_size": 10, "min_answers": 5, "dimensions": [ { "key": "gender", "values": ["m","w","d"] }, { "key": "age_bucket", "values": ["0-9","10-19","..."] }, { "key": "nationality", "values": ["AT","DE","..."] }, { "key": "region", "values": ["DE-BE","US-CA","..."] } ] }`. Errors: 500 (DB error). |
| **GET** | `/questions/{id}/answers` | Get the answer options for a question. Only approved questions are visible. | No | – | *200 OK* – array of answer option objects (`id`, `question_id`, `text`). Errors: 404 (question not found), 500 (DB error). |
| **GET** | `/questions/{id}/answers-with-auth` | Get the answer options for a question. Returns options for approved questions, and also allows the question owner to see options for their own pending submissions. | Bearer access token | – | *200 OK* – array of answer option objects (`id`, `question_id`, `text`). Errors: 401 (unauthorized), 404 (question not found or not visible to you), 500 (DB error). |

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