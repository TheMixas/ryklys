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
#include "./include/trie.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "RouteNode.h"
#include "WebSocketConnection.h"

class WebSocketConnection;
class HttpConnection;


class ZvejysServer {
    // A tagged wrapper so epoll data.ptr can tell us the connection type


    using WsHandlerSetup = std::function<void(WebSocketConnection &)>;
    using HttpHandler = std::function<HttpResponse(const HttpRequest &)>;
    typedef trie::trie_map<char, RouteNode> RouteMap;

public:
    ZvejysServer(std::string host, int port) {
        this->host = std::move(host);
        this->port_ = port;
        CreateTCPSocket();
        SetSocketToNonBlocking();
    }

    ~ZvejysServer() = default;

    void Start();

    void RegisterRoute(HttpMethod, std::string, HttpHandler);

    //Epoll
    int HandleEpollNewConnection(int epollFD, epoll_event event);

    int HandleClientClosedConnection(epoll_event event);

    int HandleEpollReceiveClientData(int epollFD, epoll_event event);

    int HandleEpollSendClientData(int epollFD, epoll_event event);

    RouteMap GetRouteMap() const {
        return route_map_;
    }

    //Getters and setters
    int GetSocketFD() const {
        return socket_fd_;
    }

    //WebSockets

    // Register a WebSocket route. The setup callback configures
    // on_message/on_close on the newly-upgraded connection.
    void RegisterWebSocketRoute(const std::string& path, WsHandlerSetup setup) {
        ws_routes_[path] = std::move(setup);
    }

    // Called by HttpConnection after it sends the 101 handshake
    void UpgradeToWebSocket(int fd, int epollFD, const std::string& path) {
        // Remove the old HttpConnection
        connections.erase(fd);

        // Create a WebSocketConnection
        auto ws_conn = std::make_unique<WebSocketConnection>(fd, this);

        // Apply the user-registered setup callback for this path
        auto it = ws_routes_.find(path);
        if (it != ws_routes_.end()) {
            it->second(*ws_conn);
        }

        // Store with a tagged wrapper
        ws_connections_[fd] = std::move(ws_conn);

        // Re-register with epoll, pointing to our tagged wrapper
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

    RouteMap route_map_;

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
};


#endif //RYKLYS_BACKEND_SERVER_H
