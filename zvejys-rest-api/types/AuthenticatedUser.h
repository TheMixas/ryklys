//
// Created by themi on 3/9/26.
//

#ifndef RYKLYS_BACKEND_AUTHENTICATEDUSER_H
#define RYKLYS_BACKEND_AUTHENTICATEDUSER_H
// Used for passing authenticated user info to route handlers without exposing unrelated data
struct AuthenticatedUser {
    int id;
    std::string username;
};
#endif //RYKLYS_BACKEND_AUTHENTICATEDUSER_H
