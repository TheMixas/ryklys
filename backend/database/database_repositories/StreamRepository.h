#pragma once
#include "Repository.h"
#include "../../types/Stream.h"
#include <optional>
#include <vector>

class StreamRepository : public Repository {
public:
    explicit StreamRepository(PGPool& pool) : Repository(pool) {}
    void EndAll() const;
    Stream Create(int userId, const std::string& streamId, const std::string& title) const;
    void SetEnded(const std::string& streamId) const;
    std::optional<Stream> FindById(const std::string& streamId) const;
    std::vector<Stream> GetActive() const;
    std::vector<Stream> GetHistoryByUserId(int userId);
};