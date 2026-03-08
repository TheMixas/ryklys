CREATE TABLE users
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

CREATE INDEX idx_users_email ON users (email);
