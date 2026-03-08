#ifndef RYKLYS_BACKEND_HTTPREQUEST_H
#define RYKLYS_BACKEND_HTTPREQUEST_H

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "HttpMethod.h"
#include "HttpResponse.h"

struct HttpRequest {
    HttpMethod method;
    std::string path;
    std::string version;
    std::string query_string;
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> query_params;
    std::unordered_map<std::string, std::string> body_params;
    std::vector<char> body;

    // Get body as a string
    std::string BodyString() const {
        return std::string(body.begin(), body.end());
    }

    // Get a query param with a default fallback
    std::optional<std::string> QueryParam(const std::string &key) const {
        auto it = query_params.find(key);
        if (it != query_params.end()) {
            return it->second;
        }
        return std::nullopt;
    }


    std::optional<std::string> BodyParam(const std::string &key) const {
        auto it = body_params.find(key);
        if (it != body_params.end()) {
            return it->second;
        }
        return std::nullopt;
    }
};

#endif //RYKLYS_BACKEND_HTTPREQUEST_H

