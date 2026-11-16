-- 000006_optional_question_category.down.sql
--
-- Makes the category mandatory again. This fails while a question without one
-- exists, which is the point: such a question could not be made public again,
-- and a revert has to decide about it explicitly -- assign a category, or
-- delete the question.

ALTER TABLE questions
    DROP CONSTRAINT IF EXISTS questions_approved_has_category;

ALTER TABLE questions ALTER COLUMN category_id SET NOT NULL;