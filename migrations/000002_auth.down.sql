-- 000002_auth.down.sql
--
-- Reverts 000002_auth.up.sql: the foreign keys of `questions` go first, then
-- the tables that reference `users`, then `users` itself, and finally the two
-- reference tables (`countries` is dropped last because `regions` points at
-- it). Reference *data* is not restored - only the tables it lived in.

ALTER TABLE questions
    DROP CONSTRAINT IF EXISTS fk_questions_submitted_by,
    DROP CONSTRAINT IF EXISTS fk_questions_reviewed_by;

DROP TABLE IF EXISTS password_resets;
DROP TABLE IF EXISTS refresh_tokens;
DROP TABLE IF EXISTS users;

DROP TABLE IF EXISTS regions;
DROP TABLE IF EXISTS countries;