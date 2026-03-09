#pragma once
#include <string>
#include <optional>
#include "Crypto.h"

namespace jwt {

    inline std::string Base64URLEncode(const uint8_t *data, size_t len) {
        static const char table[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string result;
        result.reserve(4 * ((len + 2) / 3));

        for (size_t i = 0; i < len; i += 3) {
            uint32_t n = (static_cast<uint32_t>(data[i]) << 16);
            if (i + 1 < len) n |= (static_cast<uint32_t>(data[i + 1]) << 8);
            if (i + 2 < len) n |= static_cast<uint32_t>(data[i + 2]);

            result.push_back(table[(n >> 18) & 0x3F]);
            result.push_back(table[(n >> 12) & 0x3F]);
            if (i + 1 < len) result.push_back(table[(n >> 6) & 0x3F]);
            if (i + 2 < len) result.push_back(table[n & 0x3F]);
        }

        // Replace + with -, / with _ (Base64URL)
        for (char &c : result) {
            if (c == '+') c = '-';
            else if (c == '/') c = '_';
        }
        // No padding
        return result;
    }

    inline std::string Base64URLEncode(const std::string &input) {
        return Base64URLEncode(reinterpret_cast<const uint8_t*>(input.data()), input.size());
    }

    // ─── CREATE ───

    inline std::string Create(int user_id, const std::string &username,
                              const std::string &secret, int expires_in_seconds = 86400) {
        // Header (always the same for HS256)
        std::string header = Base64URLEncode("{\"alg\":\"HS256\",\"typ\":\"JWT\"}");

        // Payload
        long exp = std::time(nullptr) + expires_in_seconds;
        std::string payload_json =
            "{\"id\":" + std::to_string(user_id) +
            ",\"username\":\"" + username + "\"" +
            ",\"exp\":" + std::to_string(exp) + "}";
        std::string payload = Base64URLEncode(payload_json);

        // Signature
        std::string signing_input = header + "." + payload;
        auto sig = crypto::HMAC_SHA256(secret, signing_input);
        std::string signature = Base64URLEncode(sig.data(), sig.size());

        return signing_input + "." + signature;
    }

    // ─── VERIFY & DECODE ───

    struct Claims {
        int id;
        std::string username;
        long exp;
    };

    // Minimal Base64URL decode
    inline std::string Base64URLDecode(const std::string &input) {
        // Restore standard Base64
        std::string b64 = input;
        for (char &c : b64) {
            if (c == '-') c = '+';
            else if (c == '_') c = '/';
        }
        // Add padding
        while (b64.size() % 4 != 0) b64.push_back('=');

        static const int lookup[128] = {
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
            52,53,54,55,56,57,58,59,60,61,-1,-1,-1, 0,-1,-1,
            -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
            15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
            -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
            41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1
        };

        std::string out;
        for (size_t i = 0; i < b64.size(); i += 4) {
            uint32_t n = (lookup[(int)b64[i]] << 18) | (lookup[(int)b64[i+1]] << 12) |
                         (lookup[(int)b64[i+2]] << 6)  |  lookup[(int)b64[i+3]];
            out.push_back((n >> 16) & 0xFF);
            if (b64[i+2] != '=') out.push_back((n >> 8) & 0xFF);
            if (b64[i+3] != '=') out.push_back(n & 0xFF);
        }
        return out;
    }

    inline std::optional<Claims> Verify(const std::string &token, const std::string &secret) {
        // Split into 3 parts
        size_t dot1 = token.find('.');
        if (dot1 == std::string::npos) return std::nullopt;
        size_t dot2 = token.find('.', dot1 + 1);
        if (dot2 == std::string::npos) return std::nullopt;
        if (token.find('.', dot2 + 1) != std::string::npos) return std::nullopt;

        std::string signing_input = token.substr(0, dot2);
        std::string signature = token.substr(dot2 + 1);

        // Verify signature
        auto expected_sig = crypto::HMAC_SHA256(secret, signing_input);
        std::string expected = Base64URLEncode(expected_sig.data(), expected_sig.size());
        if (signature != expected) return std::nullopt;

        // Decode payload
        std::string payload_b64 = token.substr(dot1 + 1, dot2 - dot1 - 1);
        std::string payload_json = Base64URLDecode(payload_b64);

        // Minimal JSON extraction (your fields are always the same shape)
        Claims claims{};

        // Extract "id":<number>
        size_t id_pos = payload_json.find("\"id\":");
        if (id_pos == std::string::npos) return std::nullopt;
        claims.id = std::stoi(payload_json.substr(id_pos + 5));

        // Extract "username":"<string>"
        size_t user_pos = payload_json.find("\"username\":\"");
        if (user_pos == std::string::npos) return std::nullopt;
        size_t user_start = user_pos + 12;
        size_t user_end = payload_json.find('"', user_start);
        claims.username = payload_json.substr(user_start, user_end - user_start);

        // Extract "exp":<number>
        size_t exp_pos = payload_json.find("\"exp\":");
        if (exp_pos == std::string::npos) return std::nullopt;
        claims.exp = std::stol(payload_json.substr(exp_pos + 6));

        // Check expiry
        if (claims.exp < std::time(nullptr)) return std::nullopt;

        return claims;
    }

} // namespace jwt