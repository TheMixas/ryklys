-- 001: Users
CREATE TABLE IF NOT EXISTS users
(
    id            SERIAL PRIMARY KEY,
    username      VARCHAR(50) UNIQUE  NOT NULL,
    email         VARCHAR(254) UNIQUE NOT NULL,
    password_hash VARCHAR(255)        NOT NULL,
    is_active     BOOLEAN     DEFAULT TRUE,
    avatar_url    TEXT,
    created_at    TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP,
    updated_at    TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_users_email ON users (email);

-- 002: Channels
CREATE TABLE IF NOT EXISTS channels
(
    id      SERIAL PRIMARY KEY,
    user_id INT UNIQUE REFERENCES users (id),
    bio     TEXT
);

-- 003: Streams
CREATE TABLE IF NOT EXISTS streams
(
    id          VARCHAR(36) PRIMARY KEY,
    user_id     INT NOT NULL REFERENCES users(id),
    title       VARCHAR(255) NOT NULL,
    status      VARCHAR(20) NOT NULL DEFAULT 'live',
    started_at  TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP,
    ended_at    TIMESTAMPTZ
);

CREATE INDEX IF NOT EXISTS idx_streams_status ON streams(status);
CREATE INDEX IF NOT EXISTS idx_streams_user_id ON streams(user_id);

-- 004: Chat messages
CREATE TABLE IF NOT EXISTS chat_messages
(
    id          BIGSERIAL PRIMARY KEY,
    stream_id   VARCHAR(64) NOT NULL,
    user_id     INT NOT NULL,
    username    VARCHAR(64) NOT NULL,
    message     TEXT NOT NULL,
    created_at  TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_chat_stream_time ON chat_messages (stream_id, created_at DESC);
