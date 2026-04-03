#!/bin/sh
set -e

echo "[entrypoint] Starting StreamPocket on :7070..."
stream_pocket &

echo "[entrypoint] Starting backend on :${SERVER_PORT:-3001}..."
SERVER_PORT=${SERVER_PORT:-3001} ryklys_backend &

echo "[entrypoint] Starting Caddy..."
caddy run --config /etc/caddy/Caddyfile --adapter caddyfile