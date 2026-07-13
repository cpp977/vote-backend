CREATE TABLE languages (
    code CHAR(2) PRIMARY KEY,
    name TEXT NOT NULL
);

INSERT INTO languages (code, name) VALUES
    ('en', 'English'),
    ('es', 'Spanish'),
    ('fr', 'French'),
    ('de', 'German'),
    ('zh', 'Chinese'),
    ('ja', 'Japanese'),
    ('ru', 'Russian'),
    ('ar', 'Arabic'),
    ('pt', 'Portuguese'),
    ('hi', 'Hindi');

CREATE TABLE categories (
    id BIGSERIAL PRIMARY KEY,
    name TEXT NOT NULL,
    language CHAR(2) NOT NULL REFERENCES languages(code) ON DELETE RESTRICT
);

-- Unique constraint so the same category name can appear only once per language
CREATE UNIQUE INDEX categories_name_lang_unique ON categories (name, language);

CREATE TABLE questions (
    id BIGSERIAL PRIMARY KEY,
    text TEXT NOT NULL,
    category_id BIGINT NOT NULL REFERENCES categories(id) ON DELETE RESTRICT,
    language CHAR(2) NOT NULL REFERENCES languages(code) ON DELETE RESTRICT,
    min_age INT NOT NULL DEFAULT 0,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- Enable pg_trgm extension for efficient LIKE/ILIKE searches
CREATE EXTENSION IF NOT EXISTS pg_trgm;

-- Create GIN trigram index on questions.text for efficient pattern matching
CREATE INDEX ix_questions_text_trigram
ON questions USING GIN (text gin_trgm_ops);

CREATE TABLE answer_options (
    id BIGSERIAL PRIMARY KEY,
    question_id BIGINT NOT NULL REFERENCES questions(id) ON DELETE CASCADE,
    text TEXT NOT NULL
);

CREATE TABLE user_answers (
    id BIGSERIAL PRIMARY KEY,
    question_id BIGINT NOT NULL REFERENCES questions(id) ON DELETE CASCADE,
    answer_id BIGINT NOT NULL REFERENCES answer_options(id) ON DELETE CASCADE,
    tags JSONB DEFAULT '{}'::jsonb,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX ix_user_answers_tags
ON user_answers
USING GIN (tags);

CREATE INDEX ix_user_answers_gender
ON user_answers ((tags->>'gender'))
WHERE tags ? 'gender';

CREATE INDEX ix_user_answers_age
ON user_answers ((tags->>'age'))
WHERE tags ? 'age';

-- Anonymous per-user answer tracking.
-- Enforces at the database layer that a given user may answer a question only
-- once. `hash_user_id` is a (salted) hash of the user id rather than the raw
-- id, so it is not possible to determine which specific user answered a given
-- question. The composite primary key on (question_id, hash_user_id) makes any
-- second answer attempt for the same (question, user) pair fail with a unique
-- violation.
CREATE TABLE question_user (
    question_id  BIGINT NOT NULL REFERENCES questions(id) ON DELETE CASCADE,
    hash_user_id TEXT NOT NULL,
    created_at   TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    PRIMARY KEY (question_id, hash_user_id)
);
