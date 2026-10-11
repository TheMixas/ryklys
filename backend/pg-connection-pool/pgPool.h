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

    /// RAII handle for a pooled connection. The destructor hands the connection
    /// back to the pool, so it is returned even if a query throws.
    class Lease {
    public:
        explicit Lease(PGPool &pool) : pool_(pool), conn_(pool.connection()) {}
        ~Lease() { pool_.freeConnection(std::move(conn_)); }
        Lease(const Lease &) = delete;
        Lease &operator=(const Lease &) = delete;

        pqxx::connection &operator*() const { return *conn_; }
        pqxx::connection *operator->() const { return conn_.get(); }

    private:
        PGPool &pool_;
        std::shared_ptr<pqxx::connection> conn_;
    };

    /// Preferred way to borrow a connection: `auto conn = Pool().lease();`
    Lease lease() { return Lease(*this); }

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
