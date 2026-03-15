FROM alpine:3.23.3 AS build

RUN apk update && \
    apk add --no-cache \
        build-base \
        cmake \
        ninja \
        git \
        postgresql-dev \
        linux-headers

# Build libpqxx 8.x from source
RUN git clone --branch 8.0.0 --depth 1 https://github.com/jtv/libpqxx.git /tmp/libpqxx && \
    cmake -S /tmp/libpqxx -B /tmp/libpqxx/build \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SHARED_LIBS=ON && \
    cmake --build /tmp/libpqxx/build -j$(nproc) && \
    cmake --install /tmp/libpqxx/build && \
    rm -rf /tmp/libpqxx

# Copy your project
WORKDIR /app
COPY . .

# Build your project
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build -j$(nproc)

# --- Runtime stage (keep image small) ---
FROM alpine:3.23.3 AS runtime

RUN apk add --no-cache \
        libstdc++ \
        libpq

COPY --from=build /usr/local/lib/libpqxx* /usr/local/lib/
COPY --from=build /app/build/ryklys_backend /usr/local/bin/ryklys_backend

RUN ldconfig /usr/local/lib || true

EXPOSE 8080
CMD ["ryklys_backend"]