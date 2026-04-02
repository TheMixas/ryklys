#pragma once
#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include "../zvejys-rest-api/WebSocketConnection.h"

class ViewerRegistry
{
public:
    void Add(const std::string& streamId, std::shared_ptr<WebSocketConnection> ws)
    {
        std::lock_guard<std::mutex> lock(mu_);
        viewers_[streamId].insert(std::move(ws));
    }

    void Remove(const std::string& streamId, const std::shared_ptr<WebSocketConnection>& ws)
    {
        std::lock_guard<std::mutex> lock(mu_);
        auto it = viewers_.find(streamId);
        if (it == viewers_.end()) return;
        it->second.erase(ws);
        if (it->second.empty()) viewers_.erase(it);
    }

    void Broadcast(const std::string& streamId, const std::string& message)
    {
        std::vector<std::shared_ptr<WebSocketConnection>> snapshot;
        {
            std::lock_guard<std::mutex> lock(mu_);
            auto it = viewers_.find(streamId);
            if (it == viewers_.end()) return;
            snapshot.assign(it->second.begin(), it->second.end());
        }
        // Send outside the lock
        for (auto& ws : snapshot)
        {
            try
            {
                if (ws->GetState() == WsState::OPEN)
                {
                    ws->SendText(message);
                }
            }
            catch (...)
            {
            }
        }
    }

    void CloseAll(const std::string& streamId, uint16_t code, const std::string& reason)
    {
        std::vector<std::shared_ptr<WebSocketConnection>> snapshot;
        {
            std::lock_guard<std::mutex> lock(mu_);
            auto it = viewers_.find(streamId);
            if (it == viewers_.end()) return;
            snapshot.assign(it->second.begin(), it->second.end());
            viewers_.erase(it);
        }
        for (auto& ws : snapshot)
        {
            try { ws->Close(code, reason); }
            catch (...)
            {
            }
        }
    }

    size_t Count(const std::string& streamId)
    {
        std::lock_guard<std::mutex> lock(mu_);
        auto it = viewers_.find(streamId);
        return it != viewers_.end() ? it->second.size() : 0;
    }

private:
    std::mutex mu_;
    std::unordered_map<std::string,
                       std::unordered_set<std::shared_ptr<WebSocketConnection>>> viewers_;
};
