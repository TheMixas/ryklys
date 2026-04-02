#include "../user-route/UserRoutes.h"
#include "./../../ServiceLocator.h"
#include <iostream>

#include "../../ServiceLocator.h"
#include "../../zvejys-rest-api/env/EnvConfig.h"
#include "../../database/database_repositories/UserRepository.h"
#include "../../pg-connection-pool/pgPool.h"
#include "./../../zvejys-rest-api/utils/Validation.h"
#include "./../../zvejys-rest-api/utils/PasswordHash.h"
#include "./../../zvejys-rest-api/utils/JsonWebToken.h"


void RegisterUserRoutes(ZvejysServer& server)
{
    server.RegisterRoute(HttpMethod::POST, "/api/users/register", [](const HttpRequest& request) -> HttpResponse
    {
        std::cout << "[REGISTER] === Registration request received ===" << std::endl;
        std::cout << "[REGISTER] Headers:" << std::endl;
        for (const auto& [key, value] : request.headers)
        {
            std::cout << "[REGISTER]   " << key << ": " << value << std::endl;
        }
        std::cout << "[REGISTER] Body: " << std::string(request.body.begin(), request.body.end()) << std::endl;
        std::optional<std::string> username = request.BodyParam("username");
        std::optional<std::string> email = request.BodyParam("email");
        std::optional<std::string> password = request.BodyParam("password");

        std::cout << "[REGISTER] Parsed fields — username: " << (username.has_value() ? *username : "MISSING")
            << ", email: " << (email.has_value() ? *email : "MISSING")
            << ", password: " << (password.has_value() ? "present" : "MISSING") << std::endl;

        if (!username.has_value())
        {
            std::cout << "[REGISTER] FAIL: Missing username" << std::endl;
            return HttpResponse::UnprocessableEntity("Missing username");
        }
        if (!email.has_value())
        {
            std::cout << "[REGISTER] FAIL: Missing email" << std::endl;
            return HttpResponse::UnprocessableEntity("Missing email");
        }
        if (!password.has_value())
        {
            std::cout << "[REGISTER] FAIL: Missing password" << std::endl;
            return HttpResponse::UnprocessableEntity("Missing password");
        }

        if (std::optional<std::string> err = validation::ValidateUsername(*username))
        {
            std::cout << "[REGISTER] FAIL: Username validation — " << *err << std::endl;
            return HttpResponse::UnprocessableEntity(*err);
        }
        if (std::optional<std::string> err = validation::ValidateEmail(*email))
        {
            std::cout << "[REGISTER] FAIL: Email validation — " << *err << std::endl;
            return HttpResponse::UnprocessableEntity(*err);
        }
        if (std::optional<std::string> err = validation::ValidatePassword(*password))
        {
            std::cout << "[REGISTER] FAIL: Password validation — " << *err << std::endl;
            return HttpResponse::UnprocessableEntity(*err);
        }

        std::cout << "[REGISTER] Validation passed, checking uniqueness..." << std::endl;
        auto userRepo = ServiceLocator::GetUserRepository();

        if (userRepo.IsUsernameTaken(username.value()))
        {
            std::cout << "[REGISTER] FAIL: Username '" << *username << "' already taken" << std::endl;
            return HttpResponse::Conflict("Username is already taken");
        }
        if (userRepo.IsEmailTaken(email.value()))
        {
            std::cout << "[REGISTER] FAIL: Email '" << *email << "' already taken" << std::endl;
            return HttpResponse::Conflict("Email is already taken");
        }

        std::cout << "[REGISTER] Hashing password..." << std::endl;
        std::string hashed = password::Hash(*password);
        std::cout << "[REGISTER] Creating user in DB..." << std::endl;
        User newUser = userRepo.Create(username.value(), email.value(), hashed);
        std::cout << "[REGISTER] SUCCESS: Created user ID=" << newUser.id << " username=" << *username << std::endl;

        HttpResponse response = HttpResponse::Created("User registered successfully");
        std::cout << "[REGISTER]Fetching JWT_SECRET..." << std::endl;
        std::optional<std::string> jwtSecret = EnvConfig::Instance().Get("JWT_SECRET");
        if (!jwtSecret.has_value())
        {
            std::cout << "[REGISTER] FAIL: JWT_SECRET not found in config!" << std::endl;
            return HttpResponse::InternalServerError("JWT secret not configured");
        }
        std::cout << "[REGISTER]JWT_SECRET found (length=" << jwtSecret->length() << ")" << std::endl;
        std::string token = jwt::Create(newUser.id, newUser.username, jwtSecret.value());
        std::string cookie = std::string("token=") + token +
            "; HttpOnly; Secure; SameSite=None; Path=/; Max-Age=360000";
        response.headers["Set-Cookie"] = cookie;
        return response;
    });

    server.RegisterRoute(HttpMethod::POST, "/api/users/login", [](const HttpRequest& request) -> HttpResponse
    {
        std::cout << "[LOGIN] === Login request received ===" << std::endl;
        std::cout << "[LOGIN] Headers:" << std::endl;
        for (const auto& [key, value] : request.headers)
        {
            std::cout << "[LOGIN]   " << key << ": " << value << std::endl;
        }
        std::cout << "[REGISTER] Body: " << std::string(request.body.begin(), request.body.end()) << std::endl;
        std::optional<std::string> username = request.BodyParam("username");
        std::optional<std::string> password = request.BodyParam("password");

        std::cout << "[LOGIN] Parsed — username: " << (username.has_value() ? *username : "MISSING")
            << ", password: " << (password.has_value() ? "present" : "MISSING") << std::endl;

        if (!username.has_value())
        {
            std::cout << "[LOGIN] FAIL: Missing username" << std::endl;
            return HttpResponse::UnprocessableEntity("Missing username");
        }
        if (!password.has_value())
        {
            std::cout << "[LOGIN] FAIL: Missing password" << std::endl;
            return HttpResponse::UnprocessableEntity("Missing password");
        }

        std::cout << "[LOGIN] Looking up user '" << *username << "' in DB..." << std::endl;
        auto userRepo = ServiceLocator::GetUserRepository();
        std::optional<User> loginUser = userRepo.FindByUsername(username.value());

        if (!loginUser.has_value())
        {
            std::cout << "[LOGIN] FAIL: User '" << *username << "' not found in DB" << std::endl;
            return HttpResponse::Unauthorized("Invalid username or password");
        }
        std::cout << "[LOGIN] Found user ID=" << loginUser->id << " username=" << loginUser->username << std::endl;

        std::cout << "[LOGIN] Verifying password..." << std::endl;
        bool passwordMatch = password::Verify(password.value(), loginUser->password_hash);
        std::cout << "[LOGIN] Password match: " << (passwordMatch ? "YES" : "NO") << std::endl;

        if (!passwordMatch)
        {
            std::cout << "[LOGIN] FAIL: Password mismatch for user '" << *username << "'" << std::endl;
            return HttpResponse::Unauthorized("Invalid username or password");
        }

        std::cout << "[LOGIN] Fetching JWT_SECRET..." << std::endl;
        std::optional<std::string> jwtSecret = EnvConfig::Instance().Get("JWT_SECRET");
        if (!jwtSecret.has_value())
        {
            std::cout << "[LOGIN] FAIL: JWT_SECRET not found in config!" << std::endl;
            return HttpResponse::InternalServerError("JWT secret not configured");
        }
        std::cout << "[LOGIN] JWT_SECRET found (length=" << jwtSecret->length() << ")" << std::endl;

        std::cout << "[LOGIN] Creating JWT token..." << std::endl;
        std::string token = jwt::Create(loginUser->id, loginUser->username, jwtSecret.value());
        std::cout << "[LOGIN] Token created (length=" << token.length() << ")" << std::endl;

        HttpResponse response = HttpResponse::Ok("Login successful");
        std::string cookie = std::string("token=") + token +
            "; HttpOnly; Secure; SameSite=None; Path=/; Max-Age=360000";
        response.headers["Set-Cookie"] = cookie;
        std::cout << "[LOGIN] SUCCESS: Set-Cookie header set for user '" << *username << "'" << std::endl;
        std::cout << "[LOGIN] Cookie value: " << cookie << std::endl;

        return response;
    });
    server.RegisterRoute(HttpMethod::POST, "/api/users/logout", [](const HttpRequest &request) -> HttpResponse {
        HttpResponse response = HttpResponse::Ok("Logged out");
        response.headers["Set-Cookie"] = "token=; HttpOnly; SameSite=Lax; Path=/; Max-Age=0";
        return response;
    });
    server.RegisterAuthenticatedRoute(HttpMethod::GET, "/api/users/me",
                                      [](const HttpRequest& request,
                                         const AuthenticatedUser& authUser) -> HttpResponse
                                      {
                                          std::cout << "[ME] === /api/users/me request received ===" << std::endl;
                                          std::cout << "[ME] Headers:" << std::endl;
                                          for (const auto& [key, value] : request.headers)
                                          {
                                              std::cout << "[ME]   " << key << ": " << value << std::endl;
                                          }
                                          std::cout << "[ME] Authenticated user: ID=" << authUser.id
                                              << " username=" << authUser.username << std::endl;

                                          HttpResponse response = HttpResponse::Json(
                                              "{\"id\":" + std::to_string(authUser.id) + ", \"username\":\"" + authUser.
                                              username + "\"}");

                                          std::cout << "[ME] SUCCESS: Returning user info" << std::endl;
                                          return response;
                                      });
}
