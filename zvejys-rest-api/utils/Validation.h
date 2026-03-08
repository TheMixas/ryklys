#pragma once
#include <string>
#include <optional>

namespace validation {

    // Returns an error message if invalid, std::nullopt if valid
    inline std::optional<std::string> ValidateUsername(const std::string &username) {
        if (username.size() < 3)
            return "Username must be at least 3 characters";
        if (username.size() > 50)
            return "Username must be at most 50 characters";
        for (char c : username) {
            if (!std::isalnum(c) && c != '_' && c != '-')
                return "Username can only contain letters, numbers, _ and -";
        }
        return std::nullopt;
    }

    inline std::optional<std::string> ValidateEmail(const std::string &email) {
        if (email.empty())
            return "Email is required";
        if (email.size() > 254)
            return "Email too long";

        size_t at = email.find('@');

        // Must have exactly one @
        if (at == std::string::npos)
            return "Invalid email";
        if (email.find('@', at + 1) != std::string::npos)
            return "Invalid email";

        std::string local = email.substr(0, at);
        std::string domain = email.substr(at + 1);

        // Local part checks
        if (local.empty() || local.size() > 64)
            return "Invalid email";
        if (local.front() == '.' || local.back() == '.')
            return "Invalid email";
        if (local.find("..") != std::string::npos)
            return "Invalid email";
        for (char c : local) {
            if (!(std::isalnum(c) || c == '.' || c == '_' || c == '-' || c == '+'))
                return "Invalid email";
        }

        // Domain part checks
        if (domain.empty() || domain.size() > 253)
            return "Invalid email";
        if (domain.front() == '-' || domain.back() == '-')
            return "Invalid email";
        if (domain.front() == '.' || domain.back() == '.')
            return "Invalid email";
        if (domain.find("..") != std::string::npos)
            return "Invalid email";

        // Must have at least one dot (e.g. "gmail.com", not "localhost")
        size_t last_dot = domain.rfind('.');
        if (last_dot == std::string::npos)
            return "Invalid email";

        // TLD must be at least 2 chars
        std::string tld = domain.substr(last_dot + 1);
        if (tld.size() < 2)
            return "Invalid email";

        // Domain can only contain alphanumeric, hyphens, dots
        for (char c : domain) {
            if (!(std::isalnum(c) || c == '-' || c == '.'))
                return "Invalid email";
        }

        // No label (part between dots) can start/end with hyphen or be empty
        size_t pos = 0;
        while (pos < domain.size()) {
            size_t dot = domain.find('.', pos);
            if (dot == std::string::npos) dot = domain.size();
            std::string label = domain.substr(pos, dot - pos);
            if (label.empty() || label.size() > 63)
                return "Invalid email";
            if (label.front() == '-' || label.back() == '-')
                return "Invalid email";
            pos = dot + 1;
        }

        return std::nullopt;
    }

    inline std::optional<std::string> ValidatePassword(const std::string &password) {
        if (password.size() < 8)
            return "Password must be at least 8 characters";
        if (password.size() > 128)
            return "Password too long";
        bool has_upper = false, has_lower = false, has_digit = false;
        for (char c : password) {
            if (std::isupper(c)) has_upper = true;
            if (std::islower(c)) has_lower = true;
            if (std::isdigit(c)) has_digit = true;
        }
        if (!has_upper || !has_lower || !has_digit)
            return "Password must contain uppercase, lowercase, and a digit";
        return std::nullopt;
    }

} // namespace validation