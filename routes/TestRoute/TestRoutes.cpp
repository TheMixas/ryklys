//
// Created by themi on 3/8/26.
//

#include "./../TestRoute/TestRoutes.h"

#include <iostream>
#include "../../zvejys-rest-api/HttpResponse.h"
#include "../../zvejys-rest-api/ZvejysServer.h"

void RegisterTestRoutes(ZvejysServer &server) {
    server.RegisterRoute(HttpMethod::GET, "/5s", [](const HttpRequest &request) -> HttpResponse {
        std::cout << "I am handling a 5s" << std::endl;

        sleep(5);

        HttpResponse response = HttpResponse::Ok("This is a 5s response");
        return response;
    });
    server.RegisterRoute(HttpMethod::GET, "/2s", [](const HttpRequest &request) -> HttpResponse {
        std::cout << "I am handling a 5s" << std::endl;

        sleep(2);

        HttpResponse response;
        response.status_code = 200;
        response.body = std::vector<char>{'2', 's', ' ', ':', ')'};

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
    server.RegisterWebSocketRoute("/ws", [](WebSocketConnection &ws) {
        ws.SetOnMessage([](WebSocketConnection &conn,
                           const std::vector<uint8_t> &data,
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

        ws.SetOnClose([](WebSocketConnection &conn,
                         uint16_t code,
                         const std::string &reason) {
            std::cout << "WS closed: " << code << " " << reason << std::endl;
        });
    });

    server.RegisterWebSocketRoute("/ws2", [](WebSocketConnection &ws) {
        ws.SetOnMessage([](WebSocketConnection &conn,
                           const std::vector<uint8_t> &data,
                           WsOpcode opcode) {
            // Echo back whatever we receive
            if (opcode == WsOpcode::TEXT) {
                std::string msg(data.begin(), data.end());
                std::cout << "WS received: " << msg << std::endl;
                conn.SendText("Echo2: " + msg);
            } else {
                conn.SendBinary(data);
            }
        });

        ws.SetOnClose([](WebSocketConnection &conn,
                         uint16_t code,
                         const std::string &reason) {
            std::cout << "WS closed: " << code << " " << reason << std::endl;
        });
    });
}
