#include "pgPool.h"

#include <iostream>
#include "../config/EnvConfig.h"


static std::string buildConnectionString() {
    auto& env = EnvConfig::Instance();
    return "host=" + env.Get("DB_HOST", "localhost") + " "
           "port=" + env.Get("DB_PORT", "5433") + " "
           "dbname=" + env.Get("DB_NAME", "postgres") + " "
           "user=" + env.Get("DB_USER", "postgres") + " "
           "password=" + env.Get("DB_PASSWORD", "");
}
PGPool::PGPool() {
    std::cout << "Pg Poll Creation started" << std::endl;
    createPool();
    std::cout << "Pg Poll Creation finished" << std::endl;
}

void PGPool::createPool() {
    std::lock_guard<std::mutex> locker_(m_mutex);

    std::string connStr = buildConnectionString();
    for (auto i = 0; i < POOL_SIZE; i++) {
        m_pool.emplace(std::make_shared<pqxx::connection>(connStr));
    }
}

std::shared_ptr<pqxx::connection> PGPool::connection() {
    std::unique_lock<std::mutex> lock_(m_mutex);

    // if pool is empty, then wait until it notifies back
    while (m_pool.empty()) {
        m_condition.wait(lock_);
    }

    // get new connection in queue
    auto conn_ = m_pool.front();
    // immediately pop as we will use it now
    m_pool.pop();

    return conn_;
}

void PGPool::freeConnection(std::shared_ptr<pqxx::connection> conn_) {
    std::unique_lock<std::mutex> lock_(m_mutex);

    // push a new connection into a pool
    m_pool.push(conn_);

    // unlock mutex
    lock_.unlock();

    // notify one of thread that is waiting
    m_condition.notify_one();
}
