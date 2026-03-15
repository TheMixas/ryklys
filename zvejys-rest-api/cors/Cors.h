//
// Created by themi on 3/11/26.
//

#ifndef RYKLYS_BACKEND_CORS_H
#define RYKLYS_BACKEND_CORS_H
#include <optional>
#include <string>
#include <unordered_set>


struct HttpRequest;
struct HttpResponse;
struct CorsConfig {
    std::unordered_set<std::string> allowed_origins;
    bool allow_credentials = false;
    std::unordered_set<std::string> allowed_methods;
    std::unordered_set<std::string> allowed_headers;
    int max_age_seconds = 0;
};
class Cors {
public:
    explicit Cors(CorsConfig config);
    bool is_origin_valid(const HttpRequest &req);

    std::optional<HttpResponse> handle_preflight(const HttpRequest &request);

    bool apply_cors_headers(const HttpRequest &req, HttpResponse &res);

private:
    static const inline std::string ACCESS_CONTROL_ALLOW_ORIGIN_KEY = "Access-Control-Allow-Origin";
    static const inline std::string ACCESS_CONTROL_ALLOW_METHODS_KEY = "Access-Control-Allow-Methods";
    static const inline std::string ACCESS_CONTROL_ALLOW_HEADERS_KEY = "Access-Control-Allow-Headers";
    static const inline std::string ACCESS_CONTROL_ALLOW_CREDENTIALS_KEY = "Access-Control-Allow-Credentials";
    static const inline std::string ACCESS_CONTROL_MAX_AGE_KEY = "Access-Control-Max-Age";

    CorsConfig config_;

    //cached header strings built from the sets
    std::string allowed_method_header_;
    std::string allowed_headers_header_;
};


#endif //RYKLYS_BACKEND_CORS_H
