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
        auto username = request.BodyParam("username");

        if (!username.has_value()) {
            return HttpResponse::UnprocessableEntity("Missing username");
        }

        // Check if username is already exists
        if (userRepo.IsUsernameTaken(username.value())) {
            return HttpResponse::Conflict("Username is already taken");
        }

        auto email = request.BodyParam("email");
        if (!email.has_value()) {
            return HttpResponse::UnprocessableEntity("Missing email");
        }

        // Check if email is already exists
        if (userRepo.IsEmailTaken(email.value())) {
            return HttpResponse::Conflict("Email is already taken");
        }


        User newUser = userRepo.Create(username.value(), email.value(), "hashed_password123123");
        std::cout << "Created user with ID: " << newUser.id << std::endl;



        // For demonstration, we just return a success response
        HttpResponse response = HttpResponse::Created("User registered successfully");
        return response;
    });
}
