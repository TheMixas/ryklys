cmake_minimum_required(VERSION 3.20)
set(CMAKE_PREFIX_PATH "/usr/local" ${CMAKE_PREFIX_PATH})
project(ryklys_backend)
set(CMAKE_CXX_STANDARD 20)
find_package(libpqxx REQUIRED)
find_package(OpenSSL REQUIRED)

find_package(LibDataChannel REQUIRED)
set(PQXX /usr/local/include/pqxx)

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
        routes/test-route/TestRoutes.cpp
        routes/test-route/TestRoutes.h
        routes/user-route/UserRoutes.cpp
        routes/user-route/UserRoutes.h
        routes/user-route/UserRoutes.cpp
        database/database_repositories/Repository.h
        database/database_repositories/UserRepository.cpp
        database/database_repositories/UserRepository.h
        types/User.h
        zvejys-rest-api/utils/Validation.h
        zvejys-rest-api/utils/Validation.h
        zvejys-rest-api/utils/PasswordHash.h
        zvejys-rest-api/utils/JsonWebToken.h
        zvejys-rest-api/utils/Crypto.h
        zvejys-rest-api/env/EnvConfig.h
        types/AuthenticatedUser.h
        ServiceLocator.h
        zvejys-rest-api/cors/Cors.cpp
        zvejys-rest-api/cors/Cors.h
        routes/stream-route/StreamerRoutes.cpp
        routes/stream-route/StreamerRoutes.h
        config/WebRTCConfig.h
        main.cpp
        main.cpp
        main.cpp
        services/SegmentUploader.h
        types/Stream.h
        database/database_repositories/StreamRepository.cpp
        database/database_repositories/StreamRepository.h
        services/RedisClient.h
        routes/viewer-route/VIewerRoutes.cpp
        routes/viewer-route/VIewerRoutes.h
        RedisPool.h
        services/ViewerRegistry.h
        services/ChatRelay.h
        database/database_repositories/ChatRepository.h
        services/StreamCleanup.h
        routes/health-route/HealthRoutes.cpp
        routes/health-route/HealthRoutes.h
)
target_compile_definitions(ryklys_backend PRIVATE PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
find_package(CURL REQUIRED)
target_link_libraries(ryklys_backend crypt libpqxx::pqxx LibDataChannel::LibDataChannel CURL::libcurl)

find_program(FFMPEG_EXECUTABLE ffmpeg)

if(NOT FFMPEG_EXECUTABLE)
    message(WARNING "FFmpeg was not found. It is required at runtime, not build time.\n"
            "Make sure FFmpeg is installed in your runtime environment.")
else()
    message(STATUS "Found FFmpeg: ${FFMPEG_EXECUTABLE}")
endif()

add_subdirectory(stream-pocket)