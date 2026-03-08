//
// Created by themi on 3/8/26.
//

#ifndef RYKLYS_BACKEND_REPOSITORY_H
#define RYKLYS_BACKEND_REPOSITORY_H
#include "../../pg-connection-pool/pgPool.h"

class Repository {
public:
    explicit Repository(PGPool &db_pool) : dbPool(db_pool) {
    }
protected:
    PGPool& Pool() const {
        return dbPool;
    }
private:
    PGPool &dbPool;
};
#endif //RYKLYS_BACKEND_REPOSITORY_H
