-- 000006_optional_question_category
--
-- A question may be filed without a category: only its text is mandatory, and
-- an administrator assigns the category while approving it. The invariant that
-- an approved question is complete is kept at the database layer as far as a
-- row-local CHECK can express it -- the "at least two answer options" half
-- needs the whole table and stays a service-layer rule in the approve
-- endpoint. See DESIGN-question-submission.md in the vote repository.

ALTER TABLE questions ALTER COLUMN category_id DROP NOT NULL;

ALTER TABLE questions
    ADD CONSTRAINT questions_approved_has_category
    CHECK (submission_status <> 'approved' OR category_id IS NOT NULL);