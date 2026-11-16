-- 000001_init.down.sql
--
-- Reverts 000001_init.up.sql. Migrations run in one transaction, so a failure
-- here rolls the whole revert back.
--
-- Only the objects this migration creates are removed. The pg_trgm extension
-- is left in place: the CREATE EXTENSION in the up migration is a no-op when
-- the extension was already installed on the cluster, so dropping it here
-- could remove something this migration does not own.

DROP TABLE IF EXISTS question_user;
DROP TABLE IF EXISTS user_answers;
DROP TABLE IF EXISTS answer_options;
DROP TABLE IF EXISTS questions;
DROP TABLE IF EXISTS categories;
DROP TABLE IF EXISTS languages;

DROP TYPE IF EXISTS special_category_type;