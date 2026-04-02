#pragma once
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <stdexcept>

#include "services/RedisClient.h"

class RedisPool {
public:
    RedisPool(const std::string& host, const std::string& port, int size = 4) {
        for (int i = 0; i < size; i++) {
            auto client = std::make_unique<RedisClient>(host, port);
            if (!client->connect()) {
                throw std::runtime_error("RedisPool: failed to connect client " + std::to_string(i));
            }
            pool_.push(std::move(client));
        }
    }

    // RAII guard — acquires on construction, releases on destruction
    class ScopedConnection {
    public:
        ScopedConnection(RedisPool& pool) : pool_(pool), client_(pool.acquire()) {}
        ~ScopedConnection() { pool_.release(std::move(client_)); }

        RedisClient* operator->() { return client_.get(); }
        RedisClient& operator*() { return *client_; }

        ScopedConnection(const ScopedConnection&) = delete;
        ScopedConnection& operator=(const ScopedConnection&) = delete;
    private:
        RedisPool& pool_;
        std::unique_ptr<RedisClient> client_;
    };

    ScopedConnection Get() { return ScopedConnection(*this); }

private:
    std::unique_ptr<RedisClient> acquire() {
        std::unique_lock<std::mutex> lock(mu_);
        cv_.wait(lock, [this] { return !pool_.empty(); });
        auto client = std::move(pool_.front());
        pool_.pop();
        return client;
    }

    void release(std::unique_ptr<RedisClient> client) {
        std::lock_guard<std::mutex> lock(mu_);
        pool_.push(std::move(client));
        cv_.notify_one();
    }

    std::queue<std::unique_ptr<RedisClient>> pool_;
    std::mutex mu_;
    std::condition_variable cv_;
};
