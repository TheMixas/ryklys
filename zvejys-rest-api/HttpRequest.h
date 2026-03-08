#ifndef RYKLYS_BACKEND_HTTPREQUEST_H
#define RYKLYS_BACKEND_HTTPREQUEST_H

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
    std::vector<char> body;
};

#endif //RYKLYS_BACKEND_HTTPREQUEST_H

