#pragma once
#include <string>
#include <crypt.h>  // link with -lcrypt
#include <random>

namespace password {
    // Generate a bcrypt salt: $2b$12$<22 random base64 chars>
    inline std::string GenerateSalt(int cost = 12) {
        static const char charset[] =
                "./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);

        // std::string salt = "$2b$" + (cost < 10 ? "0" : "") + std::to_string(cost) + "$";
        std::string salt = std::string("$2b$") + (cost < 10 ? "0" : "") + std::to_string(cost) + "$";
        for (int i = 0; i < 22; i++) {
            salt += charset[dist(gen)];
        }
        return salt;
    }

    inline std::string Hash(const std::string &password) {
        std::string salt = GenerateSalt();
        // crypt_r is thread-safe
        struct crypt_data data{};
        char *result = crypt_r(password.c_str(), salt.c_str(), &data);
        return std::string(result);
    }

    inline bool Verify(const std::string &password, const std::string &hash) {
        struct crypt_data data{};
        char *result = crypt_r(password.c_str(), hash.c_str(), &data);
        return result && hash == result;
    }
} // namespace password
