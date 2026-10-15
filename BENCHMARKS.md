# Benchmarks

How Ryklys was load tested, so the numbers in the README can be reproduced.

## Setup

- **Machine:** Intel Core i9-14900HX (24 cores, 32 threads), Fedora 44, Docker 29
- **Build:** the Docker image (Release build), started with the production override so the app uses host networking (no Docker NAT in the path):
  ```bash
  cp backend/env.example backend/.env
  echo "QUIET_LOGS=1" >> backend/.env      # turn off per-request debug logging
  echo "SITE_DOMAIN=localhost" > .env
  docker compose -f docker-compose.yml -f docker-compose.prod.yml up -d --build
  ```
- The load generator runs on the same machine as the server.

## HTTP throughput

```bash
wrk -t4 -c256 -d30s --latency http://127.0.0.1:3001/api/health
wrk -t4 -c256 -d30s --latency http://127.0.0.1:3001/api/streams/live
```

`/api/health` measures the HTTP server alone. `/api/streams/live` also takes a PostgreSQL connection from the pool and runs a query.

| Endpoint | Requests/s | p50 | p99 |
|---|---|---|---|
| `/api/health` | 125,942 | 1.99 ms | 3.30 ms |
| `/api/streams/live` (PostgreSQL) | 45,500 | 5.01 ms | 18.97 ms |

256 connections, 30 s each, no errors.

## Chat fan-out

```bash
cd bench && npm install
node ws-fanout.mjs 1000 50
node ws-fanout.mjs 5000 50
```

[`bench/ws-fanout.mjs`](bench/ws-fanout.mjs) opens N viewer WebSockets on one stream, then sends 50 chat messages from one more socket. Each message takes the real path: WebSocket → backend → PostgreSQL insert → Redis `PUBLISH` → relay → every viewer. The script records the send-to-receive time for every delivery.

| Viewers | Delivered | p50 | p99 | Last viewer (p99) |
|---|---|---|---|---|
| 1,000 | 50,000 / 50,000 | 8.0 ms | 20.1 ms | 21.9 ms |
| 5,000 | 250,000 / 250,000 | 22.1 ms | 42.8 ms | 48.3 ms |
