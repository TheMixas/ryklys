//
// Created by themi on 3/22/26.
//

#include "StreamerRoutes.h"
#include "./../../config/WebRTCConfig.h"
#include <iostream>
#include <random>
#include <set>
#include <thread>
#include "../../services/SegmentUploader.h"
#include "rtc/h264rtpdepacketizer.hpp"
#include "rtc/peerconnection.hpp"
#include "rtc/rtcpreceivingsession.hpp"
#include "./../../ServiceLocator.h"
#include "./../../services/StreamCleanup.h"

std::string GenerateStreamId()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);

    std::ostringstream ss;
    ss << std::hex << std::setfill('0')
        << std::setw(8) << dist(gen) << "-"
        << std::setw(4) << (dist(gen) & 0xFFFF) << "-"
        << std::setw(4) << (dist(gen) & 0xFFFF) << "-"
        << std::setw(4) << (dist(gen) & 0xFFFF) << "-"
        << std::setw(8) << dist(gen) << std::setw(4) << (dist(gen) & 0xFFFF);
    return ss.str();
}

// ---------------------------------------------------------
// Networking Helpers
// ---------------------------------------------------------

class SegmentUploader;
// Binds a socket to port 0, asks the OS what port it got, and returns it.
// Finds a free port, then immediately releases it so FFmpeg can use it.
int GetFreeLocalPort()
{
    int tempSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (tempSock < 0) return -1;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1
    addr.sin_port = 0;

    if (bind(tempSock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        close(tempSock);
        return -1;
    }

    socklen_t len = sizeof(addr);
    getsockname(tempSock, (struct sockaddr*)&addr, &len);
    int port = ntohs(addr.sin_port);

    close(tempSock); // Close so ffmpeg can retake

    return port;
}

void SendUdpPacket(int sockFd, int port, const std::byte* data, size_t size)
{
    if (sockFd < 0) return;
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    dest.sin_port = htons(port);

    sendto(sockFd, data, size, 0, (struct sockaddr*)&dest, sizeof(dest));
}

// Generates the SDP map so FFmpeg knows how to decode the UDP packets
std::string GenerateSdpString(int videoPort, int audioPort)
{
    return "v=0\n"
        "o=- 0 0 IN IP4 127.0.0.1\n"
        "s=Ryklys Node Stream\n"
        "c=IN IP4 127.0.0.1\n"
        "t=0 0\n"
        "m=video " + std::to_string(videoPort) + " RTP/AVP 96\n"
        "a=rtpmap:96 H264/90000\n"
        "a=fmtp:96 profile-level-id=42e01f;level-asymmetry-allowed=1;packetization-mode=1\n"
        "m=audio " + std::to_string(audioPort) + " RTP/AVP 111\n"
        "a=rtpmap:111 opus/48000/2\n";
}

void SaveSdpToTempFile(const std::string& filename, const std::string& content)
{
    std::ofstream out(filename);
    out << content;
    out.close();
}

// Holds everything needed to keep a single streamer's WebRTC session alive
struct StreamerSession
{
    std::shared_ptr<rtc::PeerConnection> pc;
    std::vector<std::shared_ptr<rtc::Track>> incomingTracks;
    std::shared_ptr<std::mutex> sessionMutex;

    std::string streamId;
    std::string outputDirectory = "./recordings";
    std::string outputFormat = ".m3u8";
    std::string dbStreamId;
    int videoPacketCounter = 0;

    bool hasRemoteDescription = false;
    std::vector<rtc::Candidate> earlyCandidates;

    FILE* ffmpegProcess = nullptr;
    FILE* debugFile = nullptr;
    int videoSocketFd;
    int videoUdpPort;
    int audioPacketCounter;
    int audioSocketFd;
    int audioUdpPort;

    // Segment uploader
    std::shared_ptr<SegmentUploader> uploader;
    std::thread uploaderThread;
    std::atomic<bool> isLive{true};


    StreamerSession(rtc::Configuration config, std::string id, std::string dir)
        : pc(std::make_shared<rtc::PeerConnection>(config)),
          sessionMutex(std::make_shared<std::mutex>()),
          streamId(std::move(id)), outputDirectory(std::move(dir))
    {
    }

    ~StreamerSession()
    {
        isLive.store(false);

        if (uploaderThread.joinable())
            uploaderThread.join();

        if (ffmpegProcess)
        {
            fprintf(ffmpegProcess, "q\n");
            fflush(ffmpegProcess);
            pclose(ffmpegProcess);
        }

        // Clean up local files after final upload is done
        std::filesystem::remove_all(outputDirectory + "/" + streamId);

        std::cout << "[Node] Cleaned up resources for stream: " << streamId << std::endl;
    }

    void startSegmentUploader()
    {
        uploaderThread = std::thread([this]()
        {
            std::set<std::string> uploaded;
            std::string dir = outputDirectory + "/" + streamId;
            std::string m3u8Name = streamId + outputFormat;

            while (isLive.load())
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));

                if (!std::filesystem::exists(dir)) continue;

                // Upload new .ts segments
                for (auto& entry : std::filesystem::directory_iterator(dir))
                {
                    std::string fname = entry.path().filename().string();
                    if (!fname.ends_with(".ts")) continue;
                    if (uploaded.contains(fname)) continue;

                    if (uploader->upload(entry.path().string(), streamId, fname))
                    {
                        uploaded.insert(fname);
                    }
                }

                // Always re-upload the playlist (it changes every segment)
                std::string m3u8Path = dir + "/" + m3u8Name;
                if (std::filesystem::exists(m3u8Path))
                {
                    uploader->upload(m3u8Path, streamId, m3u8Name);
                }
            }

            // Final pass — upload anything written during shutdown
            if (std::filesystem::exists(dir))
            {
                for (auto& entry : std::filesystem::directory_iterator(dir))
                {
                    std::string fname = entry.path().filename().string();
                    if (fname.ends_with(".ts") && !uploaded.contains(fname))
                    {
                        uploader->upload(entry.path().string(), streamId, fname);
                    }
                }
                std::string m3u8Path = dir + "/" + m3u8Name;
                if (std::filesystem::exists(m3u8Path))
                {
                    uploader->upload(m3u8Path, streamId, m3u8Name);
                }
            }

            std::cout << "[Uploader] Finished for " << streamId << std::endl;
        });
    }
};

