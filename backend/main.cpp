#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <unistd.h>
#include <unordered_map>

#include "ServiceLocator.h"
// #include "cmake-build-debug/_deps/libdatachannel-src/include/rtc/configuration.hpp"
#include "RedisPool.h"
#include "database/database_repositories/StreamRepository.h"
#include "zvejys-rest-api/env/EnvConfig.h"

#include "database/database_repositories/UserRepository.h"
#include "include/ptsouchlos_thread-pool/thread_pool.h"
#include "pg-connection-pool/pgPool.h"
#include "routes/health-route/HealthRoutes.h"
#include "routes/stream-route/StreamerRoutes.h"
#include "zvejys-rest-api/HttpConnection.h"
#include "zvejys-rest-api/ZvejysServer.h"
#include "zvejys-rest-api/utils/JsonWebToken.h"
#include "routes/test-route/TestRoutes.h"
#include "routes/user-route/UserRoutes.h"
#include "routes/viewer-route/VIewerRoutes.h"
#include "rtc/global.hpp"
#include "services/ChatRelay.h"
#include "services/RedisClient.h"
const int MAX_EVENT_BATCH = 64; // Maximum number of epoll events to process in one batch

int threadPoolSize = std::thread::hardware_concurrency() * 2;
static dp::thread_pool appThreadPool(threadPoolSize);
CorsConfig corsConfig{
    .allowed_origins = {"https://localhost:3000", "https://localhost:5173", "https://169.254.83.107:5173"},
    .allow_credentials = true,
    .allowed_methods = {"GET", "POST", "PUT", "DELETE", "OPTIONS"},
    .allowed_headers = {"Content-Type", "Authorization"},
    .max_age_seconds = 86400 // 24 hours
};

int CreateEpoll()
{
    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0)
    {
        throw std::runtime_error("Failed to create epoll instance");
    }

    return epoll_fd;
}

void AddToEpoll(int epollFD, int fd)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET; // Edge-triggered read events
    epoll_ctl(epollFD, EPOLL_CTL_ADD, fd, &event);
}

