//
// Created by themi on 2/22/26.
//

#ifndef RYKLYS_BACKEND_HTTPRESPONSE_H
#define RYKLYS_BACKEND_HTTPRESPONSE_H
#include <string>
#include <unordered_map>
#include <vector>


struct HttpResponse {
    int status_code = 200;
    std::string status_text = "OK";
    std::unordered_map<std::string, std::string> headers;
    std::vector<char> body;
};


#endif //RYKLYS_BACKEND_HTTPRESPONSE_H