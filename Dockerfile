# ============================================
# Backend build stage
# ============================================
FROM alpine:3.23.3 AS backend-build

RUN apk update && \
    apk add --no-cache \
        build-base \
        cmake \
        ninja \
        git \
        postgresql-dev \
        linux-headers \
        openssl-dev \
        curl-dev

# Build libpqxx 8.x from source
RUN git clone --branch 8.0.0 --depth 1 https://github.com/jtv/libpqxx.git /tmp/libpqxx && \
    cmake -S /tmp/libpqxx -B /tmp/libpqxx/build \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SHARED_LIBS=ON && \
    cmake --build /tmp/libpqxx/build -j$(nproc) && \
    cmake --install /tmp/libpqxx/build && \
    rm -rf /tmp/libpqxx

# Build libdatachannel from source
RUN git clone --recursive --depth 1 \
        https://github.com/paullouisageneau/libdatachannel.git \
        /tmp/libdatachannel && \
    cmake -S /tmp/libdatachannel -B /tmp/libdatachannel/build \
        -DCMAKE_BUILD_TYPE=Release \
        -DUSE_GNUTLS=0 \
        -DUSE_NICE=0 \
        -DBUILD_SHARED_LIBS=ON && \
    cmake --build /tmp/libdatachannel/build -j$(nproc) && \
    cmake --install /tmp/libdatachannel/build && \
    rm -rf /tmp/libdatachannel

WORKDIR /app
COPY backend/ .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build -j$(nproc)

# ============================================
# Frontend build stage
# ============================================
FROM node:20-alpine AS frontend-build

WORKDIR /app
COPY frontend/package.json frontend/package-lock.json ./
RUN npm ci
COPY frontend/ .
RUN npm run build

# ============================================
# Runtime stage
# ============================================
FROM alpine:3.23.3 AS runtime

RUN apk add --no-cache \
        libstdc++ \
        libpq \
        openssl \
        libcurl \
        ffmpeg \
        caddy

# Copy backend binary and shared libs
COPY --from=backend-build /usr/local/lib/libpqxx*          /usr/local/lib/
COPY --from=backend-build /usr/local/lib/libdatachannel*   /usr/local/lib/
COPY --from=backend-build /usr/local/lib/libjuice*         /usr/local/lib/
COPY --from=backend-build /app/build/ryklys_backend        /usr/local/bin/ryklys_backend
COPY --from=backend-build /app/build/stream_pocket         /usr/local/bin/stream_pocket

# Copy frontend build
COPY --from=frontend-build /app/dist /srv/frontend

# Copy Caddyfile
COPY backend/Caddyfile /etc/caddy/Caddyfile

# Copy entrypoint
COPY docker-entrypoint.sh /usr/local/bin/docker-entrypoint.sh
RUN chmod +x /usr/local/bin/docker-entrypoint.sh

RUN ldconfig /usr/local/lib || true
RUN mkdir -p /app/recordings

WORKDIR /app
EXPOSE 8080 7070

CMD ["docker-entrypoint.sh"]
