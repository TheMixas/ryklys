#pragma once
#include "Repository.h"
#include <string>
#include <vector>

struct ChatMessage
{
    int64_t id;
    std::string stream_id;
    int user_id;
    std::string username;
    std::string message;
    std::string created_at;
};

class ChatRepository : public Repository
{
public:
    explicit ChatRepository(PGPool& pool) : Repository(pool)
    {
    }

    ChatMessage Insert(const std::string& streamId,
                       const std::string& userID,
                       const std::string& username,
                       const std::string& message) const
    {
        auto conn = Pool().connection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            "INSERT INTO chat_messages (stream_id, user_id, username, message) "
            "VALUES ($1, $2, $3, $4) "
            "RETURNING id, stream_id, user_id, username, message, created_at",
            pqxx::params{streamId, userID, username, message}
        );

        txn.commit();
        Pool().freeConnection(conn);

        return ChatMessage{
            .id = result[0]["id"].as<int64_t>(),
            .stream_id = result[0]["stream_id"].as<std::string>(),
            .user_id = result[0]["user_id"].as<std::int32_t>(),
            .username = result[0]["username"].as<std::string>(),
            .message = result[0]["message"].as<std::string>(),
            .created_at = result[0]["created_at"].as<std::string>(),
        };
    }

    std::vector<ChatMessage> GetRecent(const std::string& streamId, int limit = 50) const
    {
        auto conn = Pool().connection();
        pqxx::work txn(*conn);

        auto result = txn.exec(
            "SELECT id, stream_id, user_id, username, message, created_at "
            "FROM chat_messages "
            "WHERE stream_id = $1 "
            "ORDER BY created_at DESC "
            "LIMIT $2",
            pqxx::params{streamId, limit}
        );

        txn.commit();
        Pool().freeConnection(conn);

        std::vector<ChatMessage> messages;
        messages.reserve(result.size());

        for (const auto& row : result)
        {
            messages.push_back(ChatMessage{
                .id = row["id"].as<int64_t>(),
                .stream_id = row["stream_id"].as<std::string>(),
                .user_id = row ["user_id"].as<std::int32_t>(),
                .username = row["username"].as<std::string>(),
                .message = row["message"].as<std::string>(),
                .created_at = row["created_at"].as<std::string>(),
            });
        }

        // Reverse so oldest comes first — we queried DESC for LIMIT efficiency
        std::reverse(messages.begin(), messages.end());
        return messages;
    }
};
