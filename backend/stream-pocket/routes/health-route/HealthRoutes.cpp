//
// Created by gusbunto on 3/30/26.
//

#include "HealthRoutes.h"

#include "HttpMethod.h"
#include "ZvejysServer.h"

struct HttpResponse;
struct HttpRequest;

void RegisterHealthRoutes(ZvejysServer &server)
{
    server.RegisterRoute(HttpMethod::GET, "/api/health", [](const HttpRequest &request) -> HttpResponse
    {
        return HttpResponse::Ok("I`m alive");
    });
}
