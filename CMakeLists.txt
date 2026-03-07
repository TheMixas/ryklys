cmake_minimum_required(VERSION 4.1)
project(ryklys_backend)

set(CMAKE_CXX_STANDARD 20)

add_executable(ryklys_backend main.cpp
        zvejys-rest-api/ZvejysServer.cpp
        zvejys-rest-api/HttpConnection.cpp
        zvejys-rest-api/HttpConnection.h
        zvejys-rest-api/HttpRequest.h
        zvejys-rest-api/HttpResponse.h
        zvejys-rest-api/HttpMethod.h
        zvejys-rest-api/RouteNode.h
        pg-connection-pool/pgPool.cpp
        pg-connection-pool/pgPool.h
        include/ptsouchlos_thread-pool/thread_pool.h
        zvejys-rest-api/WebSocketUtils.h
        zvejys-rest-api/WebSocketFrame.h
        zvejys-rest-api/WebSocketConnection.h
        zvejys-rest-api/Connection.h
)

add_subdirectory(include/libpqxx build-pqxx)
target_link_libraries(ryklys_backend     PRIVATE pqxx)
