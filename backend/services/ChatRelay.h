#pragma once
#include "RedisClient.h"
#include "ViewerRegistry.h"
#include <thread>
#include <atomic>
#include <iostream>

class ChatRelay {
public:
    ChatRelay(const std::string& host, const std::string& port, ViewerRegistry& registry)
        : sub_(host, port), registry_(registry) {}

    bool Start() {
        if (!sub_.connect()) {
            std::cerr << "[ChatRelay] Failed to connect to Redis" << std::endl;
            return false;
        }

        running_.store(true);
        thread_ = std::thread([this]() {
            sub_.psubscribe("chat:*", [this](const std::string& channel, const std::string& message) {
                if (channel.size() <= 5) return;
                std::string username = channel.substr(5);
                registry_.Broadcast(username, message);
            });

            std::cout << "[ChatRelay] Subscriber thread exited" << std::endl;
        });

        std::cout << "[ChatRelay] Listening on chat:*" << std::endl;
        return true;
    }

    void Stop() {
        running_.store(false);
        // Closing the fd breaks the blocking recv inside psubscribe
        sub_.disconnect();
        if (thread_.joinable()) thread_.join();
    }

    ~ChatRelay() { Stop(); }

    ChatRelay(const ChatRelay&) = delete;
    ChatRelay& operator=(const ChatRelay&) = delete;

private:
    RedisClient sub_; // dedicated connection, never shared
    ViewerRegistry& registry_;
    std::thread thread_;
    std::atomic<bool> running_{false};
};