std::shared_ptr<StreamerSession> createPeerConnection(rtc::Configuration config,
                                                      std::shared_ptr<WebSocketConnection> ws,
                                                      const std::string& streamId)
{
    auto session = std::make_shared<StreamerSession>(config, streamId, "./recordings");
    auto pc = session->pc;

    std::string streamPocketHost = EnvConfig::Instance().Get("STREAM_POCKET_SERVER_HOST", "localhost");
    std::string streamPocketPort = EnvConfig::Instance().Get("STREAM_POCKET_SERVER_PORT", "7070");
    std::string streamPocketSecret = EnvConfig::Instance().Get("STREAM_POCKET_SERVER_SECRET", "123");
    // Set up segment uploader
    session->uploader = std::make_shared<SegmentUploader>(
        "http://" + streamPocketHost + ":" + streamPocketPort, // Stream pocket url
        streamPocketSecret // StreamPocket`s secret for auth
    );
    session->startSegmentUploader();

    std::filesystem::create_directories(session->outputDirectory);
    std::filesystem::create_directories(session->outputDirectory + "/" + session->streamId);


    // Allocate UDP ports and sockets
    session->videoUdpPort = GetFreeLocalPort();
    session->audioUdpPort = GetFreeLocalPort();
    session->videoSocketFd = socket(AF_INET, SOCK_DGRAM, 0);
    session->audioSocketFd = socket(AF_INET, SOCK_DGRAM, 0);

    std::cout << "[Node] Video UDP port: " << session->videoUdpPort
        << ", Audio UDP port: " << session->audioUdpPort << std::endl;

    auto weakSession = std::weak_ptr<StreamerSession>(session);

    // DON'T start FFmpeg yet — we need to know the actual PT first.

    pc->onStateChange([weakSession](rtc::PeerConnection::State state)
    {
        std::cout << "PeerConnection state: " << state << std::endl;

        if (state == rtc::PeerConnection::State::Disconnected ||
            state == rtc::PeerConnection::State::Failed ||
            state == rtc::PeerConnection::State::Closed)
        {
            auto s = weakSession.lock();
            if (!s || !s->ffmpegProcess) return;
            fprintf(s->ffmpegProcess, "q\n");
            fflush(s->ffmpegProcess);
            pclose(s->ffmpegProcess);
            s->ffmpegProcess = nullptr;
        }
        if (state == rtc::PeerConnection::State::Closed)
        {
            auto s = weakSession.lock();
            if (s)
            {
                std::cout << "[Node] Total video packets: " << s->videoPacketCounter
                    << ", audio packets: " << s->audioPacketCounter << std::endl;
            }
        }
    });

    pc->onLocalDescription([ws](rtc::Description answer)
    {
        nlohmann::json resp = {{"type", "answer"}, {"sdp", std::string(answer)}};
        ws->SendText(resp.dump());
    });

    pc->onLocalCandidate([ws](rtc::Candidate candidate)
    {
        nlohmann::json msg = {
            {"type", "candidate"},
            {"candidate", std::string(candidate)},
            {"mid", candidate.mid()},
        };
        ws->SendText(msg.dump());
    });

    pc->onTrack([weakSession](std::shared_ptr<rtc::Track> track)
    {
        auto session = weakSession.lock();
        if (!session) return;

        session->incomingTracks.push_back(track);
        std::string trackType = track->description().type();
        std::cout << "[Node] Receiving " << trackType << " from " << session->streamId << std::endl;

        track->onMessage([weakSession, trackType](rtc::message_variant data)
        {
            auto session = weakSession.lock();
            if (!session) return;

            if (std::holds_alternative<rtc::binary>(data))
            {
                auto& message = std::get<rtc::binary>(data);
                if (message.size() < 2) return;

                // Filter out RTCP: check the actual byte (not masked)
                // RTP has version=2 in bits 6-7 of byte 0
                // RTCP PT range: 200-206. RTP PT: typically < 128
                // The raw byte[1] for RTCP has the marker bit set: 0xC8-0xCE
                uint8_t byte1 = static_cast<uint8_t>(message[1]);
                if (byte1 >= 200 && byte1 <= 206) return; // RTCP without marker bit
                if ((byte1 & 0x7F) >= 72 && (byte1 & 0x7F) <= 76
                    && message.size() < 200)
                    return; // RTCP with marker bit (small packets)

                uint8_t pt = byte1 & 0x7F;

                if (trackType == "video")
                {
                    int count = session->videoPacketCounter++;

                    if (count == 0)
                    {
                        std::cout << "[Node] First video packet! PT=" << (int)pt
                            << " size=" << message.size() << std::endl;

                        std::string sdpContent =
                            "v=0\n"
                            "o=- 0 0 IN IP4 127.0.0.1\n"
                            "s=Ryklys Node Stream\n"
                            "c=IN IP4 127.0.0.1\n"
                            "t=0 0\n"
                            "m=video " + std::to_string(session->videoUdpPort) +
                            " RTP/AVP " + std::to_string(pt) + "\n"
                            "a=rtpmap:" + std::to_string(pt) + " H264/90000\n"
                            "a=fmtp:" + std::to_string(pt) +
                            " profile-level-id=42e01f;level-asymmetry-allowed=1;"
                            "packetization-mode=1\n";

                        std::string sdpFilename = "/tmp/" + session->streamId + ".sdp";
                        std::ofstream out(sdpFilename);
                        out << sdpContent;
                        out.close();

                        std::string outputPath = session->outputDirectory + "/" + session->streamId + "/" +
                            session->streamId + session->outputFormat;
                        // std::string ffmpegCmd =
                        //     "ffmpeg -hide_banner -loglevel info "
                        //     "-protocol_whitelist file,udp,rtp "
                        //     "-analyzeduration 60000000 "
                        //     "-probesize 100000000 "
                        //     "-fflags +genpts+discardcorrupt "
                        //     "-use_wallclock_as_timestamps 1 " // real-time input pacing
                        //     "-i " + sdpFilename + " "
                        //     "-c:v libx264 -preset ultrafast -tune zerolatency "
                        //     "-crf 23 "
                        //     "-r 30 "
                        //     "-g 60 -keyint_min 60 "
                        //     "-force_key_frames \"expr:gte(t,n_forced*4)\" "
                        //     "-an "
                        //     "-f hls -hls_time 4 -hls_list_size 5 "
                        //     "-hls_flags delete_segments "
                        //     "-flush_packets 1 "
                        //     "-y " + outputPath;
                        std::string ffmpegCmd =
                            "ffmpeg -hide_banner -loglevel info "
                            "-protocol_whitelist file,udp,rtp "
                            "-analyzeduration 2000000 " // 2s
                            "-probesize 1000000 " // was 100000000 — 1MB is enough for RTP
                            "-fflags +genpts+discardcorrupt+nobuffer " // added +nobuffer
                            "-flags low_delay " // NEW — tell decoder to minimize delay
                            "-use_wallclock_as_timestamps 1 "
                            "-i " + sdpFilename + " "
                            "-c:v libx264 -preset ultrafast -tune zerolatency "
                            "-crf 23 "
                            "-r 30 "
                            "-g 30 -keyint_min 30 " // was 60 — keyframe every 1s at 30fps
                            "-an "
                            "-f hls "
                            "-hls_time 1 " // 1 second segments
                            "-hls_list_size 3 " //keep 3 segments
                            "-hls_flags delete_segments+append_list "
                            "-flush_packets 1 "
                            "-y " + outputPath;
                        session->ffmpegProcess = popen(ffmpegCmd.c_str(), "w");
                        std::cout << "[Node] Started FFmpeg with PT=" << (int)pt << std::endl;

                        // Give FFmpeg a moment to bind the UDP port
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    }

                    SendUdpPacket(session->videoSocketFd, session->videoUdpPort,
                                  message.data(), message.size());

                    if (count < 20)
                    {
                        std::cout << "[RTP-VIDEO] pkt#" << count
                            << " size=" << message.size()
                            << " PT=" << (int)pt << std::endl;
                    }
                }
                else if (trackType == "audio")
                {
                    session->audioPacketCounter++;
                    SendUdpPacket(session->audioSocketFd, session->audioUdpPort,
                                  message.data(), message.size());
                }
            }
        });

        track->onClosed([trackType]()
        {
            std::cout << "[Node] Track closed: " << trackType << std::endl;
        });
    });

    return session;
}

