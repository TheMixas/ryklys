# Ryklys Frontend 🦈

[![Frontend CI](https://github.com/themixas/ryklys-frontend/actions/workflows/ci.yml/badge.svg)](https://github.com/themixas/ryklys-frontend/actions/workflows/ci.yml)
[![React](https://img.shields.io/badge/React-18-blue.svg)](https://reactjs.org/)
[![TypeScript](https://img.shields.io/badge/TypeScript-5.0-blue.svg)](https://www.typescriptlang.org/)
[![Vite](https://img.shields.io/badge/Vite-5.0-purple.svg)](https://vitejs.dev/)

🔥**Ryklys** is a high-performance, real-time browser application engineered for live streaming and real-time user interaction via WebRTC and WebSockets. Built with React, TypeScript, and Vite, it interfaces with a custom high-concurrency C++ backend.



## 🗺️ Project Status & Roadmap
* **User Authentication**  ✔️
* **Dynamic Customizable Streaming Layouts**  ✔️ easy to change unique layouts.
* **Ability to capture video streams & images** ✔️ capture video streams and overlays with ease alike 👍
* **WebGL Rendering** ✔️ Efficient browser based rendering to combine multiple streams and overlays.
* **LiveStream popups** 🚧 these popups will display donations, special messages and announce all kind of events.
* **WebRTC Streaming Integration With Backend** 🚧 current stage: signaling
* **Streamer Chat Room** 🔜
* **User Profiles** 🔜
* **LiveStream discoverability** 🔜
---

## 🏗️ Infrastructure & CI/CD

This project is built with production readiness and reproducibility in mind. 

* **Automated Pipelines:** Fully automated CI/CD pipelines implemented via **GitHub Actions**. Every push and pull request triggers strict dependency installation (`npm ci`), code quality linting, and a full production build to ensure main branch stability.
* **Containerized Ecosystem:** The accompanying C++ backend and PostgreSQL database are fully containerised using **Docker** and `docker-compose`. The CI/CD pipeline utilizes Docker Buildx caching to verify secure and reliable container builds on every commit.

---

## 🛠️ Getting Started

### Installation & Setup

**1. Clone the repository:**
```bash
git clone [https://github.com/themixas/backend-frontend.git](https://github.com/themixas/ryklys-frontend.git)
cd backend-frontend
```

**2. Install dependencies:**
*(Note: We use `npm ci` to ensure strict adherence to the `package-lock.json` for reproducible environments)*
```bash
npm ci
```

**3. Start the development server:**
```bash
npm run dev
```

The application will be available at `http://localhost:5173`.
