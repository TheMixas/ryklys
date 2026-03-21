# Ryklys 🦈

High performance browser live streaming platform. Most infrastructure is built from scratch with minimal dependencies.

## Project Overview

Ryklys (Lithuanian for "shark") is an ambitious learning project aimed at delivering a high-performance, browser-based live streaming platform. Ryklys focuses on building core infrastructure from the ground up, minimizing external dependencies to achieve knowlege, superior control, optimization, and efficiency.

## Key Features & Benefits

*   **High Performance**: Epoll, thread pools.
*   **Minimal Dependencies**: Utilizes only a few dependencies such as libqxx and nlohmann/json.
*   **Built From Scratch**: Http 1.1, Websockets, JWT encoding & decoding are custom-built, allowing for deep optimization specific to live streaming needs.
*   **Browser-Native Streaming**: Designed to integrate seamlessly with modern web browsers.
*   **Dockerized Deployment**: Easy to deploy and manage using Docker and Docker Compose, ensuring consistent environments.

## Installation & Setup

### 1. Clone the Repository

Start by cloning the Ryklys repository to your local machine:

```bash
git clone https://github.com/TheMixas/ryklys.git
cd ryklys
```

### 2. Using Docker Compose (Recommended)

The easiest way to get Ryklys up and running is with Docker Compose. This will build the application image, set up a PostgreSQL database, and connect them.

1.  **Build and run the services**:

    ```bash
    docker-compose up --build -d
    ```
    *   `--build`: Ensures that the Ryklys service image is built from scratch.
    *   `-d`: Runs the services in detached mode (in the background).

2.  **Verify services are running**:

    ```bash
    docker-compose ps
    ```
    You should see `ryklys` and `db` services listed with a `healthy` or `Up` status.

### 3. Manual Docker Build & Run

If you prefer to build and run the Ryklys Docker image independently of Docker Compose:

1.  **Build the Docker image**:

    ```bash
    docker build -t ryklys .
    ```

2.  **Run the container**:
    You'll need a running PostgreSQL instance accessible from the container. You can use Docker Compose for the database or run a PostgreSQL container separately.
    Assuming a PostgreSQL container is running and accessible at `host.docker.internal:5432` (or adjust `POSTGRES_HOST`):

    ```bash
    docker run -p 8080:8080 --name ryklys_app \
        -e POSTGRES_HOST=host.docker.internal \
        -e POSTGRES_PORT=5432 \
        -e POSTGRES_DB=ryklys_db \
        -e POSTGRES_USER=ryklys_user \
        -e POSTGRES_PASSWORD=ryklys_password \
        ryklys
    ```
    *   `-p 8080:8080`: Maps the container's port `8080` to your host's port `8080`.
    *   `-e`: Sets environment variables for database connection (adjust as per your PostgreSQL setup).
    *   
## Configuration

Ryklys primarily uses environment variables for configuration. These are set in the .env file in the source directory and passed to docker-comose.yml.

### Environment Variables

| Variable          | Description                                           | Default (if any) |
| :---------------- | :---------------------------------------------------- | :--------------- |
| `DB_HOST`         | Hostname or IP address of the PostgreSQL server.      | `db` (for Docker Compose) |
| `DB_PORT`         | Port number for the PostgreSQL server.                | `5432`           |
| `DB_NAME    `     | Name of the PostgreSQL database to connect to.        | `ryklys_db`      |
| `DB_USER      `   | Username for PostgreSQL database access.              | `ryklys_user`    |
| `DB_PASSWORD     `| Password for the PostgreSQL database user.           | `N/A`|
| `SERVER_PORT`     | The port on which the Ryklys application will listen.| `8080`           |
| `SERVER_HOST`     | The server's ip adress.| `0.0.0.0`           |
| `JWT_SECRET`      | The JWT secret used for JWT tokens. | `N/A`           |



Refer to `config/EnvConfig.h` for all available configuration options and their default values.

## Usage

Once Ryklys is running (e.g., via `docker-compose up -d`), you can access the platform through your web browser.
