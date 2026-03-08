//
// Created by themi on 3/8/26.
//

#include "./UserRepository.h"

/// Make sure that appropriate values are unique (e.g., username, email) to avoid conflicts with existing records in the database,
/// this will hang .
User UserRepository::Create(const std::string &username, const std::string &email,
                            const std::string &password_hash) const {
    auto conn = Pool().connection();
    pqxx::work txn(*conn);

    auto result = txn.exec(
        "INSERT INTO users (username, email, password_hash) VALUES ($1, $2, $3) RETURNING id, username, email"
        , {username, email, password_hash}
    );
    //print result

    int id = result[0]["id"].as<int>();
    std::string username_ = result[0]["username"].as<std::string>();
    std::string email_ = result[0]["email"].as<std::string>();
    // Print column headers
    txn.commit();


    Pool().freeConnection(conn);
    return User{
        .id = id,
        .username = username_,
        .email = email_,
    };
}

bool UserRepository::IsUsernameTaken(const std::string &username) const {
    auto conn = Pool().connection();
    pqxx::work txn(*conn);
    auto result = txn.exec(
        "SELECT COUNT(*) FROM users WHERE username = $1"
        , pqxx::params{username}
    );

    int count = result[0][0].as<int>();

    txn.commit();
    Pool().freeConnection(conn);
    return count > 0;
}

bool UserRepository::IsEmailTaken(const std::string &email) const {
    auto conn = Pool().connection();
    pqxx::work txn(*conn);
    auto result = txn.exec(
        "SELECT COUNT(*) FROM users WHERE email = $1"
        , pqxx::params{email}
    );

    int count = result[0][0].as<int>();

    txn.commit();
    Pool().freeConnection(conn);
    return count > 0;
}
