//
// Created by themi on 3/8/26.
//

#include "../UserRoute/UserRoutes.h"

#include <iostream>

#include "../../database/database_repositories/UserRepository.h"
#include "../../pg-connection-pool/pgPool.h"
#include "./../../zvejys-rest-api/utils/Validation.h"
#include "./../../zvejys-rest-api/utils/PasswordHash.h"

static const std::string JWT_SECRET = "your-secret-key-change-this-in-production";
void RegisterUserRoutes(ZvejysServer &server, UserRepository &userRepo) {
    server.RegisterRoute(HttpMethod::POST, "/user/register", [&userRepo](const HttpRequest &request) -> HttpResponse {
        // Here you would handle user registration logic, e.g., validate input, save to database, etc.
        std::cout << "Handling user registration" << std::endl;

        std::optional<std::string> username = request.BodyParam("username");
        std::optional<std::string> email = request.BodyParam("email");
        std::optional<std::string> password = request.BodyParam("password");

        if (!username.has_value()) { return HttpResponse::UnprocessableEntity("Missing username");}
        if (!email.has_value()) { return HttpResponse::UnprocessableEntity("Missing email"); }
        if (!password.has_value()) { return HttpResponse::UnprocessableEntity("Missing password"); }

        if (std::optional<std::string> err = validation::ValidateUsername(*username))
            return HttpResponse::UnprocessableEntity(*err);
        if (std::optional<std::string> err = validation::ValidateEmail(*email)) // Some invalid emails still go thru, fix
            return HttpResponse::UnprocessableEntity(*err);
        if (std::optional<std::string> err = validation::ValidatePassword(*password))
            return HttpResponse::UnprocessableEntity(*err);

        // Check if username is already exists or email is already exists
        if (userRepo.IsUsernameTaken(username.value())) {
            return HttpResponse::Conflict("Username is already taken");
        }
        if (userRepo.IsEmailTaken(email.value())) {
            return HttpResponse::Conflict("Email is already taken");
        }

        std::string hashed = password::Hash(*password);
        User newUser = userRepo.Create(username.value(), email.value(), hashed);
        std::cout << "Created user with ID: " << newUser.id << std::endl;



        // For demonstration, we just return a success response
        HttpResponse response = HttpResponse::Created("User registered successfully");
        return response;
    });

    server.RegisterRoute(HttpMethod::POST, "/user/login", [&userRepo](const HttpRequest &request) -> HttpResponse {
        // Here you would handle user login logic, e.g., validate credentials, generate tokens, etc.
        std::cout << "Handling user login" << std::endl;

        std::optional<std::string> username = request.BodyParam("username");
        std::optional<std::string> password = request.BodyParam("password");

        if (!username.has_value()) { return HttpResponse::UnprocessableEntity("Missing username");}
        if (!password.has_value()) { return HttpResponse::UnprocessableEntity("Missing password"); }

        std::optional<User> loginUser = userRepo.FindByUsername(username.value());
        if (!loginUser.has_value()) {
            return HttpResponse::Unauthorized("Invalid username or password");
        }

        bool passwordMatch = password::Verify(password.value(), loginUser->password_hash);
        if (!passwordMatch) {
            return HttpResponse::Unauthorized("Invalid username or password");
        }

        // For demonstration, we just return a success response
        HttpResponse response = HttpResponse::Ok("User logged in successfully");
        return response;
    });
}
