#ifndef CONNECTION_H
#define CONNECTION_H
class ZvejysServer;
#include <memory>
class Connection : public std::enable_shared_from_this<Connection> {
public:
    explicit Connection(int fd, ZvejysServer* server, int epollFd) : socket_fd_(fd), server_(server),epollFD(epollFd) {}
    virtual ~Connection() = default;

    virtual void OnReadable() = 0;
    virtual void OnWritable(int epollFD) = 0;


    int GetSocketFD() const { return socket_fd_; }

protected:
    int socket_fd_;
    ZvejysServer* server_; // non-owning pointer, server outlives connections
    int epollFD;
    std::mutex conn_mutex_;
};
#endif // CONNECTION_H
