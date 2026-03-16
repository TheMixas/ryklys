//
// Created by themi on 2/19/26.
//

#include "HttpConnection.h"
#include "ZvejysServer.h"

#include <cerrno>
#include <charconv>
#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>

#include "WebSocketUtils.h"

//HandleRequest -> { Read -> Parse -> the body of this function handling the request, generate a response -> SendResponse }

void HttpConnection::OnReadable() {

    // ── Phase 1: Drain the socket into the buffer ────────────────────────────
    while (true) {
        size_t old_size = read_buffer_.size();
        read_buffer_.resize(old_size + MAX_READ_CHUNK_SIZE);

        ssize_t bytes_read = recv(GetSocketFD(), read_buffer_.data() + old_size, MAX_READ_CHUNK_SIZE, 0);

        if (bytes_read == -1) {
            read_buffer_.resize(old_size); // undo the speculative resize
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break; // No more data right now
            else {
                perror("HttpConnection::OnReadable recv");
                return;
            }
        }

        if (bytes_read == 0) {
            // Client closed the connection
            read_buffer_.resize(old_size);
            return;
        }

        read_buffer_.resize(old_size + bytes_read);
    }

    // ── Phase 2: Check if we have a complete HTTP request ───────────────────
    // A complete request requires at minimum a full header block
    std::string_view raw(read_buffer_.data(), read_buffer_.size());

    size_t header_end = raw.find("\r\n\r\n");
    if (header_end == std::string_view::npos)
        return; // Haven't received full headers yet — wait for more data

    // Check Content-Length to see if we also have the full body
    size_t body_start = header_end + 4; // skip past the "\r\n\r\n"
    size_t content_length = 0;

    std::string_view header_section = raw.substr(0, header_end);
    size_t cl_pos = header_section.find("Content-Length:");
    if (cl_pos == std::string_view::npos)
        cl_pos = header_section.find("content-length:"); // case fallback

    if (cl_pos != std::string_view::npos) {
        size_t val_start = header_section.find_first_not_of(" \t", cl_pos + 15);
        size_t val_end   = header_section.find("\r\n", val_start);
        std::string_view cl_str = header_section.substr(val_start, val_end - val_start);
        std::from_chars(cl_str.data(), cl_str.data() + cl_str.size(), content_length);
    }

    if (read_buffer_.size() < body_start + content_length)
        return; // Body not fully received yet — wait for more data

    // ── Phase 3: We have a complete request — parse and handle it ───────────
    std::vector<char> complete_request(read_buffer_.begin(),
                                       read_buffer_.begin() + body_start + content_length);

    // Consume the request from the buffer (supports pipelining)
    read_buffer_.erase(read_buffer_.begin(),
                       read_buffer_.begin() + body_start + content_length);

    HttpRequest request = Parse(complete_request);
    HandleRead(std::move(request));
}

//Handles the full request
void HttpConnection::HandleRead(HttpRequest request) {
    // std::vector<char> buffer = Read();
    // HttpRequest request = Parse(buffer);

    // ─── CHECK FOR WEBSOCKET UPGRADE ───
    auto upgrade_it = request.headers.find("Upgrade");
    auto conn_it = request.headers.find("Connection");
    auto wskey_it = request.headers.find("Sec-WebSocket-Key");

    bool isWsUpgrade = (request.method == HttpMethod::GET)
                       && (upgrade_it != request.headers.end() && upgrade_it->second == "websocket")
                       && (conn_it != request.headers.end()) // "Upgrade" should be in Connection
                       && (wskey_it != request.headers.end());

    bool isCorsPreflight = (request.method == HttpMethod::OPTIONS)
                           && (request.headers.find("Origin") != request.headers.end())
                           && (request.headers.find("Access-Control-Request-Method") != request.headers.end());

    if (isWsUpgrade) {
        // Compute the accept key
        std::string accept_key = ws_crypto::ComputeAcceptKey(wskey_it->second);

        // Send the 101 Switching Protocols response
        std::string handshake_response =
                "HTTP/1.1 101 Switching Protocols\r\n"
                "Upgrade: websocket\r\n"
                "Connection: Upgrade\r\n"
                "Sec-WebSocket-Accept: " + accept_key + "\r\n"
                "\r\n";

        send(GetSocketFD(), handshake_response.data(),
             handshake_response.size(), MSG_NOSIGNAL);

        // ─── UPGRADE: Replace this HttpConnection with a WebSocketConnection ───
        // Tell the server to perform the swap
        GetServer()->UpgradeToWebSocket(GetSocketFD(), epollFD, request.path);
    } else if (isCorsPreflight) {
        std::optional<HttpResponse> response = server_->GetCors().handle_preflight(request);
        if (!response.has_value()) {
            response = HttpResponse::Forbidden("CORS preflight failed: Origin not allowed");
        }
        response_queue_.push(std::move(response.value()));
        //Arm for writing/sending
        epoll_event sendEvent{};
        sendEvent.data.ptr = this;
        sendEvent.events = EPOLLOUT | EPOLLET; // Edge-triggered write event
        epoll_ctl(epollFD, EPOLL_CTL_MOD, GetSocketFD(), &sendEvent);
    } else {
        //Handle regular http request

        HttpResponse response;
        //Look up at the MapRouting
        auto handler_it = GetServer()->GetRouteMap().find(request.path);
        if (handler_it != GetServer()->GetRouteMap().end()) {
            // response = handler_it.value().handler(request);
            //response = handler_it->second.handler(request);
            auto node = handler_it->second;
            response = node.get()->HandleRequest(request);
        } else {
            response = HttpResponse::NotFound("Route not found");
        }

        //apply CORS headers if needed
        GetServer()->GetCors().apply_cors_headers(request, response);

        response_queue_.push(std::move(response));
        //Arm for writing/sending
        epoll_event sendEvent{};
        sendEvent.data.ptr = this;
        sendEvent.events = EPOLLOUT | EPOLLET; // Edge-triggered write event
        epoll_ctl(epollFD, EPOLL_CTL_MOD, GetSocketFD(), &sendEvent);
    }
}


