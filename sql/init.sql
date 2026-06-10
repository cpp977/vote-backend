CREATE TABLE categories (
    id BIGSERIAL PRIMARY KEY,
    name TEXT NOT NULL UNIQUE
);

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

CREATE TABLE questions (
    id BIGSERIAL PRIMARY KEY,
    text TEXT NOT NULL,
    category_id BIGINT NOT NULL REFERENCES categories(id) ON DELETE RESTRICT,
    language CHAR(2) NOT NULL REFERENCES languages(code) ON DELETE RESTRICT,
    min_age INT NOT NULL DEFAULT 0,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);


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
