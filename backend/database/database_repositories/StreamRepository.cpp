#include "StreamRepository.h"

void StreamRepository::EndAll() const {
    auto conn = Pool().connection();
    pqxx::work txn(*conn);

    txn.exec(
        "UPDATE streams SET status = 'ended', ended_at = CURRENT_TIMESTAMP "
        "WHERE status = 'live'"
    );

    txn.commit();
    Pool().freeConnection(conn);

    std::cout << "[Startup] Cleaned up orphaned streams" << std::endl;
}
Stream StreamRepository::Create(int userId, const std::string& streamId, const std::string& title) const {
    auto conn = Pool().connection();
    pqxx::work txn(*conn);

    auto result = txn.exec(
        "INSERT INTO streams (id, user_id, title, status) "
        "VALUES ($1, $2, $3, 'live') "
        "RETURNING id, user_id, title, status, started_at",
        pqxx::params{streamId, userId, title}
    );

    txn.commit();
    Pool().freeConnection(conn);

    return Stream{
        .id = result[0]["id"].as<std::string>(),
        .user_id = result[0]["user_id"].as<int>(),
        .title = result[0]["title"].as<std::string>(),
        .status = result[0]["status"].as<std::string>(),
        .started_at = result[0]["started_at"].as<std::string>(),
    };
}

void StreamRepository::SetEnded(const std::string& streamId) const {
    auto conn = Pool().connection();
    pqxx::work txn(*conn);

    txn.exec(
        "UPDATE streams SET status = 'ended', ended_at = CURRENT_TIMESTAMP "
        "WHERE id = $1",
        pqxx::params{streamId}
    );

    txn.commit();
    Pool().freeConnection(conn);
}

std::optional<Stream> StreamRepository::FindById(const std::string& streamId) const {
    auto conn = Pool().connection();
    pqxx::work txn(*conn);

    auto result = txn.exec(
        "SELECT id, user_id, title, status, started_at, ended_at "
        "FROM streams WHERE id = $1",
        pqxx::params{streamId}
    );

    txn.commit();
    Pool().freeConnection(conn);

    if (result.empty()) return std::nullopt;

    return Stream{
        .id = result[0]["id"].as<std::string>(),
        .user_id = result[0]["user_id"].as<int>(),
        .title = result[0]["title"].as<std::string>(),
        .status = result[0]["status"].as<std::string>(),
        .started_at = result[0]["started_at"].as<std::string>(),
        .ended_at = result[0]["ended_at"].is_null()
            ? std::nullopt
            : std::optional<std::string>(result[0]["ended_at"].as<std::string>()),
    };
}

std::vector<Stream> StreamRepository::GetActive() const {
    auto conn = Pool().connection();
    pqxx::work txn(*conn);

    auto result = txn.exec(
        "SELECT s.id, s.user_id, s.title, s.status, s.started_at, u.username "
        "FROM streams s JOIN users u ON s.user_id = u.id "
        "WHERE s.status = 'live' "
        "ORDER BY s.started_at DESC"
    );

    txn.commit();
    Pool().freeConnection(conn);

    std::vector<Stream> streams;
    for (const auto& row : result) {
        streams.push_back(Stream{
            .id = row["id"].as<std::string>(),
            .user_id = row["user_id"].as<int>(),
            .title = row["title"].as<std::string>(),
            .status = row["status"].as<std::string>(),
            .started_at = row["started_at"].as<std::string>(),
            .username = row["username"].as<std::string>(),
        });
    }

    return streams;
}
std::vector<Stream> StreamRepository::GetHistoryByUserId(int userId) {
    auto conn = Pool().connection();
    pqxx::work txn(*conn);
    auto result = txn.exec_params(
        "SELECT stream_id, user_id, title, status, started_at, ended_at "
        "FROM streams WHERE user_id = $1 ORDER BY started_at DESC LIMIT 50",
        userId
    );
    txn.commit();
    Pool().freeConnection(conn);


    std::vector<Stream> streams;
    for (const auto& row : result) {
        Stream s;
        s.id= row["stream_id"].as<std::string>();
        s.user_id= row["user_id"].as<int>();
        s.title = row["title"].as<std::string>("");
        s.status = row["status"].as<std::string>();
        s.started_at= row["started_at"].as<std::string>();
        s.ended_at = row["ended_at"].is_null() ? "" : row["ended_at"].as<std::string>();
        streams.push_back(s);
    }
    return streams;
}