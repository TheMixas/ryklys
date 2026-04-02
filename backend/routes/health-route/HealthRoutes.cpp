//
// Created by gusbunto on 4/2/26.
//
#pragma once
#include "HealthRoutes.h"
#include "./../../zvejys-rest-api/HttpMethod.h"
#include "./../../zvejys-rest-api/HttpResponse.h"
#include "./../../zvejys-rest-api/HttpRequest.h"
#include "./../../zvejys-rest-api/env/EnvConfig.h"



#include <filesystem>

void RegisterHealthRoutes(ZvejysServer& server)
{
    server.RegisterRoute(HttpMethod::GET, "/api/health", [](const HttpRequest& request) -> HttpResponse
    {
        std::cout << "[HEALTH] Health check hit on this node" << std::endl;
        std::string port = EnvConfig::Instance().Get("SERVER_PORT", "unknown");
        return HttpResponse::Json("{\"status\":\"ok\",\"port\":" + port + "}");
    });
}
