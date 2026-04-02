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
#include <iostream>
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


class ZvejysServer
{
    using WsHandlerSetup = std::function<void(std::shared_ptr<WebSocketConnection>)>;
    using HttpHandler = std::function<HttpResponse(const HttpRequest&)>;
    using AuthenticatedHttpHandler = std::function<HttpResponse(const HttpRequest&, const AuthenticatedUser&)>;
    using ExtractAuthUserFromRequest = std::function<std::optional<AuthenticatedUser>(const HttpRequest&)>;
    typedef phamphilong::radix_tree<std::string, std::shared_ptr<PathTreeNode>> RouteRadixTree;
    using AuthenticatedWsHandlerSetup = std::function<void(std::shared_ptr<WebSocketConnection>,
                                                           const AuthenticatedUser&)>;

public:
    ZvejysServer(std::string host, int port, CorsConfig& corsConfig,
                 ExtractAuthUserFromRequest authFunc) : cors(std::move(corsConfig)), auth_func_(std::move(authFunc))
    {
        this->host = std::move(host);
        this->port_ = port;
        CreateTCPSocket();
        SetSocketToNonBlocking();
    }

    ~ZvejysServer() = default;

    void Start();

    void RegisterRoute(HttpMethod httpMethod, const std::string& path, const HttpHandler& handler);

    void RegisterAuthenticatedRoute(HttpMethod httpMethod, const std::string& path, AuthenticatedHttpHandler handler);

    //Set path params
    static bool MatchPattern(const std::string& pattern, const std::string& path,
                             std::unordered_map<std::string, std::string>& params)
    {
        // Split both by '/'
        std::vector<std::string> patternParts, pathParts;

        std::istringstream ps(pattern), rs(path);
        std::string seg;
        while (std::getline(ps, seg, '/')) if (!seg.empty()) patternParts.push_back(seg);
        while (std::getline(rs, seg, '/')) if (!seg.empty()) pathParts.push_back(seg);

        if (patternParts.size() != pathParts.size())
            return false;

        for (size_t i = 0; i < patternParts.size(); i++)
        {
            if (patternParts[i].front() == '{' && patternParts[i].back() == '}')
            {
                // Extract param name without braces
                std::string key = patternParts[i].substr(1, patternParts[i].size() - 2);
                params[key] = pathParts[i];
            }
            else if (patternParts[i] != pathParts[i])
            {
                return false;
            }
        }
        return true;
    }

    //Epoll
    int HandleEpollNewConnection(int epollFD, epoll_event event);

    int HandleClientClosedConnection(epoll_event event);

    int HandleEpollReceiveClientData(int epollFD, epoll_event event);

    int HandleEpollSendClientData(int epollFD, epoll_event event);

    Cors& GetCors()
    {
        return cors;
    }

    const RouteRadixTree& GetRouteMap() const
    {
        return route_map_;
    }

    const std::vector<std::pair<std::string, std::shared_ptr<PathTreeNode>>>& GetPatternRoutes() const
    {
        return pattern_routes_;
    }

    //Getters and setters
    int GetSocketFD() const
    {
        return socket_fd_;
    }

    //WebSockets

    // Register a WebSocket route. The setup callback configures
    // on_message/on_close on the newly-upgraded connection.
    void RegisterWebSocketRoute(const std::string& path, WsHandlerSetup setup)
    {
        ws_routes_[path] = std::move(setup);
    }

    void RegisterAuthenticatedWebSocketRoute(const std::string& path, AuthenticatedWsHandlerSetup setup)
    {
        auth_ws_routes_[path] = std::move(setup);
    }

    // Called by HttpConnection after it sends the 101 handshake
    void UpgradeToWebSocket(int fd, int epollFD, HttpRequest http_request)
    {
        connections.erase(fd);

        auto ws_conn = std::make_shared<WebSocketConnection>(fd, this, epollFD, std::move(http_request));

        // Store and register with epoll FIRST, before any handler logic
        ws_connections_[fd] = ws_conn;

        epoll_event ev{};
        ev.data.ptr = ws_connections_[fd].get();
        ev.events = EPOLLIN | EPOLLET;
        epoll_ctl(epollFD, EPOLL_CTL_MOD, fd, &ev);

        const std::string& path = ws_conn->GetHttpRequest().path;

        auto auth_it = auth_ws_routes_.find(path);
        if (auth_it != auth_ws_routes_.end())
        {
            if (!auth_func_)
            {
                ws_conn->Close(1008, "Auth not configured");
                return;
            }

            auto authUser = auth_func_(ws_conn->GetHttpRequest());
            if (!authUser.has_value())
            {
                ws_conn->Close(1008, "Unauthorized");
                return;
            }

            auth_it->second(ws_conn, authUser.value());
        }
        else
        {
            auto it = ws_routes_.find(path);
            if (it != ws_routes_.end())
            {
                it->second(ws_conn);
            }
            else
            {
                std::cerr << "No WebSocket route registered for path: " << path << std::endl;
                ws_conn->Close(1008, "No route");
            }
        }
    }

    //debug
    void DebugHttpRoutes();

private:
    std::string host;
    int port_;
    int socket_fd_;
    int epoll_fd_;

    sockaddr_in server_address;

    RouteRadixTree route_map_;
    std::vector<std::pair<std::string, std::shared_ptr<PathTreeNode>>> pattern_routes_;

    void CreateTCPSocket();

    void SetSocketToNonBlocking();

    const int SOCKET_DOMAIN = AF_INET; // IPv4
    const int SOCKET_TYPE = SOCK_STREAM | SOCK_NONBLOCK; // TCP socket with non-blocking mode
    const int SOCKET_PROTOCOL = 0; // Default protocol for our SOCK_STREAM socket type.

    const int REUSE_ADDRESS_AND_PORT = 1; // Option to allow reusing address and port

    const int BACKLOG_SIZE = 128; // Maximum number of pending connections in the queue


    //Webscokets
    std::unordered_map<std::string, WsHandlerSetup> ws_routes_;
    std::unordered_map<std::string, AuthenticatedWsHandlerSetup> auth_ws_routes_;
    std::unordered_map<int, std::shared_ptr<HttpConnection>> connections;
    std::unordered_map<int, std::shared_ptr<WebSocketConnection>> ws_connections_;

    Cors cors;


    // Function provided at server construction that extracts an AuthenticatedUser
    // from an incoming request (e.g. by validating a JWT). Authenticated route
    // handlers invoke this to resolve the caller's identity before dispatching.
    ExtractAuthUserFromRequest auth_func_;
};


#endif //RYKLYS_BACKEND_SERVER_H
