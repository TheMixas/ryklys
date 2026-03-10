
#pragma once
#include "./database/database_repositories/UserRepository.h"
#include <atomic>
#include <stdexcept>

class ServiceLocator {
public:

    ServiceLocator() = delete;

    static void SetUserRepository(UserRepository* repo) {
        userRepo_.store(repo, std::memory_order_release);
    }
    static UserRepository& GetUserRepository() {
        UserRepository* repo = userRepo_.load(std::memory_order_acquire);
        if (!repo) throw std::runtime_error("UserRepository not registered");
        return *repo;
    }
private:
    static inline std::atomic<UserRepository*> userRepo_{nullptr};
};
