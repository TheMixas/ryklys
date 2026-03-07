#include <iostream>
#include <memory>
#include <unistd.h>
#include <unordered_map>

#include "include/ptsouchlos_thread-pool/thread_pool.h"
#include "pg-connection-pool/pgPool.h"
#include "zvejys-rest-api/HttpConnection.h"
#include "zvejys-rest-api/ZvejysServer.h"
const int MAX_EVENT_BATCH = 64; // Maximum number of epoll events to process in one batch
// static PGPool dbPool;
int threadPoolSize = std::thread::hardware_concurrency() * 2;
static dp::thread_pool pool(threadPoolSize);

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

void RegisterRoutes(ZvejysServer &server) {
    server.RegisterRoute(HttpMethod::GET, "/5s", [](const HttpRequest &request) -> HttpResponse {
        std::cout << "I am handling a 5s" << std::endl;

        sleep(5);

        HttpResponse response;
        response.status_code = 200;
        response.body = std::vector<char>{'5', 's', 'l', 'l', 'o'};

        return response;
    });
    // server.RegisterRoute(HttpMethod::GET, "/hello", [](const HttpRequest &request) -> HttpResponse {
    //     std::cout << "I am handling a request to /hello" << (&dbPool) << std::endl;
    //
    //     auto connection = dbPool.connection();
    //     sleep(5);
    //     pqxx::work W(*connection.get());
    //     int maxDEmoId = W.query_value<int>("SELECT max(id) FROM demo;");
    //     std::cout << "Max demo id: " << maxDEmoId << std::endl;
    //
    //     HttpResponse response;
    //     response.status_code = 200;
    //     response.body = std::vector<char>{'H', 'e', 'l', 'l', 'o'};
    //     W.commit();
    //
    //     // free connection when things done
    //     dbPool.freeConnection(connection);
    //     return response;
    // });

    // WebSocket echo handler
    server.RegisterWebSocketRoute("/ws", [](WebSocketConnection& ws) {
        ws.SetOnMessage([](WebSocketConnection& conn,
                           const std::vector<uint8_t>& data,
                           WsOpcode opcode) {
            // Echo back whatever we receive
            if (opcode == WsOpcode::TEXT) {
                std::string msg(data.begin(), data.end());
                std::cout << "WS received: " << msg << std::endl;
                conn.SendText("Echo: " + msg);
            } else {
                conn.SendBinary(data);
            }
        });

        ws.SetOnClose([](WebSocketConnection& conn,
                         uint16_t code,
                         const std::string& reason) {
            std::cout << "WS closed: " << code << " " << reason << std::endl;
        });
    });
}

int main() {
    pool.enqueue_detach([]() {
        std::cout << "I am handling a 5s 1" << std::endl;
        sleep(5);
        std::cout << "I finished handling a 5s" << std::endl;
    });
    pool.enqueue_detach([]() {
        std::cout << "I am handling a 5s 2" << std::endl;

        sleep(5);
        std::cout << "I finished handling a 5s 2" << std::endl;

    });
    std::cout << "Hello, World!" << std::endl;
    ZvejysServer httpServer("127.0.0.1", 8080);
    int httpServerSocketFD = httpServer.GetSocketFD();
    httpServer.Start();
    RegisterRoutes(httpServer);
    std::cout << "I  have routes: " << httpServer.GetRouteMap().size() << std::endl;
    std::cout << "Routes: " << std::endl;
    for (auto route = httpServer.GetRouteMap().begin(); route != httpServer.GetRouteMap().end(); ++route) {
        std::cout << route.key() << std::endl;
    }

    int epoll_fd = CreateEpoll();
    AddToEpoll(epoll_fd, httpServerSocketFD);

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
                auto conn = static_cast<Connection*>(event.data.ptr);
                if (event.events & EPOLLIN) {
                    pool.enqueue_detach([conn, epoll_fd]() {
                        conn->OnReadable(epoll_fd);
                    });
                }
                if (event.events & EPOLLOUT) {
                    pool.enqueue_detach([conn, epoll_fd]() {
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
                    pool.enqueue_detach([&httpServer, event]() {
                        httpServer.HandleClientClosedConnection(event);
                        std::cout << "Client disconnected or error occurred" << std::endl;
                    });
                }
            }
        }
    }

    return 0;
}
