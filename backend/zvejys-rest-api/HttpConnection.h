//
// Created by themi on 2/19/26.
//

#ifndef RYKLYS_BACKEND_CONNECTION_H
#define RYKLYS_BACKEND_CONNECTION_H
#include <queue>
#include <vector>

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Connection.h"
#include "include/nlohmann/json.hpp"
class ZvejysServer; // forward declaration

class HttpConnection : public Connection {
    using json = nlohmann::json;
public:
    HttpConnection(int fd, ZvejysServer* server, int epollFd) : Connection(fd, server, epollFd) {
        read_buffer_.reserve(READ_BUFFER_RESERVE);
    }

    ~HttpConnection() override = default;

    ZvejysServer* GetServer() const { return server_; }


    void HandleRead(HttpRequest request);

    static int find_end_of_one_http_request(std::span<const char> buf);

    static int parse_content_length(std::span<const char> headers);

    void OnReadable() override;
    void OnWritable(int epollFD) override;

private:

    std::vector<char> Read() const;
    HttpRequest Parse(const std::vector<char>&) const;
    static constexpr int READ_BUFFER_RESERVE = 16000; // 16KB, which is more than enough for a single HTTP request
    static constexpr int MAX_READ_CHUNK_SIZE = 1024; // Read in chunks of 1KB

    std::queue<HttpResponse> response_queue_;

    std::vector<char> read_buffer_; // persistent across calls!
};


#endif //RYKLYS_BACKEND_CONNECTION_H