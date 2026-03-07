//
// Created by themi on 2/19/26.
//

#include "HttpConnection.h"
#include "ZvejysServer.h"

#include <cerrno>
#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>

//HandleRequest -> { Read -> Parse -> the body of this function handling the request, generate a response -> SendResponse }

void HttpConnection::HandleRequest(int epollFD) {
    std::vector<char> buffer = Read();
    HttpRequest request = Parse(buffer);

    HttpResponse response;
    //Look up at the MapRouting
    auto handler_it = GetServer()->GetRouteMap().find(request.path);
    if (handler_it != GetServer()->GetRouteMap().end()) {
        response = handler_it.value().handler(request);
    } else {
        // If no handler found, send a 404 response
        response.status_code = 404;
        response.status_text = "Not Found";
        response.body = std::vector<char>{'N', 'o', 't', ' ', 'F', 'o', 'u', 'n', 'd'};
    }

    response_queue_.push(std::move(response));

    //Arm for writing
    epoll_event sendEvent{};
    sendEvent.data.ptr = this;
    sendEvent.events = EPOLLOUT | EPOLLET; // Edge-triggered write event
    epoll_ctl(epollFD, EPOLL_CTL_MOD, GetSocketFD(), &sendEvent);

}

void HttpConnection::HandleWrite(int epollFD) {
    while (!response_queue_.empty()) {
        const HttpResponse &response = response_queue_.front();
        std::string header = "HTTP/1.1 " + std::to_string(response.status_code) + " " +
                     response.status_text + "\r\n" +
                     "Content-Length: " + std::to_string(response.body.size()) + "\r\n" +
                     "Connection: keep-alive\r\n" +
                     "\r\n";

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

    // Separate query string from path
    size_t query_pos = request.path.find('?');
    if (query_pos != std::string::npos) {
        request.query_string = request.path.substr(query_pos + 1);
        request.path = request.path.substr(0, query_pos);
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

    return request;
}
