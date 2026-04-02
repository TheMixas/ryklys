#pragma once
#include <string>
#include <optional>

struct Stream {
    std::string id;
    int user_id;
    int viewerCount;
    std::string title;
    std::string status;
    std::string started_at;
    std::optional<std::string> ended_at;
    std::string username;
};