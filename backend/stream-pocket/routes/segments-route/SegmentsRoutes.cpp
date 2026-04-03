//
// Created by gusbunto on 3/30/26.
//

#include "SegmentsRoutes.h"

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "ZvejysServer.h"
#include "env/EnvConfig.h"
#include "./backend/stream-pocket/StorageConfig.h"

void RegisterSegmentRoutes(ZvejysServer& server)
{
    server.RegisterRoute(HttpMethod::GET, "/api/segments/{streamId}/{filename}",
                         [](const HttpRequest& req) -> HttpResponse
                         {
                             auto streamId = req.PathParam("streamId").value();
                             auto filename = req.PathParam("filename").value();

                             // Block anything that isn't alphanumeric, dash, underscore, or dot
                             auto isSafe = [](const std::string& s)
                             {
                                 return !s.empty() && s.find("..") == std::string::npos
                                     && s.find('/') == std::string::npos
                                     && s.find('\\') == std::string::npos
                                     && s.find('%') == std::string::npos
                                     && s.find('\0') == std::string::npos;
                             };

                             if (!isSafe(streamId) || !isSafe(filename))
                                 return HttpResponse::Forbidden("Invalid path");

                             // Only allow .ts and .m3u8 files
                             if (!filename.ends_with(".ts") && !filename.ends_with(".m3u8"))
                                 return HttpResponse::Forbidden("Invalid file type");

                             std::string path = StorageConfig::BaseDir() + "/" + streamId + "/" + filename;
                             return HttpResponse::File(path);
                         });

    server.RegisterRoute(HttpMethod::PUT, "/api/segments/{streamId}/{filename}",
                         [](const HttpRequest& req) -> HttpResponse
                         {
                             // Auth
                             const std::string secret = EnvConfig::Instance().Get("STREAM_POCKET_SECRET", "123");
                             auto authHeader = req.headers.find("X-Storage-Key");

                             if (authHeader == req.headers.end() || authHeader->second != secret)
                                 return HttpResponse::Unauthorized("Invalid or missing storage key");

                             // Path params
                             auto streamId = req.PathParam("streamId").value();
                             auto filename = req.PathParam("filename").value();

                             // Safety checks
                             auto isSafe = [](const std::string& s)
                             {
                                 return !s.empty() && s.find("..") == std::string::npos
                                     && s.find('/') == std::string::npos
                                     && s.find('\\') == std::string::npos
                                     && s.find('%') == std::string::npos
                                     && s.find('\0') == std::string::npos;
                             };

                             if (!isSafe(streamId) || !isSafe(filename))
                                 return HttpResponse::Forbidden("Invalid path");

                             if (!filename.ends_with(".ts") && !filename.ends_with(".m3u8"))
                                 return HttpResponse::Forbidden("Invalid file type");

                             // Write file
                             std::string dir = StorageConfig::BaseDir() + "/" + streamId;
                             std::filesystem::create_directories(dir);

                             std::string path = dir + "/" + filename;
                             std::ofstream out(path, std::ios::binary);
                             if (!out.is_open())
                                 return HttpResponse::InternalServerError("Failed to write file");

                             out.write(req.body.data(), req.body.size());
                             out.close();

                             return HttpResponse::Ok("Stored");
                         });
    server.RegisterRoute(HttpMethod::DELETE, "/api/segments/{streamId}/{filename}",
                         [](const HttpRequest& req) -> HttpResponse
                         {
                             // Auth
                             const std::string secret = EnvConfig::Instance().Get("STREAM_POCKET_SECRET", "123");
                             auto authHeader = req.headers.find("X-Storage-Key");

                             if (authHeader == req.headers.end() || authHeader->second != secret)
                                 return HttpResponse::Unauthorized("Invalid or missing storage key");

                             // Path params
                             auto streamId = req.PathParam("streamId").value();
                             auto filename = req.PathParam("filename").value();

                             // Safety
                             auto isSafe = [](const std::string& s)
                             {
                                 return !s.empty() && s.find("..") == std::string::npos
                                     && s.find('/') == std::string::npos
                                     && s.find('\\') == std::string::npos
                                     && s.find('%') == std::string::npos
                                     && s.find('\0') == std::string::npos;
                             };

                             if (!isSafe(streamId) || !isSafe(filename))
                                 return HttpResponse::Forbidden("Invalid path");

                             std::string path = StorageConfig::BaseDir() + "/" + streamId + "/" + filename;

                             if (std::filesystem::remove(path))
                                 return HttpResponse::Ok("Deleted");

                             return HttpResponse::NotFound("File not found");
                         });
}