static std::optional<AuthenticatedUser> ExtractAuthenticatedUserFromRequest(const HttpRequest& request)
{
    std::cout << "[AUTH] === Extracting auth from request ===" << std::endl;
    std::string token;
    auto cookieIt = request.headers.find("Cookie");
    if (cookieIt != request.headers.end())
    {
        std::cout << "[AUTH] Cookie header found: " << cookieIt->second << std::endl;
        const std::string& cookie = cookieIt->second;
        auto pos = cookie.find("token=");
        if (pos != std::string::npos)
        {
            auto start = pos + 6;
            auto end = cookie.find(';', start);
            token = cookie.substr(start, (end == std::string::npos) ? std::string::npos : end - start);
            std::cout << "[AUTH] Extracted token (length=" << token.length() << ")" << std::endl;
        }
        else
        {
            std::cout << "[AUTH] No 'token=' found in cookie string" << std::endl;
        }
    }
    else
    {
        std::cout << "[AUTH] No Cookie header present in request" << std::endl;
    }

    if (token.empty())
    {
        std::cout << "[AUTH] FAIL: Token is empty" << std::endl;
        return std::nullopt;
    }

    auto jwtSecretOpt = EnvConfig::Instance().Get("JWT_SECRET");
    if (!jwtSecretOpt.has_value())
    {
        std::cout << "[AUTH] FAIL: JWT_SECRET not configured" << std::endl;
        return std::nullopt;
    }
    try
    {
        auto claims = jwt::Verify(token, jwtSecretOpt.value());
        if (!claims.has_value())
        {
            std::cout << "[AUTH] FAIL: Token verification returned no claims" << std::endl;
            return std::nullopt;
        }
        std::cout << "[AUTH] SUCCESS: Verified user ID=" << claims->id << " username=" << claims->username << std::endl;
        AuthenticatedUser user{claims->id, claims->username};
        return user;
    }
    catch (const std::exception& e)
    {
        std::cout << "[AUTH] FAIL: Exception during verification — " << e.what() << std::endl;
        return std::nullopt;
    }
    catch (...)
    {
        std::cout << "[AUTH] FAIL: Unknown exception during verification" << std::endl;
        return std::nullopt;
    }
}
[[noreturn]] int main()
{
    rtc::InitLogger(rtc::LogLevel::Info);

    //load local path + .env
    auto& env = EnvConfig::Instance();
    env.Load(std::string(PROJECT_ROOT) + "/.env");

    static PGPool databaseConnectionPool;
    static UserRepository userRepository(databaseConnectionPool);
    static StreamRepository streamRepository(databaseConnectionPool);
    static ChatRepository chatRepository(databaseConnectionPool);
    ServiceLocator::SetUserRepository(&userRepository);
    ServiceLocator::SetStreamRepository(&streamRepository);
    ServiceLocator::SetChatRepository(&chatRepository);
    std::string redisHost = env.Get("REDIS_HOST", "127.0.0.1");
    std::string redisPort = env.Get("REDIS_PORT", "6379");
    static RedisClient redisClient(redisHost, redisPort);
    if (!redisClient.connect())
    {
        std::cerr << "Failed to connect to Redis!" << std::endl;
        return 1;
    }
    ServiceLocator::SetRedisClient(&redisClient);
    std::cout << "Redis connected!" << std::endl;

    static RedisPool redisPool(redisHost, redisPort, 4);
    ServiceLocator::SetRedisPool(&redisPool);
    std::cout << "Redis pool ready (4 connections)" << std::endl;

    // Viewer tracking
    static ViewerRegistry viewerRegistry;
    ServiceLocator::SetViewerRegistry(&viewerRegistry);

    // Chat relay — own connection, own thread
    static ChatRelay chatRelay(redisHost, redisPort, viewerRegistry);
    if (!chatRelay.Start())
    {
        std::cerr << "Failed to start ChatRelay!" << std::endl;
        return 1;
    }
    ServiceLocator::SetChatRelay(&chatRelay);


    streamRepository.EndAll();

    int epoll_fd = CreateEpoll();

    // -- Setup http rest server
    std::string serverHost = env.Get("SERVER_HOST", "127.0.0.1");
    std::string jwtSecret = env.Get("JWT_SECRET", "buillshit");
    int serverPort = std::stoi(env.Get("SERVER_PORT", "8080"));

    ZvejysServer httpServer(serverHost, serverPort, corsConfig, ExtractAuthenticatedUserFromRequest);
    int httpServerSocketFD = httpServer.GetSocketFD();
    httpServer.Start();


    RegisterTestRoutes(httpServer);
    RegisterUserRoutes(httpServer);
    RegisterStreamerRoutes(httpServer);
    RegisterViewerRoutes(httpServer);
    RegisterHealthRoutes(httpServer);
    httpServer.DebugHttpRoutes();

    AddToEpoll(epoll_fd, httpServerSocketFD);

    struct epoll_event ready_events[MAX_EVENT_BATCH];
    while (true)
    {
        // here we wait for any event from the socket
        int nfds = epoll_wait(epoll_fd, ready_events, MAX_EVENT_BATCH, -1);
        // printf("Epoll wait returned %d\n", nfds);
        // loop over the ready FDs (in this case we have only one)
        for (int i = 0; i < nfds; i++)
        {
            epoll_event& event = ready_events[i];

            if (event.data.fd == httpServerSocketFD)
            {
                // new connection
                httpServer.HandleEpollNewConnection(epoll_fd, event);
                std::cout << "I just accepted a new connection" << std::endl;
            }
            else
            {
                auto raw_conn = static_cast<Connection*>(event.data.ptr);

                // Elevate to a smart pointer to safely keep it alive
                std::shared_ptr<Connection> safe_conn = raw_conn->shared_from_this();

                appThreadPool.enqueue_detach([safe_conn, &httpServer,epoll_fd,event, events = event.events]()
                {
                    // Read first to parse the request and generate the response
                    if (events & EPOLLIN)
                    {
                        safe_conn->OnReadable();
                    }

                    // Write second so any response generated by the read gets sent out immediately
                    if (events & EPOLLOUT)
                    {
                        safe_conn->OnWritable(epoll_fd);
                    }

                    if (events & (EPOLLHUP | EPOLLERR))
                    {
                        httpServer.HandleClientClosedConnection(event);
                    }
                });
            }
        }
    }
}
