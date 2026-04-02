#ifndef RYKLYS_BACKEND_WEBSOCKETCONNECTION_H
#define RYKLYS_BACKEND_WEBSOCKETCONNECTION_H

#include <cstdio>
#include <functional>
#include <queue>
#include <string>
#include <vector>

#include <cerrno>
#include <mutex>
#include <sys/socket.h>
#include <sys/epoll.h>

#include "Connection.h"
#include "HttpRequest.h"
#include "WebSocketFrame.h"

class ZvejysServer;

enum class WsState {
    OPEN,
    CLOSING,
    CLOSED
};

class WebSocketConnection : public Connection{
public:
    // Callbacks the user can set
    using OnMessageCallback = std::function<void(std::shared_ptr<WebSocketConnection>, const std::vector<uint8_t> &, WsOpcode)>;
    using OnCloseCallback = std::function<void(std::shared_ptr<WebSocketConnection>, uint16_t,const std::string &reason)>;

    WebSocketConnection(int fd, ZvejysServer *server, int epollFd, HttpRequest http_request) : Connection(fd, server, epollFd) {
        state_ = WsState::OPEN;
        request_ = std::move(http_request);
    }

    // --- Public API for sending ---

    void SendText(const std::string &message) {
        std::lock_guard<std::mutex> lock(write_mu_);
        WebSocketFrame frame;
        frame.fin = true;
        frame.opcode = WsOpcode::TEXT;
        frame.payload.assign(message.begin(), message.end());
        QueueFrame(frame);
    }

    void SendBinary(const std::vector<uint8_t> &data) {
        std::lock_guard<std::mutex> lock(write_mu_);
        WebSocketFrame frame;
        frame.fin = true;
        frame.opcode = WsOpcode::BINARY;
        frame.payload = data;
        QueueFrame(frame);
    }

    void SendPing(const std::vector<uint8_t> &data = {}) {
        std::lock_guard<std::mutex> lock(write_mu_);
        WebSocketFrame frame;
        frame.fin = true;
        frame.opcode = WsOpcode::PING;
        frame.payload = data;
        QueueFrame(frame);
    }

    void Close(uint16_t code = 1000, const std::string &reason = "") {
        std::lock_guard<std::mutex> lock(write_mu_);
        if (state_ != WsState::OPEN) return;
        state_ = WsState::CLOSING;

        WebSocketFrame frame;
        frame.fin = true;
        frame.opcode = WsOpcode::CLOSE;
        // Close frame payload: 2-byte status code + reason
        frame.payload.push_back(static_cast<uint8_t>((code >> 8) & 0xFF));
        frame.payload.push_back(static_cast<uint8_t>(code & 0xFF));
        frame.payload.insert(frame.payload.end(), reason.begin(), reason.end());
        QueueFrame(frame);
    }

    // Reads raw bytes, feeds them to the frame parser, dispatches callbacks.
    void OnReadable() override{
        std::unique_lock<std::mutex> lock(conn_mutex_);
        // Read available data from socket into read_buffer_
        uint8_t chunk[4096];
        while (true) {
            ssize_t n = recv(socket_fd_, chunk, sizeof(chunk), 0);
            if (n > 0) {
                read_buffer_.insert(read_buffer_.end(), chunk, chunk + n);
            } else if (n == 0) {
                // Peer closed
                state_ = WsState::CLOSED;
                return;
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) break; // No more data for now
                perror("WebSocketConnection::HandleRead");
                return;
            }
        }

