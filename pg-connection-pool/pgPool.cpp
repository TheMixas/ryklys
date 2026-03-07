#include "pgPool.h"

#include <iostream>

PGPool::PGPool() {
    std::cout << "Pg Poll Creation started" << std::endl;
    createPool();
    std::cout << "Pg Poll Creation finished" << std::endl;
}

void PGPool::createPool() {
    std::lock_guard<std::mutex> locker_(m_mutex);

    for (auto i = 0; i < POOL_SIZE; i++) {
        m_pool.emplace(std::make_shared<pqxx::connection>(
            "host=172.24.240.1 "
            "port=5432 "
            "dbname=postgres "
            "user=postgres "
            "password=1"
        ));
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
