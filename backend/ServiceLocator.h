#pragma once
#include "./database/database_repositories/UserRepository.h"
#include "./database/database_repositories/StreamRepository.h"
#include <atomic>
#include <stdexcept>

#include "RedisPool.h"
#include "database/database_repositories/ChatRepository.h"
#include "services/ChatRelay.h"
#include "services/RedisClient.h"
#include "services/ViewerRegistry.h"

class ServiceLocator
{
public:
    ServiceLocator() = delete;

    static void SetUserRepository(UserRepository* repo)
    {
        userRepo_.store(repo, std::memory_order_release);
    }

    static UserRepository& GetUserRepository()
    {
        UserRepository* repo = userRepo_.load(std::memory_order_acquire);
        if (!repo) throw std::runtime_error("UserRepository not registered");
        return *repo;
    }

    static void SetStreamRepository(StreamRepository* repo)
    {
        streamRepo_.store(repo, std::memory_order_release);
    }

    static StreamRepository& GetStreamRepository()
    {
        StreamRepository* repo = streamRepo_.load(std::memory_order_acquire);
        if (!repo) throw std::runtime_error("StreamRepository not registered");
        return *repo;
    }

    static void SetRedisClient(RedisClient* client)
    {
        redis_.store(client, std::memory_order_release);
    }

    static RedisClient& GetRedisClient()
    {
        RedisClient* client = redis_.load(std::memory_order_acquire);
        if (!client) throw std::runtime_error("RedisClient not registered");
        return *client;
    }

    static void SetRedisPool(RedisPool* pool)
    {
        redisPool_.store(pool, std::memory_order_release);
    }

    static RedisPool& GetRedisPool()
    {
        RedisPool* pool = redisPool_.load(std::memory_order_acquire);
        if (!pool) throw std::runtime_error("RedisPool not registered");
        return *pool;
    }

    static void SetViewerRegistry(ViewerRegistry* reg)
    {
        viewerRegistry_.store(reg, std::memory_order_release);
    }

    static ViewerRegistry& GetViewerRegistry()
    {
        ViewerRegistry* reg = viewerRegistry_.load(std::memory_order_acquire);
        if (!reg) throw std::runtime_error("ViewerRegistry not registered");
        return *reg;
    }

    static void SetChatRepository(ChatRepository* repo)
    {
        chatRepo_.store(repo, std::memory_order_release);
    }

    static ChatRepository& GetChatRepository()
    {
        ChatRepository* repo = chatRepo_.load(std::memory_order_acquire);
        if (!repo) throw std::runtime_error("ChatRepository not registered");
        return *repo;
    }

    static void SetChatRelay(ChatRelay* relay)
    {
        chatRelay_.store(relay, std::memory_order_release);
    }

    static ChatRelay& GetChatRelay()
    {
        ChatRelay* repo = chatRelay_.load(std::memory_order_acquire);
        if (!repo) throw std::runtime_error("ChatRelay not registered");
        return *repo;
    }

private:
    static inline std::atomic<UserRepository*> userRepo_{nullptr};
    static inline std::atomic<StreamRepository*> streamRepo_{nullptr};
    static inline std::atomic<RedisClient*> redis_{nullptr};
    static inline std::atomic<RedisPool*> redisPool_{nullptr};
    static inline std::atomic<ViewerRegistry*> viewerRegistry_{nullptr};
    static inline std::atomic<ChatRepository*> chatRepo_{nullptr};
    static inline std::atomic<ChatRelay*> chatRelay_{nullptr};
};
