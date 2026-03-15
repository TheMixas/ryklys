//
// Created by themi on 2/16/26.
//

#ifndef RYKLYS_BACKEND_SERVER_H
#define RYKLYS_BACKEND_SERVER_H
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <functional>
#include <arpa/inet.h>
#include <sstream>
#include <unordered_map>

#include <sys/types.h>
#include <memory>
#include <unordered_set>
#include <variant>

#include "./include/PhamPhiLong_Radix-Tree/radix_tree.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "RouteNode.h"
#include "WebSocketConnection.h"
#include "HttpConnection.h"
#include "../types/AuthenticatedUser.h"
#include "cors/Cors.h"
class WebSocketConnection;


class ZvejysServer {
    using WsHandlerSetup = std::function<void(WebSocketConnection &)>;
    using HttpHandler = std::function<HttpResponse(const HttpRequest &)>;
    using AuthenticatedHttpHandler = std::function<HttpResponse(const HttpRequest &, const AuthenticatedUser &)>;
    using ExtractAuthUserFromRequest = std::function<std::optional<AuthenticatedUser>(const HttpRequest &)>;
    typedef phamphilong::radix_tree<std::string, std::shared_ptr<RadixTreeNode> > RouteRadixTree;

public:
    ZvejysServer(std::string host, int port, CorsConfig &corsConfig,
                 ExtractAuthUserFromRequest authFunc) : cors(std::move(corsConfig)), auth_func_(std::move(authFunc)) {
        this->host = std::move(host);
        this->port_ = port;
        CreateTCPSocket();
        SetSocketToNonBlocking();
    }

    ~ZvejysServer() = default;

    void Start();

    void RegisterRoute(HttpMethod httpMethod, const std::string &path, const HttpHandler &handler);

    void RegisterAuthenticatedRoute(HttpMethod httpMethod, const std::string &path, AuthenticatedHttpHandler handler);

    //Epoll
    int HandleEpollNewConnection(int epollFD, epoll_event event);

    int HandleClientClosedConnection(epoll_event event);

    int HandleEpollReceiveClientData(int epollFD, epoll_event event);

    int HandleEpollSendClientData(int epollFD, epoll_event event);

    Cors &GetCors() {
        return cors;
    }

    const RouteRadixTree &GetRouteMap() const {
        return route_map_;
    }

    //Getters and setters
    int GetSocketFD() const {
        return socket_fd_;
    }

    //WebSockets

    // Register a WebSocket route. The setup callback configures
    // on_message/on_close on the newly-upgraded connection.
    void RegisterWebSocketRoute(const std::string &path, WsHandlerSetup setup) {
        ws_routes_[path] = std::move(setup);
    }

    // Called by HttpConnection after it sends the 101 handshake
    void UpgradeToWebSocket(int fd, int epollFD, const std::string &path) {
        // Remove the old HttpConnection
        connections.erase(fd);

        // Create a WebSocketConnection
        auto ws_conn = std::make_unique<WebSocketConnection>(fd, this, epollFD);

        // Apply the user-registered setup callback for this path
        auto it = ws_routes_.find(path);
        if (it != ws_routes_.end()) {
            it->second(*ws_conn);
        }


        ws_connections_[fd] = std::move(ws_conn);

        // Re-register with epoll, pointing to our new WebSocketConnection
        epoll_event ev{};
        ev.data.ptr = ws_connections_[fd].get();
        ev.events = EPOLLIN | EPOLLET;
        epoll_ctl(epollFD, EPOLL_CTL_MOD, fd, &ev);
    }

private:
    std::string host;
    int port_;
    int socket_fd_;
    int epoll_fd_;

    sockaddr_in server_address;

    RouteRadixTree route_map_;

    void CreateTCPSocket();

    void SetSocketToNonBlocking();

    const int SOCKET_DOMAIN = AF_INET; // IPv4
    const int SOCKET_TYPE = SOCK_STREAM | SOCK_NONBLOCK; // TCP socket with non-blocking mode
    const int SOCKET_PROTOCOL = 0; // Default protocol for our SOCK_STREAM socket type.

    const int REUSE_ADDRESS_AND_PORT = 1; // Option to allow reusing address and port

    const int BACKLOG_SIZE = 128; // Maximum number of pending connections in the queue

    std::unordered_map<int, std::unique_ptr<HttpConnection> > connections;

    //Webscokets
    std::unordered_map<std::string, WsHandlerSetup> ws_routes_;
    std::unordered_map<int, std::unique_ptr<WebSocketConnection> > ws_connections_;

    Cors cors;


    // Function provided at server construction that extracts an AuthenticatedUser
    // from an incoming request (e.g. by validating a JWT). Authenticated route
    // handlers invoke this to resolve the caller's identity before dispatching.
    ExtractAuthUserFromRequest auth_func_;
};


#endif //RYKLYS_BACKEND_SERVER_H
