#ifndef PGPOOL_H
#define PGPOOL_H

#include <mutex>
#include <pqxx/pqxx>
#include <memory>
#include <condition_variable>
#include <iostream>
#include <queue>

class PGPool {
public:
    PGPool();

    std::shared_ptr<pqxx::connection> connection();

    void freeConnection(std::shared_ptr<pqxx::connection>);

    void testPrint() {
        std::cout << "I am a pg pool" << std::endl;
    }

private:
    void createPool();

    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::queue<std::shared_ptr<pqxx::connection> > m_pool;

    const int POOL_SIZE = 20;
};

#endif
