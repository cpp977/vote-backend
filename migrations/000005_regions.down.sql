-- 000005_regions.down.sql
--
-- Reverts 000005_regions.up.sql, which only inserts the ISO 3166-2
-- subdivision rows into the `regions` table created by 000002_auth.
--
-- The DELETE fails while a user still references a region (users.region is
-- ON DELETE RESTRICT), which is the intended outcome: dropping the reference
-- data of a database that has users on it is not something a revert should do
-- silently.

DELETE FROM regions;