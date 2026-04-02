#pragma once
#include <string>
#include <filesystem>

struct StorageConfig {
    static std::string& BaseDir() {
        static std::string dir = std::string(PROJECT_ROOT) + "/storage";
        return dir;
    }

    static void Init() {
        std::filesystem::create_directories(BaseDir());
    }
};