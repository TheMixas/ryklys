CREATE TABLE streams (
                         id          VARCHAR(36) PRIMARY KEY,
                         user_id     INT NOT NULL REFERENCES users(id),
                         title       VARCHAR(255) NOT NULL,
                         status      VARCHAR(20) NOT NULL DEFAULT 'live',
                         started_at  TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP,
                         ended_at    TIMESTAMPTZ
);

CREATE INDEX idx_streams_status ON streams(status);
CREATE INDEX idx_streams_user_id ON streams(user_id);