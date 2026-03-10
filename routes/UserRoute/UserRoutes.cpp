//
// Created by themi on 3/8/26.
//

#include "../UserRoute/UserRoutes.h"
#include "./../../ServiceLocator.h"
#include <iostream>

#include "../../ServiceLocator.h"
#include "./../../config/EnvConfig.h"
#include "../../database/database_repositories/UserRepository.h"
#include "../../pg-connection-pool/pgPool.h"
#include "./../../zvejys-rest-api/utils/Validation.h"
#include "./../../zvejys-rest-api/utils/PasswordHash.h"
#include "./../../zvejys-rest-api/utils/JsonWebToken.h"


void RegisterUserRoutes(ZvejysServer &server) {
    server.RegisterRoute(HttpMethod::POST, "/api/users/register", [](const HttpRequest &request) -> HttpResponse {
        // Here you would handle user registration logic, e.g., validate input, save to database, etc.
        std::cout << "Handling user registration" << std::endl;

        std::optional<std::string> username = request.BodyParam("username");
        std::optional<std::string> email = request.BodyParam("email");
        std::optional<std::string> password = request.BodyParam("password");

        if (!username.has_value()) { return HttpResponse::UnprocessableEntity("Missing username"); }
        if (!email.has_value()) { return HttpResponse::UnprocessableEntity("Missing email"); }
        if (!password.has_value()) { return HttpResponse::UnprocessableEntity("Missing password"); }

        if (std::optional<std::string> err = validation::ValidateUsername(*username))
            return HttpResponse::UnprocessableEntity(*err);
        if (std::optional<std::string> err = validation::ValidateEmail(*email))
            // Some invalid emails still go thru, fix
            return HttpResponse::UnprocessableEntity(*err);
        if (std::optional<std::string> err = validation::ValidatePassword(*password))
            return HttpResponse::UnprocessableEntity(*err);

        auto userRepo = ServiceLocator::GetUserRepository();
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

    server.RegisterRoute(HttpMethod::POST, "/api/users/login", [](const HttpRequest &request) -> HttpResponse {
        // Here you would handle user login logic, e.g., validate credentials, generate tokens, etc.
        std::cout << "Handling user login" << std::endl;

        std::optional<std::string> username = request.BodyParam("username");
        std::optional<std::string> password = request.BodyParam("password");

        if (!username.has_value()) { return HttpResponse::UnprocessableEntity("Missing username"); }
        if (!password.has_value()) { return HttpResponse::UnprocessableEntity("Missing password"); }

        auto userRepo = ServiceLocator::GetUserRepository();
        std::optional<User> loginUser = userRepo.FindByUsername(username.value());
        if (!loginUser.has_value()) {
            return HttpResponse::Unauthorized("Invalid username or password");
        }

        bool passwordMatch = password::Verify(password.value(), loginUser->password_hash);
        if (!passwordMatch) {
            return HttpResponse::Unauthorized("Invalid username or password");
        }

        std::optional<std::string> jwtSecret = EnvConfig::Instance().Get("JWT_SECRET");
        if (!jwtSecret.has_value()) {
            return HttpResponse::InternalServerError("JWT secret not configured");
        }

        std::string token = jwt::Create(loginUser->id, loginUser->username, jwtSecret.value());

        return HttpResponse::Json("{\"token\":\"" + token + "\"}");
    });

    server.RegisterAuthenticatedRoute(HttpMethod::GET, "/api/users/me", [](const HttpRequest &request, const AuthenticatedUser &authUser) -> HttpResponse {
        // This route requires authentication. The authUser parameter contains the authenticated user's info.
        std::cout << "Handling authenticated request for user: " << authUser.username << std::endl;

        // For demonstration, we just return the authenticated user's info
        HttpResponse response = HttpResponse::Json("{\"id\":" + std::to_string(authUser.id) + ", \"username\":\"" + authUser.username + "\"}");
        return response;
    });
}
