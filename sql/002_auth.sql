-- Authentication tables

CREATE TABLE users (
    id BIGSERIAL PRIMARY KEY,
    username TEXT NOT NULL UNIQUE,
    email TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL,
    birth_year INT,
    gender CHAR(1) CHECK (gender IN ('m', 'w', 'd')),
    nationality VARCHAR(100),
    is_admin BOOLEAN NOT NULL DEFAULT FALSE,
    is_active BOOLEAN NOT NULL DEFAULT TRUE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    password_changed_at TIMESTAMPTZ
);

CREATE TABLE refresh_tokens (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    token_hash TEXT NOT NULL,
    expires_at TIMESTAMPTZ NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    revoked BOOLEAN NOT NULL DEFAULT FALSE
);

CREATE INDEX ix_refresh_tokens_token_hash ON refresh_tokens (token_hash);

-- ---------------------------------------------------------------------------
-- Password reset tokens
-- Stored as SHA-256 hashes (never plain text). Each token is single-use,
-- time-limited, and protected by an attempt counter that is incremented on
-- every failed validation; after a configurable number of failures the token
-- is "killed" (used = TRUE) so it can never be redeemed.
-- ---------------------------------------------------------------------------
CREATE TABLE password_resets (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(id),
    token_hash TEXT NOT NULL,
    expires_at TIMESTAMP NOT NULL,
    used BOOLEAN NOT NULL DEFAULT FALSE,
    attempt_count INT NOT NULL DEFAULT 0,
    created_at TIMESTAMP NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_password_resets_user_created
    ON password_resets (user_id, created_at);

-- ---------------------------------------------------------------------------
-- Link question submissions to the users who created / reviewed them.
-- This must live in 002_auth.sql (not 001_init.sql) because the users table is
-- created above, and Docker runs the init scripts in alphabetical order
-- (001 before 002), so the referenced table exists by the time we get here.
ALTER TABLE questions
    ADD CONSTRAINT fk_questions_submitted_by
        FOREIGN KEY (submitted_by) REFERENCES users(id) ON DELETE SET NULL,
    ADD CONSTRAINT fk_questions_reviewed_by
        FOREIGN KEY (reviewed_by) REFERENCES users(id) ON DELETE SET NULL;
