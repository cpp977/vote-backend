-- 004_test_data.sql
-- Integration test seed data: a single user and pre-defined answer submissions
-- so that GET /questions/{id}/stats returns deterministic results.

-- ---------------------------------------------------------------------------
-- Test user (password: 12345678, hashed with argon2id)
-- ---------------------------------------------------------------------------
INSERT INTO users (username, email, password_hash, birth_year, gender, nationality)
VALUES (
  'Jim',
  'jim@example.com',
  '$argon2id$v=19$m=65536,t=2,p=1$7IwaKiaTuSf8MV6JOC/InA$KPZSbQabGbDdhRz1JtzjWUk4wokot/5PebP8xmP/nzQ',
  1990,
  'm',
  'US'
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
