#pragma once
#include "../zvejys-rest-api/env/EnvConfig.h"
#include "rtc/configuration.hpp"

struct WebRTCConfig {
    static rtc::Configuration load() {
        rtc::Configuration config;
        config.portRangeBegin = 10000; // default is 1024, but we want to avoid conflicts with common services
        config.portRangeEnd = 20000; // default is 65535, but
        config.disableAutoNegotiation = false;
        config.mtu = 1200; // typical MTU for WebRTC, can be adjusted based on network conditions
        // STUN
        config.iceServers.emplace_back(
            "stun:stun.l.google.com:19302"
        );
        config.iceServers.emplace_back(
            "stun:stun1.l.google.com:19302");

        // TODO: TURN (required for production behind NAT)
        // const std::string turnUrl  = EnvConfig::Instance().Get("TURN_URL", "");
        // const std::string turnUser = EnvConfig::Instance().Get("TURN_USER", "");
        // const std::string turnPass = EnvConfig::Instance().Get("TURN_PASS", "");
        // if (!turnUrl.empty()) {
        //     config.iceServers.emplace_back(turnUrl, turnUser, turnPass);
        // }

        return config;
    }
};
