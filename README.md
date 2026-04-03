# Ryklys

**A live streaming platform built from scratch — no frameworks, no shortcuts.**

![Ryklys Demo](docs/demo.gif)

Stream directly from your browser to an audience in real time. No OBS, no downloads. Just open Ryklys, go live, and your viewers are watching within seconds.

## What makes this different

The entire backend — HTTP server, WebSocket handler, connection pool, stream pipeline — is **written from scratch in C++** using raw Linux sockets and epoll. The frontend captures your camera/screen via WebRTC, sends it to the backend, which pipes it through FFmpeg into HLS segments and delivers them to viewers. Chat runs over Redis Pub/Sub across multiple nodes.

The custom HTTP server parses requests, routes them, handles CORS, manages cookies, and upgrades WebSocket connections — all custom. The connection pool, thread pool, JWT auth, and service locator are all hand-rolled.

## Architecture

```
Browser (Streamer)                        Browser (Viewer)
     │                                         ▲
     │ WebRTC                                  │ HLS
     ▼                                         │
┌─────────────────────────────────────────────────────┐
│                      Caddy                          │
│              (reverse proxy / TLS)                  │
└────────────┬────────────────────────┬───────────────┘
             │                        │
     ┌───────▼───────┐       ┌───────▼───────┐
     │   Backend A   │       │   Backend B   │
     │  (C++ / :3001)│       │  (C++ / :3002)│
     └───┬───┬───┬───┘       └───┬───┬───┬───┘
         │   │   │               │   │   │
         │   │   └──────┬────────┘   │   │
         │   │          │            │   │
    ┌────▼┐ ┌▼─────┐ ┌─▼──────┐ ┌──▼─┐ │
    │FFmpeg│ │Redis │ │PostgreSQL│ │FFmpeg│
    │     │ │Pub/Sub│ │        │ │     │ │
    └──┬──┘ └──────┘ └────────┘ └──┬──┘ │
       │                           │     │
       ▼                           ▼     │
  ┌──────────────────────────────────┐   │
  │         StreamPocket             │◄──┘
  │   (HLS segment storage / :7070) │
  └──────────────────────────────────┘
```

## Tech Stack

**All built from scratch (no frameworks):**
- Custom HTTP/1.1 server with epoll (edge-triggered) and thread pool
- WebSocket protocol implementation (handshake, framing, masking)
- Connection pooling for PostgreSQL and Redis
- JWT authentication (create, verify, cookie management)
- CORS middleware
- Service locator pattern for dependency injection
- StreamPocket — a custom HLS segment storage and delivery server

**Infrastructure:**
- C++20 backend on Linux
- React + TypeScript + Vite frontend
- PostgreSQL for users and stream metadata
- Redis for pub/sub chat, viewer tracking, and multi-node coordination
- FFmpeg for WebRTC → HLS transcoding
- Caddy for reverse proxy and load balancing
- Docker + Docker Compose

## Features

- **Browser-native streaming** — go live with your webcam or screen share, no software to install
- **Real-time HLS delivery** — sub-5-second latency from streamer to viewer
- **Live chat** — Redis pub/sub powered, works across multiple backend nodes
- **Viewer count tracking** — real-time, broadcast to all connected viewers
- **Multi-node ready** — stateless backend design with Caddy load balancing and Redis coordination
- **User authentication** — register, login, JWT sessions via HttpOnly cookies
- **Stream history** — browse your past streams with duration and metadata
- **Stream browsing** — discover who's live and jump into any stream

## Getting Started

### Docker (recommended)

The fastest way to run everything. Just Docker — no other dependencies needed.

```bash
git clone https://github.com/TheMixas/ryklys.git
cd ryklys
docker compose up --build
```

Open `http://localhost:8080` — that's it. Caddy serves the frontend and proxies the backend, StreamPocket, and WebSocket connections through a single port.

To tear down and reset the database:

```bash
docker compose down -v
```

### Local Development

For active development where you want hot reload on the frontend and quick rebuilds on the backend.

**Prerequisites:** C++20 compiler (GCC 11+ or Clang 14+), CMake 3.20+, Node.js 18+, PostgreSQL, Redis, FFmpeg, Caddy

**1. Database setup:**

```bash
# Start PostgreSQL and Redis (or use your existing instances)
sudo systemctl start postgresql redis

# Create the tables
psql -U postgres -d postgres -f backend/init.sql
```

**2. Backend:**

```bash
cd backend
cp .env.example .env          # edit with your DB/Redis connection details

cmake -B cmake-build-debug
cmake --build cmake-build-debug

./cmake-build-debug/ryklys_backend
```

StreamPocket starts as a separate binary (built automatically by CMake):

```bash
./cmake-build-debug/stream-pocket/stream_pocket
```

**3. Frontend:**

```bash
cd frontend
npm install
npm run dev
```

The frontend dev server runs on `http://localhost:5173` with hot reload.

**4. Multi-node with Caddy (optional):**

```bash
cd backend
SERVER_PORT=3001 ./cmake-build-debug/ryklys_backend &
SERVER_PORT=3002 ./cmake-build-debug/ryklys_backend &
caddy run --config Caddyfile
```

All traffic goes through `http://localhost:8080`. Caddy handles round-robin for API routes and sticky sessions for WebSocket connections.

## Project Structure

```
ryklys/
├── backend/
│   ├── main.cpp                    # Entry point — epoll loop, server bootstrap
│   ├── zvejys-rest-api/            # Custom HTTP server core
│   │   ├── ZvejysServer.h          # Server, routing, WebSocket upgrade
│   │   ├── HttpConnection.h        # Request parsing, response writing
│   │   └── utils/                  # JWT, password hashing, validation
│   ├── routes/                     # Route handlers
│   │   ├── user-route/             # Register, login, profile
│   │   ├── stream-route/           # Go live, stream management
│   │   └── viewer-route/           # Viewer WebSocket, chat
│   ├── services/                   # ChatRelay, RedisClient, ViewerRegistry
│   ├── database/                   # PostgreSQL repositories and migrations
│   ├── stream-pocket/              # HLS segment storage server
│   ├── pg-connection-pool/         # Custom PostgreSQL connection pool
│   ├── include/                    # Thread pool, external headers
│   ├── init.sql                    # Database schema
│   ├── .env.example                # Environment variable template
│   ├── CMakeLists.txt
│   ├── Caddyfile                   # Local dev reverse proxy config
│   └── Dockerfile
├── frontend/
│   ├── src/
│   │   ├── app/routes/             # Landing, browse, stream view, dashboard
│   │   ├── components/             # Navbar, ChatPanel, RequireAuth
│   │   ├── hooks/                  # Auth context, custom hooks
│   │   ├── features/               # Stream capture, scene management
│   │   └── config/                 # Environment, paths
│   ├── package.json
│   └── vite.config.ts
├── docker-compose.yml
├── Dockerfile
├── docker-entrypoint.sh
└── README.md
```

## Roadmap

- [ ] HTTPS everywhere (Caddy + Let's Encrypt)
- [ ] FFmpeg process watchdog
- [ ] Stream thumbnails for the browse page
- [ ] VOD — keep segments after stream ends
- [ ] Follow system and channel pages
- [ ] Stream categories and tags
- [ ] Clip system
- [ ] Adaptive bitrate (multiple quality options)
- [ ] CDN for segment delivery at scale

## Author

**TheMixas** — [github.com/TheMixas](https://github.com/TheMixas)

Built as a deep dive into systems programming, networking, and real-time media delivery.