        // Try to parse complete frames from the buffer
        while (!read_buffer_.empty()) {
            size_t consumed = 0;
            auto maybe_frame = WebSocketFrame::Parse(
                read_buffer_.data(), read_buffer_.size(), consumed);

            if (!maybe_frame.has_value()) break; // incomplete frame, wait for more data

            const WebSocketFrame &frame = maybe_frame.value();

            switch (frame.opcode) {
                case WsOpcode::TEXT:
                case WsOpcode::BINARY:
                    // TODO: Handle fragmentation (CONTINUATION frames) if needed
                    if (on_message_) {
                        on_message_(std::static_pointer_cast<WebSocketConnection>(shared_from_this()), frame.payload, frame.opcode);
                    }
                    break;

                case WsOpcode::PING: {
                    // Respond with Pong, echoing the payload
                    WebSocketFrame pong;
                    pong.fin = true;
                    pong.opcode = WsOpcode::PONG;
                    pong.payload = frame.payload;
                    QueueFrame(pong);
                    break;
                }

                case WsOpcode::PONG:
                    // Could track latency here; for now, ignore
                    break;

                case WsOpcode::CLOSE: {
                    uint16_t code = 1005; // No status code
                    std::string reason;
                    if (frame.payload.size() >= 2) {
                        code = (frame.payload[0] << 8) | frame.payload[1];
                        if (frame.payload.size() > 2) {
                            reason.assign(frame.payload.begin() + 2, frame.payload.end());
                        }
                    }
                    if (state_ == WsState::OPEN) {
                        // Peer initiated close — echo it back
                        Close(code, reason);
                    }
                    state_ = WsState::CLOSED;
                    if (on_close_) {
                        on_close_(std::static_pointer_cast<WebSocketConnection>(shared_from_this()), code, reason);
                    }
                    break;
                }

                default:
                    break;
            }

            // Remove consumed bytes from the front of the buffer
            read_buffer_.erase(read_buffer_.begin(), read_buffer_.begin() + consumed);
        }

        // Arm for writing if we have frames to send
        if (!write_queue_.empty()) {
            ArmForWrite(epollFD);
        }
    }

    // Called when EPOLLOUT fires. Flushes write_queue_ to the socket.
    void OnWritable(int epollFD) override{\
        std::lock_guard<std::mutex> lock(write_mu_);
        while (!write_queue_.empty()) {
            const auto &data = write_queue_.front();
            ssize_t sent = send(socket_fd_, data.data() + write_offset_,
                                data.size() - write_offset_, MSG_NOSIGNAL);
            if (sent < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // Socket buffer full, wait for next EPOLLOUT
                    return;
                }
                perror("WebSocketConnection::HandleWrite");
                state_ = WsState::CLOSED;
                return;
            }
            write_offset_ += sent;
            if (write_offset_ >= data.size()) {
                write_queue_.pop();
                write_offset_ = 0;
            }
        }

        // All data sent, switch back to reading
        ArmForRead(epollFD);
    }

    // --- Setters for callbacks ---
    void SetOnMessage(OnMessageCallback cb) { on_message_ = std::move(cb); }
    void SetOnClose(OnCloseCallback cb) { on_close_ = std::move(cb); }

    HttpRequest GetHttpRequest() const { return request_; }
    int GetSocketFD() const { return socket_fd_; }
    WsState GetState() const { return state_; }
    ZvejysServer *GetServer() const { return server_; }

private:
    std::mutex conn_mutex_;   // protects read_buffer_, state_
    std::mutex write_mu_;     // protects write_queue_, write_offset_
    WsState state_;

    std::vector<uint8_t> read_buffer_;
    std::queue<std::vector<uint8_t> > write_queue_;
    size_t write_offset_ = 0; // partial-write tracking within front of queue

    OnMessageCallback on_message_;
    OnCloseCallback on_close_;

    HttpRequest request_; // Store the initial HTTP request that led to this WebSocket connection, if needed for context

    void QueueFrame(const WebSocketFrame &frame) {
        write_queue_.push(frame.Serialize());
        ArmForWrite(epollFD);
    }

    void ArmForWrite(int epollFD) {
        epoll_event ev{};
        ev.data.ptr = this;
        ev.events = EPOLLOUT | EPOLLET;
        epoll_ctl(epollFD, EPOLL_CTL_MOD, socket_fd_, &ev);
    }

    void ArmForRead(int epollFD) {
        epoll_event ev{};
        ev.data.ptr = this;
        ev.events = EPOLLIN | EPOLLET;
        epoll_ctl(epollFD, EPOLL_CTL_MOD, socket_fd_, &ev);
    }
};

#endif // RYKLYS_BACKEND_WEBSOCKETCONNECTION_H
