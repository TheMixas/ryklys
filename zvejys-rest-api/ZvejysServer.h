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

#include "./include/trie.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "RouteNode.h"

class HttpConnection;

class ZvejysServer {

    using HttpHandler = std::function<HttpResponse(const HttpRequest&)>;
    typedef trie::trie_map<char, RouteNode> RouteMap;
public:
    ZvejysServer(std::string host,int port) {
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
    int HandleEpollReceiveClientData (int epollFD, epoll_event event);
    int HandleEpollSendClientData (int epollFD, epoll_event event);

    RouteMap GetRouteMap() const {
        return route_map_;
    }

    //Getters and setters
    int GetSocketFD() const {
        return socket_fd_;
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

    std::unordered_map<int, std::unique_ptr<HttpConnection>> connections;

};


#endif //RYKLYS_BACKEND_SERVER_H