void RegisterStreamerRoutes(ZvejysServer& server)
{
    server.RegisterRoute(HttpMethod::GET, "/api/streams/live",
                         [](const HttpRequest& req) -> HttpResponse
                         {
                             auto& repo = ServiceLocator::GetStreamRepository();
                             auto streams = repo.GetActive();
                             auto& redis = ServiceLocator::GetRedisClient();
                             nlohmann::json arr = nlohmann::json::array();
                             for (auto& s : streams)
                             {
                                 auto countOpt = redis.get("viewers:" + s.username);
                                 int viewerCount = countOpt.has_value() ? std::stoi(countOpt.value()) : 0;
                                 arr.push_back({
                                     {"streamId", s.id},
                                     {"viewerCount", viewerCount},
                                     {"userId", s.user_id},
                                     {"title", s.title},
                                     {"startedAt", s.started_at},
                                     {"username", s.username},
                                 });
                             }
                             return HttpResponse::Json(arr.dump());
                         });
    server.RegisterAuthenticatedWebSocketRoute(
        "/sig", [](std::shared_ptr<WebSocketConnection> ws, const AuthenticatedUser& user)
        {
            // Create stream in DB
            std::string streamDbId = GenerateStreamId();
            auto& repo = ServiceLocator::GetStreamRepository();
            auto stream = repo.Create(user.id, streamDbId, "Live Stream");

            std::cout << "[Sig] Stream " << streamDbId << " by " << user.username << std::endl;

            // 1. Create the PeerConnection ONCE when the client first connects
            rtc::Configuration webRtcConfig = WebRTCConfig::load();
            //auto pc = createPeerConnection(webRtcConfig, ws);
            auto session = createPeerConnection(webRtcConfig, ws, user.username);
            session->dbStreamId = streamDbId;
            ws->SetOnMessage([session](std::shared_ptr<WebSocketConnection> ws_,
                                       const std::vector<uint8_t>& data,
                                       WsOpcode opcode)
            {
                std::lock_guard<std::mutex> lock(*(session->sessionMutex));
                if (opcode == WsOpcode::TEXT)
                {
                    std::string msg(data.begin(), data.end());
                    try
                    {
                        nlohmann::json payload = nlohmann::json::parse(msg);
                        std::string type = payload.value("type", "");

                        if (type == "offer")
                        {
                            rtc::Description offer(
                                payload["sdp"].get<std::string>(),
                                rtc::Description::Type::Offer
                            );
                            session->pc->setRemoteDescription(offer);

                            // Flush any candidates that arrived before the offer
                            for (auto& queued : session->earlyCandidates)
                            {
                                session->pc->addRemoteCandidate(queued);
                                std::cout << "Added queued candidate: " << queued << std::endl;
                            }
                            session->earlyCandidates.clear();
                            session->hasRemoteDescription = true;
                        }
                        else if (type == "candidate")
                        {
                            std::string candidateStr = payload["candidate"]["candidate"].get<std::string>();

                            // Skip empty end-of-candidates signal
                            if (candidateStr.empty()) return;

                            if (candidateStr.rfind("a=", 0) == 0)
                                candidateStr = candidateStr.substr(2);
                            // Also strip "candidate:" prefix if missing "a="
                            // (browser sends "candidate:..." not "a=candidate:...")

                            std::string sdpMid = payload["candidate"]["sdpMid"].get<std::string>();
                            rtc::Candidate candidate(candidateStr, sdpMid);

                            if (session->hasRemoteDescription)
                            {
                                session->pc->addRemoteCandidate(candidate);
                                std::cout << "Added remote candidate: " << candidate << std::endl;
                            }
                            else
                            {
                                session->earlyCandidates.push_back(candidate);
                                std::cout << "Queued early candidate: " << candidate << std::endl;
                            }
                        }
                        else
                        {
                            std::cerr << "Unknown message type: " << type << std::endl;
                        }
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "Failed to parse signaling message: " << e.what() << std::endl;
                    }
                }
            });

            ws->SetOnClose([session, user](std::shared_ptr<WebSocketConnection> ws_,
                                           uint16_t code,
                                           const std::string& reason)
            {
                if (code < 1000 || code > 4999)
                {
                    std::cerr << "Received invalid close code " << code
                        << " — likely a non-WebSocket client, ignoring." << std::endl;
                    return;
                }

                session->isLive.store(false);

                auto& repo = ServiceLocator::GetStreamRepository();
                repo.SetEnded(session->dbStreamId);

                CleanupStream(user.username);

                std::cout << "WS closed: " << code << " " << reason << ". Killing WebRTC session." << std::endl;
                session->pc->close(); // This triggers the chain reaction that safely kills FFmpeg
            });
        });
    server.RegisterAuthenticatedRoute(HttpMethod::GET, "/api/users/me/streams",
                                      [](const HttpRequest& request, const AuthenticatedUser& authUser) -> HttpResponse
                                      {
                                          std::cout << "[ME/STREAMS] Fetching stream history for user ID=" << authUser.
                                              id << std::endl;
                                          auto streamRepo = ServiceLocator::GetStreamRepository();
                                          auto streams = streamRepo.GetHistoryByUserId(authUser.id);

                                          std::string json = "[";
                                          for (size_t i = 0; i < streams.size(); i++)
                                          {
                                              if (i > 0) json += ",";
                                              json += "{\"id\":\"" + streams[i].id + "\""
                                                  + ",\"title\":\"" + streams[i].title + "\""
                                                  + ",\"status\":\"" + streams[i].status + "\""
                                                  + ",\"started_at\":\"" + streams[i].started_at + "\""
                                                  + ",\"ended_at\":\"" + streams[i].ended_at.value() + "\"}";
                                          }
                                          json += "]";

                                          return HttpResponse::Json(json);
                                      });
}
