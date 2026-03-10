//
// Created by themi on 2/22/26.
//

#ifndef RYKLYS_BACKEND_HTTPRESPONSE_H
#define RYKLYS_BACKEND_HTTPRESPONSE_H
#include <string>
#include <unordered_map>
#include <vector>


struct HttpResponse {
    int status_code;
    std::string status_text;
    std::unordered_map<std::string, std::string> headers;
    std::vector<char> body;

    static HttpResponse Ok(const std::string &body) {
        HttpResponse res;
        res.status_code = 200;
        res.status_text = "OK";
        res.body.assign(body.begin(), body.end());
        return res;
    }

    static HttpResponse Created(const std::string &body) {
        HttpResponse res;
        res.status_code = 201;
        res.status_text = "Created";
        res.body.assign(body.begin(), body.end());
        return res;
    }

    static HttpResponse BadRequest(const std::string &body) {
        HttpResponse res;
        res.status_code = 400;
        res.status_text = "Bad Request";
        res.body.assign(body.begin(), body.end());
        return res;
    }

    static HttpResponse Unauthorized(const std::string &body) {
        HttpResponse res;
        res.status_code = 401;
        res.status_text = "Unauthorized";
        res.body.assign(body.begin(), body.end());
        return res;
    }

    static HttpResponse NotFound() {
        HttpResponse res;
        res.status_code = 404;
        res.status_text = "Not Found";
        res.body.assign({'N','o','t',' ','F','o','u','n','d'});
        return res;
    }

    static HttpResponse Conflict(const std::string &body) {
        HttpResponse res;
        res.status_code = 409;
        res.status_text = "Conflict";
        res.body.assign(body.begin(), body.end());
        return res;
    }

    static HttpResponse UnprocessableEntity(const std::string &body) {
        HttpResponse res;
        res.status_code = 422;
        res.status_text = "Unprocessable Entity";
        res.body.assign(body.begin(), body.end());
        return res;
    }

    static HttpResponse Json(const std::string &json) {
        HttpResponse res;
        res.status_code = 200;
        res.status_text = "OK";
        res.headers["Content-Type"] = "application/json";
        res.body.assign(json.begin(), json.end());
        return res;
    }

    static HttpResponse InternalServerError(const std::string &body) {
        HttpResponse res;
        res.status_code = 500;
        res.status_text = "Internal Server Error";
        res.body.assign(body.begin(), body.end());
        return res;
    }
};


#endif //RYKLYS_BACKEND_HTTPRESPONSE_H