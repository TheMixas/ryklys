#!/bin/sh
# Starts StreamPocket, the backend and Caddy in one container.
# If ANY of them exits, the container exits too, so Docker's restart policy
# brings the whole thing back up (e.g. backend started before Postgres was ready).

echo "[entrypoint] Starting StreamPocket on :7070..."
stream_pocket &
SP_PID=$!

echo "[entrypoint] Starting backend on :${SERVER_PORT:-3001}..."
SERVER_PORT=${SERVER_PORT:-3001} ryklys_backend &
BE_PID=$!

echo "[entrypoint] Starting Caddy..."
caddy run --config /etc/caddy/Caddyfile --adapter caddyfile &
CADDY_PID=$!

# Forward docker stop (SIGTERM) to the children so shutdown stays clean.
trap 'kill $SP_PID $BE_PID $CADDY_PID 2>/dev/null; exit 0' TERM INT

# Supervise: poll every 2s; kill -0 only checks that the process still exists.
while kill -0 "$SP_PID" 2>/dev/null \
   && kill -0 "$BE_PID" 2>/dev/null \
   && kill -0 "$CADDY_PID" 2>/dev/null; do
    sleep 2
done

echo "[entrypoint] A process exited — stopping container so Docker restarts it."
kill $SP_PID $BE_PID $CADDY_PID 2>/dev/null
exit 1
