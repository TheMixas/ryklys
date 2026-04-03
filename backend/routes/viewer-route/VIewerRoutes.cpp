#include "../../ServiceLocator.h"
#include <iostream>
#include "./../../zvejys-rest-api/ZvejysServer.h"

void RegisterViewerRoutes(ZvejysServer& server)
{
    server.RegisterAuthenticatedWebSocketRoute("/ws/view",
                                               [](std::shared_ptr<WebSocketConnection> ws,
                                                  const AuthenticatedUser& user)
                                               {
                                                   const auto& req = ws->GetHttpRequest();
                                                   auto it = req.query_params.find("streamId");
                                                   if (it == req.query_params.end() || it->second.empty())
                                                   {
                                                       ws->Close(1008, "Missing streamId");
                                                       return;
                                                   }

                                                   std::string streamId = it->second;
                                                   auto& registry = ServiceLocator::GetViewerRegistry();
                                                   auto& pool = ServiceLocator::GetRedisPool();

                                                   registry.Add(streamId, ws);

                                                   int count;
                                                   {
                                                       auto redis = pool.Get();
                                                       count = redis->incr("viewers:" + streamId);
                                                   }

                                                   nlohmann::json countMsg = {
                                                       {"type", "viewer_count"},
                                                       {"count", count},
                                                   };
                                                   registry.Broadcast(streamId, countMsg.dump());

                                                   std::cout << "[Viewer] Joined " << streamId
                                                       << " (" << count << " watching)" << std::endl;
                                                   ws->SetOnMessage([streamId, user](
                                                       std::shared_ptr<WebSocketConnection> ws_,
                                                       const std::vector<uint8_t>& data,
                                                       WsOpcode opcode)
                                                       {
                                                           if (opcode != WsOpcode::TEXT) return;

                                                           std::string raw(data.begin(), data.end());
                                                           try
                                                           {
                                                               auto msg = nlohmann::json::parse(raw);
                                                               if (msg.value("type", "") != "chat") return;

                                                               std::string text = msg.value("text", "");
                                                               std::string username = user.username;
                                                               int userID = user.id;
                                                               if (text.empty() || text.size() > 500) return;

                                                               // Persist first
                                                               auto& chatRepo = ServiceLocator::GetChatRepository();
                                                               auto saved = chatRepo.Insert(
                                                                   streamId, std::to_string(userID), username, text);

                                                               // Build broadcast with the DB-assigned timestamp for consistency
                                                               nlohmann::json broadcast = {
                                                                   {"type", "chat"},
                                                                   {"id", saved.id},
                                                                   {"user_id", saved.user_id},
                                                                   {"username", saved.username},
                                                                   {"text", saved.message},
                                                                   {"createdAt", saved.created_at},
                                                               };

                                                               // Then publish to all nodes
                                                               auto& pool = ServiceLocator::GetRedisPool();
                                                               auto redis = pool.Get();
                                                               std::cout << "About to publish " << broadcast.dump() <<
                                                                   std::endl;
                                                               redis->publish("chat:" + streamId, broadcast.dump());
                                                           }
                                                           catch (...)
                                                           {
                                                           }
                                                       });
                                                   ws->SetOnClose([streamId](std::shared_ptr<WebSocketConnection> ws_,
                                                                             uint16_t code,
                                                                             const std::string& reason)
                                                   {
                                                       auto& registry = ServiceLocator::GetViewerRegistry();
                                                       auto& pool = ServiceLocator::GetRedisPool();

                                                       registry.Remove(streamId, ws_);

                                                       auto redis = pool.Get();
                                                       int count = redis->decr("viewers:" + streamId);
                                                       if (count < 0)
                                                       {
                                                           redis->set("viewers:" + streamId, "0");
                                                           count = 0;
                                                       }

                                                       nlohmann::json countMsg = {
                                                           {"type", "viewer_count"},
                                                           {"count", count},
                                                       };
                                                       registry.Broadcast(streamId, countMsg.dump());

                                                       std::cout << "[Viewer] Left " << streamId
                                                           << " (" << count << " watching)" << std::endl;
                                                   });
                                               });

    server.RegisterRoute(HttpMethod::GET, "/api/streams/{streamId}/chat",
                         [](const HttpRequest& req) -> HttpResponse
                         {
                             auto streamIdOpt = req.PathParam("streamId");
                             if (!streamIdOpt.has_value())
                             {
                                 return HttpResponse::BadRequest("Missing streamId");
                             }

                             int limit = 50;
                             auto limitParam = req.QueryParam("limit");
                             if (limitParam.has_value())
                             {
                                 try
                                 {
                                     limit = std::stoi(limitParam.value());
                                     if (limit < 1) limit = 1;
                                     if (limit > 200) limit = 200;
                                 }
                                 catch (...)
                                 {
                                     limit = 50;
                                 }
                             }

                             auto& chatRepo = ServiceLocator::GetChatRepository();
                             auto messages = chatRepo.GetRecent(streamIdOpt.value(), limit);

                             nlohmann::json arr = nlohmann::json::array();
                             for (const auto& m : messages)
                             {
                                 arr.push_back({
                                     {"id", m.id},
                                     {"username", m.username},
                                     {"text", m.message},
                                     {"createdAt", m.created_at},
                                 });
                             }

                             return HttpResponse::Json(arr.dump());
                         });
}
