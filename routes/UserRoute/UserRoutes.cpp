//
// Created by themi on 3/8/26.
//

#include "../UserRoute/UserRoutes.h"

#include <iostream>

#include "../../database/database_repositories/UserRepository.h"
#include "../../pg-connection-pool/pgPool.h"

void RegisterUserRoutes(ZvejysServer &server, UserRepository &userRepo) {
    server.RegisterRoute(HttpMethod::POST, "/user/register", [&userRepo](const HttpRequest &request) -> HttpResponse {
        // Here you would handle user registration logic, e.g., validate input, save to database, etc.
        std::cout << "Handling user registration" << std::endl;

        User newUser = userRepo.Create("testuse123123r", "22@gmaill.com@123123123123", "hashed_password123123");
        std::cout << "Created user with ID: " << newUser.id << std::endl;



        // For demonstration, we just return a success response
        HttpResponse response = HttpResponse::Created("User registered successfully");
        return response;
    });
}
