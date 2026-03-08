CREATE TABLE channels
(
    id      SERIAL PRIMARY KEY,
    user_id INT UNIQUE REFERENCES users (id),
    bio     TEXT
);