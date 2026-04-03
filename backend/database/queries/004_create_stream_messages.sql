CREATE TABLE chat_messages (
                               id          BIGSERIAL PRIMARY KEY,
                               stream_id   VARCHAR(64) NOT NULL,
                               user_id     INT NOT NULL,
                               username    VARCHAR(64) NOT NULL,
                               message     TEXT NOT NULL,
                               created_at  TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_chat_stream_time ON chat_messages (stream_id, created_at DESC);