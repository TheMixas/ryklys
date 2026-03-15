#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <unistd.h>
#include <unordered_map>

#include "ServiceLocator.h"
#include "config/EnvConfig.h"

#include "database/database_repositories/UserRepository.h"
#include "include/ptsouchlos_thread-pool/thread_pool.h"
#include "pg-connection-pool/pgPool.h"
#include "zvejys-rest-api/HttpConnection.h"
#include "zvejys-rest-api/ZvejysServer.h"
#include "zvejys-rest-api/utils/JsonWebToken.h"
#include "routes/TestRoute/TestRoutes.h"
#include "routes/UserRoute/UserRoutes.h"
const int MAX_EVENT_BATCH = 64; // Maximum number of epoll events to process in one batch

int threadPoolSize = std::thread::hardware_concurrency() * 2;
static dp::thread_pool appThreadPool(threadPoolSize);
CorsConfig corsConfig{
    .allowed_origins = {"http://localhost:3000", "http://localhost:5173"},
    .allow_credentials = true,
    .allowed_methods = {"GET", "POST", "PUT", "DELETE", "OPTIONS"},
    .allowed_headers = {"Content-Type", "Authorization"},
    .max_age_seconds = 86400 // 24 hours
};
int CreateEpoll() {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        throw std::runtime_error("Failed to create epoll instance");
    }

    return epoll_fd;
}

void AddToEpoll(int epollFD, int fd) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET; // Edge-triggered read events
    epoll_ctl(epollFD, EPOLL_CTL_ADD, fd, &event);
}
std::optional<AuthenticatedUser> ExtractAuthenticatedUserFromRequest(const HttpRequest &request) {
    std::string token;
    auto cookieIt = request.headers.find("Cookie");
    if (cookieIt != request.headers.end()) {
        const std::string &cookie = cookieIt->second;
        // crude parsing; robust parser recommended
        auto pos = cookie.find("token=");
        if (pos != std::string::npos) {
            auto start = pos + 6;
            auto end = cookie.find(';', start);
            token = cookie.substr(start, (end==std::string::npos) ? std::string::npos : end-start);
        }
    }

    if (token.empty()) return std::nullopt;

    // validate token as before
    auto jwtSecretOpt = EnvConfig::Instance().Get("JWT_SECRET");
    if (!jwtSecretOpt.has_value()) return std::nullopt;
    try {
        auto claims = jwt::Verify(token, jwtSecretOpt.value());
        if (!claims.has_value()) return std::nullopt;
        AuthenticatedUser user{claims->id, claims->username};
        return user;
    } catch (...) {
        return std::nullopt;
    }
}
// std::optional<AuthenticatedUser> ExtractAuthenticatedUserFromRequest(const HttpRequest &request) {
//     auto authHeaderIt = request.headers.find("Authorization");
//     if (authHeaderIt == request.headers.end()) {
//         return std::nullopt;
//     }
//
//     const std::string &authHeader = authHeaderIt->second;
//     if (authHeader.rfind("Bearer ", 0) != 0) {
//         return std::nullopt;
//     }
//
//     std::string token = authHeader.substr(7);
//     auto jwtSecretOpt = EnvConfig::Instance().Get("JWT_SECRET");
//     if (!jwtSecretOpt.has_value()) {
//         std::cerr << "JWT_SECRET not set in environment variables" << std::endl;
//         return std::nullopt;
//     }
//     std::string jwtSecret = jwtSecretOpt.value();
//
//     try {
//         auto claims = jwt::Verify(token, jwtSecret);
//         if (!claims.has_value()) {
//             return std::nullopt;
//         }
//         AuthenticatedUser user;
//         user.id = claims.value().id;
//         user.username = claims.value().username;
//         return user;
//     } catch (const std::exception &e) {
//         std::cerr << "Failed to decode JWT: " << e.what() << std::endl;
//         return std::nullopt;
//     }
// }

// void RegisterRoutes(ZvejysServer &server) {
//
// }

int main() {
    //load local path + .env
    auto &env = EnvConfig::Instance();
    env.Load(std::string(PROJECT_ROOT) + "/.env");

    static PGPool databaseConnectionPool;
    static UserRepository userRepository(databaseConnectionPool);
    ServiceLocator::SetUserRepository(&userRepository);


    std::cout << "Hello, World!" << std::endl;
    int epoll_fd = CreateEpoll();

    // -- Setup http rest server
    std::string serverHost = env.Get("SERVER_HOST", "127.0.0.1");
    std::string jwtSecret = env.Get("JWT_SECRET", "buillshit");


    int serverPort = std::stoi(env.Get("SERVER_PORT", "8080"));

    ZvejysServer httpServer(serverHost, serverPort,corsConfig, ExtractAuthenticatedUserFromRequest);
    int httpServerSocketFD = httpServer.GetSocketFD();
    httpServer.Start();
    RegisterTestRoutes(httpServer);
    RegisterUserRoutes(httpServer);
    std::cout << "I  have routes: " << httpServer.GetRouteMap().size() << std::endl;
    std::cout << "Routes: " << std::endl;
    for (auto route = httpServer.GetRouteMap().begin(); route != httpServer.GetRouteMap().end(); ++route) {
        std::cout << route->first << std::endl;
    }

    AddToEpoll(epoll_fd, httpServerSocketFD);
    // --

    struct epoll_event ready_events[MAX_EVENT_BATCH];
    while (1) {
        // here we wait for any event from the socket
        int nfds = epoll_wait(epoll_fd, ready_events, MAX_EVENT_BATCH, -1);
        printf("Epoll wait returned %d\n", nfds);
        // loop over the ready FDs (in this case we have only one)
        for (int i = 0; i < nfds; i++) {
            epoll_event &event = ready_events[i];

            if (event.data.fd == httpServerSocketFD) {
                // new connection
                httpServer.HandleEpollNewConnection(epoll_fd, event);
                std::cout << "I just accepted a new connection" << std::endl;
            } else {
                auto conn = static_cast<Connection *>(event.data.ptr);
                if (event.events & EPOLLIN) {
                    appThreadPool.enqueue_detach([conn, epoll_fd]() {
                        conn->OnReadable();
                    });
                }
                if (event.events & EPOLLOUT) {
                    appThreadPool.enqueue_detach([conn, epoll_fd]() {
                        conn->OnWritable(epoll_fd);
                    });
                }
                // if (event.events & EPOLLIN) {
                //     pool.enqueue_detach([&httpServer, event, epoll_fd]() {
                //         httpServer.HandleEpollReceiveClientData(epoll_fd, event);
                //         std::cout << "IM GETTING DATA GAY" << std::endl;
                //     });
                // }
                // if (event.events & EPOLLOUT) {
                //     // Socket is ready for writing
                //     // Here we would write the response back to the client
                //     pool.enqueue_detach([&httpServer, event, epoll_fd]() {
                //         httpServer.HandleEpollSendClientData(epoll_fd, event);
                //         std::cout << "i WANT TO WRITE GAY" << std::endl;
                //     });
                // }
                if (event.events & (EPOLLHUP | EPOLLERR)) {
                    appThreadPool.enqueue_detach([&httpServer, event]() {
                        httpServer.HandleClientClosedConnection(event);
                        std::cout << "Client disconnected or error occurred" << std::endl;
                    });
                }
            }
        }
    }

    return 0;
}
