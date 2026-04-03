#pragma once
#include <string>
#include <vector>
#include <optional>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
class RedisClient {
public:
    RedisClient(const std::string& host, int port)
        : host_(host), port_(port), fd_(-1)
    {}

    RedisClient(const std::string& host, const std::string& port)
    : host_(host), port_(std::stoi(port)), fd_(-1)
    {}

    ~RedisClient() {
        disconnect();
    }

    bool connect() {
        struct addrinfo hints{}, *res;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        int err = getaddrinfo(host_.c_str(), std::to_string(port_).c_str(), &hints, &res);
        if (err != 0) {
            std::cerr << "[Redis] DNS resolve failed for " << host_ << ": " << gai_strerror(err) << std::endl;
            return false;
        }

        fd_ = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (fd_ < 0) {
            freeaddrinfo(res);
            return false;
        }

        if (::connect(fd_, res->ai_addr, res->ai_addrlen) < 0) {
            std::cerr << "[Redis] Connect failed to " << host_ << ":" << port_ << std::endl;
            close(fd_);
            fd_ = -1;
            freeaddrinfo(res);
            return false;
        }

        freeaddrinfo(res);
        return true;
    }

    void disconnect() {
        if (fd_ >= 0) {
            close(fd_);
            fd_ = -1;
        }
    }

    bool isConnected() const { return fd_ >= 0; }

    // --- Commands ---

    bool set(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mu_);
        sendCommand({"SET", key, value});
        auto reply = readLine();
        return reply == "+OK";
    }

    std::optional<std::string> get(const std::string& key) {
        std::lock_guard<std::mutex> lock(mu_);
        sendCommand({"GET", key});
        return readBulkString();
    }

    int incr(const std::string& key) {
        std::lock_guard<std::mutex> lock(mu_);
        sendCommand({"INCR", key});
        return readInteger();
    }

    int decr(const std::string& key) {
        std::lock_guard<std::mutex> lock(mu_);
        sendCommand({"DECR", key});
        return readInteger();
    }

    bool del(const std::string& key) {
        std::lock_guard<std::mutex> lock(mu_);
        sendCommand({"DEL", key});
        return readInteger() > 0;
    }

    // Publish returns the number of subscribers that received the message
    int publish(const std::string& channel, const std::string& message) {
        std::lock_guard<std::mutex> lock(mu_);
        sendCommand({"PUBLISH", channel, message});
        return readInteger();
    }

    // Subscribe — blocks the connection. Use on a dedicated thread.
    // Calls the callback for each message received.
    using SubCallback = std::function<void(const std::string& channel, const std::string& message)>;

    void subscribe(const std::string& channel, SubCallback callback) {
        sendCommand({"SUBSCRIBE", channel});
        // Read the subscribe confirmation (array of 3)
        readArray(); // discard confirmation

        // Now block and read messages forever
        while (fd_ >= 0) {
            auto arr = readArray();
            // Message array: ["message", channel, payload]
            if (arr.size() == 3 && arr[0] == "message") {
                callback(arr[1], arr[2]);
            }
        }
    }

    void psubscribe(const std::string& pattern, SubCallback callback) {
        sendCommand({"PSUBSCRIBE", pattern});
        readArray(); // discard subscription confirmation

        while (fd_ >= 0) {
            auto arr = readArray();
            // pmessage: ["pmessage", pattern, channel, payload]
            if (arr.size() == 4 && arr[0] == "pmessage") {
                callback(arr[2], arr[3]);
            }
        }
    }

    void subscribeMultiple(const std::vector<std::string>& channels, SubCallback callback) {
        std::vector<std::string> cmd = {"SUBSCRIBE"};
        for (auto& ch : channels) cmd.push_back(ch);
        sendCommand(cmd);

        // Read confirmations
        for (size_t i = 0; i < channels.size(); i++) {
            readArray();
        }

        while (fd_ >= 0) {
            auto arr = readArray();
            if (arr.size() == 3 && arr[0] == "message") {
                callback(arr[1], arr[2]);
            }
        }
    }

    // Get the raw fd for external close (to break out of subscribe)
    int getFd() const { return fd_; }

private:
    std::mutex mu_;
    std::string host_;
    int port_;
    int fd_;

    // Encode a command as RESP array
    void sendCommand(const std::vector<std::string>& args) {
        std::string cmd = "*" + std::to_string(args.size()) + "\r\n";
        for (auto& arg : args) {
            cmd += "$" + std::to_string(arg.size()) + "\r\n";
            cmd += arg + "\r\n";
        }

        size_t total = 0;
        while (total < cmd.size()) {
            ssize_t sent = send(fd_, cmd.data() + total, cmd.size() - total, MSG_NOSIGNAL);
            if (sent <= 0) {
                std::cerr << "[Redis] Send failed" << std::endl;
                disconnect();
                return;
            }
            total += sent;
        }
    }

    // Read until \r\n
    std::string readLine() {
        std::string line;
        char c;
        while (true) {
            ssize_t n = recv(fd_, &c, 1, 0);
            if (n <= 0) {
                disconnect();
                return "";
            }
            if (c == '\r') {
                recv(fd_, &c, 1, 0); // consume \n
                return line;
            }
            line += c;
        }
    }

    // Read exactly n bytes
    std::string readBytes(size_t count) {
        std::string buf(count, '\0');
        size_t total = 0;
        while (total < count) {
            ssize_t n = recv(fd_, buf.data() + total, count - total, 0);
            if (n <= 0) {
                disconnect();
                return "";
            }
            total += n;
        }
        // Consume trailing \r\n
        char crlf[2];
        recv(fd_, crlf, 2, 0);
        return buf;
    }

    // Read a RESP integer reply (:123\r\n)
    int readInteger() {
        std::string line = readLine();
        if (line.empty() || line[0] != ':') return 0;
        return std::stoi(line.substr(1));
    }

    // Read a RESP bulk string ($5\r\nhello\r\n or $-1\r\n for nil)
    std::optional<std::string> readBulkString() {
        std::string line = readLine();
        if (line.empty()) return std::nullopt;

        if (line[0] == '$') {
            int len = std::stoi(line.substr(1));
            if (len < 0) return std::nullopt; // nil
            return readBytes(len);
        }
        // Simple string (+OK)
        if (line[0] == '+') return line.substr(1);
        // Error
        if (line[0] == '-') {
            std::cerr << "[Redis] Error: " << line.substr(1) << std::endl;
            return std::nullopt;
        }
        return std::nullopt;
    }

    // Read a RESP array (*3\r\n...) — returns vector of strings
    std::vector<std::string> readArray() {
        std::string line = readLine();
        std::vector<std::string> result;

        if (line.empty() || line[0] != '*') return result;

        int count = std::stoi(line.substr(1));
        for (int i = 0; i < count; i++) {
            auto val = readBulkString();
            result.push_back(val.value_or(""));
        }
        return result;
    }
};