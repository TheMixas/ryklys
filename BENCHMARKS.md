# Benchmarks

How Ryklys was load tested, so every number in the README can be reproduced.

## Setup

- **Machine:** _CPU, cores, RAM, OS_
- **Build:** the Docker image (Release build), started with the production override so the app uses host networking (no Docker NAT in the path):
  ```bash
  cp backend/env.example backend/.env
  echo "QUIET_LOGS=1" >> backend/.env      # turn off per-request debug logging
  echo "SITE_DOMAIN=localhost" > .env
  docker compose -f docker-compose.yml -f docker-compose.prod.yml up -d --build
  ```
- The load generator runs on the same machine, pinned to separate cores with `taskset`. Each test was run 3 times; the tables show the median run.

## 1. HTTP throughput

```bash
taskset -c 0-3 wrk -t4 -c256 -d30s --latency http://127.0.0.1:3001/api/health
taskset -c 0-3 wrk -t4 -c256 -d30s --latency http://127.0.0.1:3001/api/streams/live
```

`/api/health` measures the HTTP server alone. `/api/streams/live` also takes a PostgreSQL connection from the pool and runs a query.

| Endpoint | Requests/s | p50 | p99 |
|---|---|---|---|
| `/api/health` | | | |
| `/api/streams/live` (PostgreSQL) | | | |

## 2. Chat fan-out

```bash
cd bench && npm install
node ws-fanout.mjs 1000 50
node ws-fanout.mjs 5000 50
```

[`bench/ws-fanout.mjs`](bench/ws-fanout.mjs) opens N viewer WebSockets on one stream, then sends chat messages from one more socket. Each message takes the real path: WebSocket → backend → PostgreSQL insert → Redis `PUBLISH` → relay → every viewer. The script reports the send-to-receive time for every delivery.

| Viewers | Delivered | p50 | p99 | Last viewer (p99) |
|---|---|---|---|---|
| 1,000 | | | | |
| 5,000 | | | | |

## 3. Glass-to-glass latency

Screen-share a page showing a millisecond clock, open the stream in a second window next to it, and take screenshots of both. The latency is the difference between the two clocks. Median of 10 screenshots:

| Browser | Median | Range |
|---|---|---|
| Chrome | | |

## 4. Footprint on the live demo

`docker stats --no-stream` on the 1 vCPU / 1 GB Azure VM while idle and while streaming:

| | CPU | Memory |
|---|---|---|
| Idle | | |
| One live stream | | |
