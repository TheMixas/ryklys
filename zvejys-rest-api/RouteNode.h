//
// Created by themi on 2/23/26.
//

#ifndef RYKLYS_BACKEND_ROUTENODE_H
#define RYKLYS_BACKEND_ROUTENODE_H
#include <functional>
#include <string>

#include "HttpRequest.h"
#include "HttpResponse.h"

class RouteNode {

public:
    RouteNode(HttpMethod method = HttpMethod::GET, std::function<HttpResponse(const HttpRequest&)> handlerFunc = nullptr) {
        routeMethod = method;
        handler = std::move(handlerFunc);
    }
    ~RouteNode() = default;

    HttpMethod routeMethod;
    std::function<HttpResponse(const HttpRequest&)> handler;
};
#endif //RYKLYS_BACKEND_ROUTENODE_H