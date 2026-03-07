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

class ZvejysServer; // forward declaration

class HttpConnection : public Connection {
public:
    HttpConnection(int fd, ZvejysServer* server) : Connection(fd, server) {
    }
    ~HttpConnection() override = default;

    ZvejysServer* GetServer() const { return server_; }



    void OnReadable(int epollFD) override;
    void OnWritable(int epollFD) override;

private:
    std::vector<char> Read() const;
    HttpRequest Parse(const std::vector<char>&) const;
    static constexpr int READ_BUFFER_RESERVE = 16000; // 16KB, which is more than enough for a single HTTP request
    static constexpr int MAX_READ_CHUNK_SIZE = 1024; // Read in chunks of 1KB

    std::queue<HttpResponse> response_queue_;
};


#endif //RYKLYS_BACKEND_CONNECTION_H