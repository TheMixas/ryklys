#ifndef CONNECTION_H
#define CONNECTION_H
class ZvejysServer;

class Connection {
public:
    explicit Connection(int fd, ZvejysServer* server) : socket_fd_(fd), server_(server) {}
    virtual ~Connection() = default;

    virtual void OnReadable(int epollFD) = 0;
    virtual void OnWritable(int epollFD) = 0;


    int GetSocketFD() const { return socket_fd_; }

protected:
    int socket_fd_;
    ZvejysServer* server_; // non-owning pointer, server outlives connections

};
#endif // CONNECTION_H
