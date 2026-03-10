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
        routes/TestRoute/TestRoutes.cpp
        routes/TestRoute/TestRoutes.h
        routes/UserRoute/UserRoutes.cpp
        routes/UserRoute/UserRoutes.h
        routes/UserRoute/UserRoutes.cpp
        database/database_repositories/Repository.h
        database/database_repositories/UserRepository.cpp
        database/database_repositories/UserRepository.h
        types/User.h
        zvejys-rest-api/utils/Validation.h
        zvejys-rest-api/utils/Validation.h
        zvejys-rest-api/utils/PasswordHash.h
        zvejys-rest-api/utils/JsonWebToken.h
        zvejys-rest-api/utils/Crypto.h
        config/EnvConfig.h
        types/AuthenticatedUser.h
        ServiceLocator.h
)
target_compile_definitions(ryklys_backend PRIVATE PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
add_subdirectory(include/libpqxx build-pqxx)
target_link_libraries(ryklys_backend     PRIVATE crypt pqxx)
