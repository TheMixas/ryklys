
#pragma once
#include "../ServiceLocator.h"
#include <iostream>

inline void CleanupStream(const std::string& channel) {
    auto& registry = ServiceLocator::GetViewerRegistry();
    auto& pool     = ServiceLocator::GetRedisPool();

    // 1. Close all viewer WebSockets with a clean "stream ended" frame
    //    Code 1001 = "going away"
    registry.CloseAll(channel, 1001, "Stream ended");
    // 2. Remove viewer count key from Redis
    {
        auto redis = pool.Get();
        redis->set("viewers:" + channel, "0");
    }

    std::cout << "[Cleanup] Channel " << channel << " stream ended" << std::endl;}