void HttpConnection::OnWritable(int epollFD) {
    while (!response_queue_.empty()) {
        const HttpResponse &response = response_queue_.front();


        // Start status line
        std::string header = "HTTP/1.1 " + std::to_string(response.status_code) + " " +
                             response.status_text + "\r\n";


        // Add any custom headers from the response object (Content-Type, Set-Cookie, etc.)
        for (const auto &pair: response.headers) {
            header += pair.first + ": " + pair.second + "\r\n";
        }

        // Add required headers
        header += "Content-Length: " + std::to_string(response.body.size()) + "\r\n";
        header += "Connection: keep-alive\r\n";
        header += "\r\n";

        // Send headers and body
        ssize_t sent = send(GetSocketFD(), header.data(), header.size(), MSG_NOSIGNAL);
        if (sent == -1) {
            if (errno == EPIPE || errno == ECONNRESET) {
                // Client disconnected
                return;
            } else {
                perror("Connection::HandleWrite");
                return;
            }
        }
        send(GetSocketFD(), response.body.data(), response.body.size(), MSG_NOSIGNAL);
        response_queue_.pop();
    }

    // After sending all responses, arm for reading again
    epoll_event readEvent{};
    readEvent.data.ptr = this;
    readEvent.events = EPOLLIN | EPOLLET; // Edge-triggered read event
    epoll_ctl(epollFD, EPOLL_CTL_MOD, GetSocketFD(), &readEvent);
}

//Reads and then parses
std::vector<char> HttpConnection::Read() const {
    std::vector<char> buffer;

    buffer.reserve(READ_BUFFER_RESERVE);

    while (true) {
        size_t old_size = buffer.size();
        buffer.resize(old_size + MAX_READ_CHUNK_SIZE);
        if (GetSocketFD() == -1) {
            throw std::runtime_error("Invalid socket file descriptor");
        }

        ssize_t bytes_read = recv(GetSocketFD(), buffer.data() + old_size, MAX_READ_CHUNK_SIZE, 0);

        if (bytes_read == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            else
                perror("Connection::HandleRead");
        }

        if (bytes_read == 0) {
            // Client closed the connection
            return {};
        }

        buffer.resize(old_size + bytes_read);
    }

    return buffer;
}

/// Parse the passed in buffer as an HTTP request
HttpRequest HttpConnection::Parse(const std::vector<char> &buffer) const {
    HttpRequest request;

    if (buffer.empty())
        return request;

    std::string raw(buffer.begin(), buffer.end());


    // Find end of headers (double CRLF)
    size_t header_end = raw.find("\r\n\r\n");
    if (header_end == std::string::npos)
        throw std::runtime_error("Malformed HTTP request: no header terminator");

    std::string header_section = raw.substr(0, header_end);
    size_t body_start = header_end + 4;

    // Parse request line
    size_t first_line_end = header_section.find("\r\n");
    std::string request_line = header_section.substr(0, first_line_end);

    std::istringstream request_line_stream(request_line);
    std::string method_str;
    request_line_stream >> method_str >> request.path >> request.version;
    request.method = GetEnumForString(method_str);

    // // Separate query string from path
    // size_t query_pos = request.path.find('?');
    // if (query_pos != std::string::npos) {
    //     request.query_string = request.path.substr(query_pos + 1);
    //     request.path = request.path.substr(0, query_pos);
    // }
    size_t query_pos = request.path.find('?');
    if (query_pos != std::string::npos) {
        request.query_string = request.path.substr(query_pos + 1);
        request.path = request.path.substr(0, query_pos);

        // Parse query params: key1=val1&key2=val2
        std::string &qs = request.query_string;
        size_t pos = 0;
        while (pos < qs.size()) {
            size_t amp = qs.find('&', pos);
            if (amp == std::string::npos) amp = qs.size();
            size_t eq = qs.find('=', pos);
            if (eq != std::string::npos && eq < amp) {
                request.query_params[qs.substr(pos, eq - pos)] = qs.substr(eq + 1, amp - eq - 1);
            }
            pos = amp + 1;
        }
    }

    // Parse headers
    size_t pos = first_line_end + 2; // skip past first \r\n
    while (pos < header_section.size()) {
        size_t line_end = header_section.find("\r\n", pos);
        if (line_end == std::string::npos)
            line_end = header_section.size();

        std::string line = header_section.substr(pos, line_end - pos);
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);

            // Trim leading whitespace from value
            size_t val_start = value.find_first_not_of(' ');
            if (val_start != std::string::npos)
                value = value.substr(val_start);

            request.headers[key] = value;
        }

        pos = line_end + 2;
    }

    // Extract body
    if (body_start < raw.size()) {
        request.body.assign(raw.begin() + body_start, raw.end());
    }

    // Parse JSON body into body_params (flat keys only)
    auto ct = request.headers.find("Content-Type");
    if (ct != request.headers.end() && ct->second.find("application/json") != std::string::npos
        && !request.body.empty()) {
        try {
            json body = json::parse(request.BodyString());
            for (auto &[key, value]: body.items()) {
                if (value.is_string()) {
                    request.body_params[key] = value.get<std::string>();
                } else {
                    request.body_params[key] = value.dump(); // numbers, bools, etc. as string
                }
            }
        } catch (...) {
            // Malformed JSON — body_params stays empty, handler can check
        }
    }
    return request;
}
