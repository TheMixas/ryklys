#ifndef RYKLYS_BACKEND_ENVCONFIG_H
#define RYKLYS_BACKEND_ENVCONFIG_H

#include <string>
#include <unordered_map>
#include <optional>
#include <fstream>
#include <iostream>
#include <algorithm>

class EnvConfig {
public:
    static EnvConfig& Instance() {
        static EnvConfig instance;
        return instance;
    }

    void Load(const std::string& path ) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "[EnvConfig] Warning: could not open '" << path << "'" << std::endl;
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            // Trim leading whitespace
            line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char c) {
                return !std::isspace(c);
            }));

            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') continue;

            // Find the first '='
            auto eqPos = line.find('=');
            if (eqPos == std::string::npos) continue;

            std::string key = line.substr(0, eqPos);
            std::string value = line.substr(eqPos + 1);

            // Trim whitespace from key
            key.erase(key.find_last_not_of(" \t\r") + 1);
            key.erase(0, key.find_first_not_of(" \t\r"));

            // Trim whitespace from value
            value.erase(value.find_last_not_of(" \t\r") + 1);
            value.erase(0, value.find_first_not_of(" \t\r"));

            // Strip matching quotes
            if (value.size() >= 2) {
                char front = value.front();
                char back = value.back();
                if ((front == '"' && back == '"') || (front == '\'' && back == '\'')) {
                    value = value.substr(1, value.size() - 2);
                }
            }

            vars_[key] = value;
        }
    }

    std::optional<std::string> Get(const std::string& key) const {
        auto it = vars_.find(key);
        if (it != vars_.end()) return it->second;
        return std::nullopt;
    }

    std::string Get(const std::string& key, const std::string& defaultValue) const {
        auto it = vars_.find(key);
        if (it != vars_.end()) return it->second;
        return defaultValue;
    }

private:
    EnvConfig() = default;
    std::unordered_map<std::string, std::string> vars_;
};

#endif // RYKLYS_BACKEND_ENVCONFIG_H