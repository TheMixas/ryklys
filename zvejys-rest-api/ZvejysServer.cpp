//
// Created by themi on 2/16/26.
//

#include "ZvejysServer.h"

#include <iostream>
#include <stdexcept>
#include <utility>

#include "HttpConnection.h"

void ZvejysServer::CreateTCPSocket() {
    if ((socket_fd_ = socket(SOCKET_DOMAIN, SOCKET_TYPE, SOCKET_PROTOCOL)) < 0) {
        throw std::runtime_error("Failed to create a TCP socket");
    }
}

void ZvejysServer::SetSocketToNonBlocking() {
    int flags = fcntl(GetSocketFD(), F_GETFL, 0);
    fcntl(GetSocketFD(), F_SETFL, flags | O_NONBLOCK);
}

void ZvejysServer::Start() {
    sockaddr_in server_address;

    // Allow reusing address and port
    if (setsockopt(GetSocketFD(), SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &REUSE_ADDRESS_AND_PORT,
                   sizeof(REUSE_ADDRESS_AND_PORT)) < 0) {
        throw std::runtime_error("Failed to set socket options");
    }

    server_address.sin_family = SOCKET_DOMAIN;
    server_address.sin_addr.s_addr = INADDR_ANY; // Accept connections from any address

    // Convert host and port to binary form and update server_address
    inet_pton(AF_INET, host.c_str(), &(server_address.sin_addr.s_addr));
    server_address.sin_port = htons(port_);

    if (bind(GetSocketFD(), (sockaddr *)&server_address, sizeof(server_address)) < 0) {
        throw std::runtime_error("Failed to bind to socket");
    }

    if (listen(GetSocketFD(), BACKLOG_SIZE) < 0) {
        throw std::runtime_error("Failed to listen on port " + std::to_string(port_));
    }

    std::cout << "Server is listening on " << host << ":" << port_ << std::endl;
}

void ZvejysServer::RegisterRoute(HttpMethod httpMethod,const std::string &path, const HttpHandler& handler) {
    auto handlerNode = std::make_shared<RouteNode>(httpMethod,handler);
    route_map_.insert(std::pair<const std::string, std::shared_ptr<RadixTreeNode>>(path, std::move(handlerNode)));
}

void ZvejysServer::RegisterAuthenticatedRoute(HttpMethod httpMethod,const std::string &path, AuthenticatedHttpHandler handler) {
    auto authenticatedRouteNode = std::make_shared<AuthenticatedRouteNode>(httpMethod,std::move(handler), auth_func_);
    route_map_.insert(std::pair<const std::string, std::shared_ptr<RadixTreeNode>>(path, std::move(authenticatedRouteNode)));
}

int ZvejysServer::HandleEpollNewConnection(int epollFD, epoll_event event) {
    int client_socket_fd = accept4(event.data.fd, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (client_socket_fd < 0) {
        perror("Failed to accept new connection");
        return -1;
    }

    // Register the new client socket with epoll
    struct epoll_event client_event;
    auto conn = std::make_unique<HttpConnection>(client_socket_fd, this, epollFD);
    client_event.data.ptr = conn.get();
    connections[client_socket_fd] = std::move(conn);
    client_event.events = EPOLLIN | EPOLLET; // Edge-triggered read events

    if (epoll_ctl(epollFD, EPOLL_CTL_ADD, client_socket_fd, &client_event) < 0) {
        perror("Failed to add client socket to epoll");
        close(client_socket_fd);
        connections.erase(client_socket_fd);
    }
    return client_socket_fd;
}

int ZvejysServer::HandleClientClosedConnection(epoll_event event) {
    // Handle disconnection or error
    auto *conn = static_cast<HttpConnection *>(event.data.ptr);
    int fd = conn->GetSocketFD();
    close(fd);
    connections.erase(fd);
    return 1;
}

int ZvejysServer::HandleEpollReceiveClientData(int epollFD, epoll_event event) {
    auto *conn = static_cast<HttpConnection *>(event.data.ptr);
    conn->OnReadable();
    return 1;
}

int ZvejysServer::HandleEpollSendClientData(int epollFD, epoll_event event) {
    auto *conn = static_cast<HttpConnection *>(event.data.ptr);
    conn->OnWritable(epollFD);
    return 1;
}
