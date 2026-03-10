//
// Created by themi on 2/23/26.
//

#ifndef RYKLYS_BACKEND_ROUTENODE_H
#define RYKLYS_BACKEND_ROUTENODE_H
#include <functional>
#include <string>

#include "HttpRequest.h"
#include "HttpResponse.h"

#include "./types/AuthenticatedUser.h"
#include "./ZvejysServer.h"

class RadixTreeNode {
public:
    RadixTreeNode(HttpMethod method) : routeMethod(method) {
    }

    virtual ~RadixTreeNode() = 0;

    virtual HttpResponse HandleRequest(const HttpRequest &request) = 0;

    HttpMethod GetRouteMethod() const {
        return routeMethod;
    }

private:
    HttpMethod routeMethod;
};

// Pure virtual destructor must have a definition
inline RadixTreeNode::~RadixTreeNode() = default;

class RouteNode : public RadixTreeNode {
    using HttpHandler = std::function<HttpResponse(const HttpRequest &)>;

public:
    RouteNode(HttpMethod method, HttpHandler handlerFunc) : RadixTreeNode(method) {
        handler = std::move(handlerFunc);
    }

    ~RouteNode() override = default;

    RouteNode(const RouteNode &obj)
        : RadixTreeNode(obj) {
        handler = obj.handler;
    }

    HttpResponse HandleRequest(const HttpRequest &request) override {
        if (handler) {
            return handler(request);
        } else {
            return HttpResponse::InternalServerError("Handler not implemented");
        }
    }

private:
    HttpHandler handler;
};

class AuthenticatedRouteNode : public RadixTreeNode {
    using AuthenticatedHttpHandler = std::function<HttpResponse(const HttpRequest &, const AuthenticatedUser &)>;
    using ExtractAuthUserFromRequest = std::function<std::optional<AuthenticatedUser>(const HttpRequest &)>;

public:
    AuthenticatedRouteNode(HttpMethod method,
                           AuthenticatedHttpHandler handlerFunc, // HABNDLER CANNOT BE CONST REF !!!!!! THE LAMBDAS DIE OFF  after the register func ends
                           const ExtractAuthUserFromRequest& authFunc)
        : RadixTreeNode(method), handler_(std::move(handlerFunc)), authFunc_(authFunc) {
    }

    // AuthenticatedRouteNode(const AuthenticatedRouteNode &obj)
    //     : RadixTreeNode(obj) {
    //     handler_ = obj.handler_;
    //     authFunc_ = obj.authFunc_;
    // }

    ~AuthenticatedRouteNode() override = default;

    HttpResponse HandleRequest(const HttpRequest &request) override {
        if (!handler_) {
            return HttpResponse::InternalServerError("Handler not implemented");
        }

        auto authUserOpt = authFunc_(request);
        if (!authUserOpt.has_value()) {
            return HttpResponse::Unauthorized("Invalid or missing token");
        }
        const AuthenticatedUser& authUser = authUserOpt.value();

        return handler_(request, authUser);
    }

private:
    AuthenticatedHttpHandler handler_;
    const ExtractAuthUserFromRequest& authFunc_;
};

// struct AuthenticatedRouteNode {
//     HttpMethod routeMethod;
//     std::function<HttpResponse(const HttpRequest &, const AuthenticatedUser &)> handler;
// };
#endif //RYKLYS_BACKEND_ROUTENODE_H
