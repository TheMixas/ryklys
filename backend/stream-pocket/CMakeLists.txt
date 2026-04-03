cmake_minimum_required(VERSION 3.20)
project(stream_pocket)
set(CMAKE_CXX_STANDARD 20)

find_package(OpenSSL REQUIRED)

# Reuse the Zvejys HTTP framework from the main project
set(ZVEJYS_DIR ${CMAKE_SOURCE_DIR}/zvejys-rest-api)
add_executable(stream_pocket
        main.cpp
        ${ZVEJYS_DIR}/ZvejysServer.cpp
        ${ZVEJYS_DIR}/HttpConnection.cpp
        ${ZVEJYS_DIR}/cors/Cors.cpp
        routes/health-route/HealthRoutes.cpp
        routes/health-route/HealthRoutes.h
        routes/segments-route/SegmentsRoutes.cpp
        routes/segments-route/SegmentsRoutes.h
        StorageConfig.h
)

target_include_directories(stream_pocket PRIVATE
        ${CMAKE_SOURCE_DIR}             # backend root (for config/, types/, etc)
        ${ZVEJYS_DIR}                   # ZvejysServer headers
)

target_compile_definitions(stream_pocket PRIVATE
        PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(stream_pocket OpenSSL::SSL OpenSSL::Crypto)