//
// Created by themi on 3/8/26.
//

#ifndef RYKLYS_BACKEND_USERREPOSITORY_H
#define RYKLYS_BACKEND_USERREPOSITORY_H
#include "Repository.h"
#include "./../../types/User.h"


class UserRepository : public Repository {
public:
    explicit UserRepository(PGPool &db_pool) : Repository(db_pool) {
    }

    // Here you would add methods for user-related database operations, e.g.:
    // - createUser
    // - getUserById
    // - updateUser
    // - deleteUser

    User Create(const std::string &username, const std::string &email,
            const std::string &password_hash) const;

    bool IsUsernameTaken(const std::string &username) const;

    bool IsEmailTaken(const std::string &email) const;

    std::optional<User> FindByUsername(const std::string &username) const;
};



#endif //RYKLYS_BACKEND_USERREPOSITORY_H
