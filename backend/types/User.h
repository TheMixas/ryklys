//
// Created by themi on 3/8/26.
//

#ifndef RYKLYS_BACKEND_USER_H
#define RYKLYS_BACKEND_USER_H
struct User {
    int id;
    std::string username;
    std::string email;
    std::string avatar_url;
    std::string created_at;
    std::string password_hash;

};
#endif //RYKLYS_BACKEND_USER_H
