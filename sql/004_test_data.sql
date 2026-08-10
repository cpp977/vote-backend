-- 004_test_data.sql
-- Integration test seed data: a single user and pre-defined answer submissions
-- so that GET /questions/{id}/stats returns deterministic results.

-- ---------------------------------------------------------------------------
-- Regular test user (password: 12345678, hashed with argon2id)
-- ---------------------------------------------------------------------------
INSERT INTO users (username, email, password_hash, birth_year, gender, nationality, is_active)
VALUES (
  'Jim',
  'jim@example.com',
  '$argon2id$v=19$m=65536,t=2,p=1$7IwaKiaTuSf8MV6JOC/InA$KPZSbQabGbDdhRz1JtzjWUk4wokot/5PebP8xmP/nzQ',
  1990,
  'm',
  'US',
  TRUE
);

-- ---------------------------------------------------------------------------
-- Admin test user (password: 12345678, hashed with argon2id).
-- Flagged as admin (is_admin = TRUE) so that admin-only endpoints can be
-- exercised end-to-end once they are added.
-- ---------------------------------------------------------------------------
INSERT INTO users (username, email, password_hash, birth_year, gender, nationality, is_admin)
VALUES (
  'Admin',
  'admin@example.com',
  '$argon2id$v=19$m=65536,t=2,p=1$7IwaKiaTuSf8MV6JOC/InA$KPZSbQabGbDdhRz1JtzjWUk4wokot/5PebP8xmP/nzQ',
  1985,
  'w',
  'US',
  TRUE
);

-- ---------------------------------------------------------------------------
-- Inactive test user (password: 12345678, hashed with argon2id)
-- Flagged as inactive so that authentication tests can verify the 423 response.
-- ---------------------------------------------------------------------------
INSERT INTO users (username, email, password_hash, birth_year, gender, nationality, is_active)
VALUES (
  'InactiveUser',
  'inactive@example.com',
  '$argon2id$v=19$m=65536,t=2,p=1$7IwaKiaTuSf8MV6JOC/InA$KPZSbQabGbDdhRz1JtzjWUk4wokot/5PebP8xmP/nzQ',
  1995,
  'w',
  'CA',
  FALSE
);

-- ---------------------------------------------------------------------------
-- User answers for question 1 ("How many bananas do you eat per week?")
--   answer_id=1 ("0")   -> 2 votes
--   answer_id=2 ("1-2") -> 1 vote
-- ---------------------------------------------------------------------------
INSERT INTO user_answers (question_id, answer_id, tags)
VALUES (1, 1, '{"gender": "m"}');

INSERT INTO user_answers (question_id, answer_id, tags)
VALUES (1, 1, '{"gender": "f"}');

INSERT INTO user_answers (question_id, answer_id, tags)
VALUES (1, 2, '{"gender": "m"}');

-- ---------------------------------------------------------------------------
-- User answers for question 3 ("Do you have an own car?")
--   answer_id=11 ("Yes")            -> 2 votes with gender=m, 1 vote with gender=f
--   answer_id=12 ("No")             -> 1 vote with gender=m
--   answer_id=13 ("I share one")     -> 0 votes
-- ---------------------------------------------------------------------------
INSERT INTO user_answers (question_id, answer_id, tags)
VALUES (3, 11, '{"gender": "m"}');

INSERT INTO user_answers (question_id, answer_id, tags)
VALUES (3, 11, '{"gender": "m"}');

INSERT INTO user_answers (question_id, answer_id, tags)
VALUES (3, 11, '{"gender": "f"}');

INSERT INTO user_answers (question_id, answer_id, tags)
VALUES (3, 12, '{"gender": "m"}');

-- ---------------------------------------------------------------------------
-- Minimum-age overrides for the age filter of POST /questions/restSearch
--   The seed data (003_seed_data.sql) leaves every question at the schema
--   default min_age = 0, so without the overrides below the age filter could
--   only ever return either everything (age = 0) or nothing (age > 0). We raise
--   min_age on a small, well-known subset of *existing* questions (no new rows,
--   so the pagination tests that assume exactly 100 questions stay valid) so
--   the GreaterEq behaviour of the filter can be exercised deterministically:
--     id 1, 3, 5 -> min_age 18   (all English)
--     id 7, 9    -> min_age 21   (all English)
-- ---------------------------------------------------------------------------
UPDATE questions SET min_age = 18 WHERE id IN (1, 3, 5);
UPDATE questions SET min_age = 21 WHERE id IN (7, 9);

-- ---------------------------------------------------------------------------
-- Password reset tokens for integration tests.
--
-- SHA-256 hashes of well-known token strings:
--   "test-reset-token"    -> 34d5d7ef743781a981a2efa15be22a860a4a792fd27595a7d70b72e697d88372
--   "expired-reset-token" -> 5bb79ac95be343e8bb144fc1d2d97ca3703a3ef41f5a91f28c301ec62401e9f4
--   "used-reset-token"    -> 8205601e8152da142931880206bcb4e93ed22ee960be08ea3f19adf76fcb47e1
--
-- Jim (id=1) has three tokens:
--   1. A valid, unused token expiring 1 hour in the future (test-reset-token).
--   2. An expired token (expired-reset-token) -- used to test the expiry check.
--   3. A used token (used-reset-token) -- used to test the "already used" check.
-- ---------------------------------------------------------------------------
INSERT INTO password_resets (user_id, token_hash, expires_at, used, attempt_count)
VALUES (1, '34d5d7ef743781a981a2efa15be22a860a4a792fd27595a7d70b72e697d88372',
        NOW() + INTERVAL '1 hour', FALSE, 0);

INSERT INTO password_resets (user_id, token_hash, expires_at, used, attempt_count)
VALUES (1, '5bb79ac95be343e8bb144fc1d2d97ca3703a3ef41f5a91f28c301ec62401e9f4',
        NOW() - INTERVAL '1 hour', FALSE, 0);

INSERT INTO password_resets (user_id, token_hash, expires_at, used, attempt_count)
VALUES (1, '8205601e8152da142931880206bcb4e93ed22ee960be08ea3f19adf76fcb47e1',
        NOW() + INTERVAL '1 hour', TRUE, 0);
