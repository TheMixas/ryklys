//
// Created by themi on 3/11/26.
//

#include "Cors.h"

#include <optional>

#include "../HttpResponse.h"

#include "./../HttpRequest.h"

static std::string join_set(const std::unordered_set<std::string> &set, const std::string &sep = ", ") {
    std::string result;
    result.reserve(set.size());
    for (const auto &item: set) {
        if (!result.empty()) result += sep;
        result += item;
    }
    return result;
}
Cors::Cors(CorsConfig config) : config_(std::move(config)) {
    allowed_method_header_ = join_set(config_.allowed_methods);
    allowed_headers_header_ = join_set(config_.allowed_headers);
}

/// Check if the Origin header in the request is allowed.
bool Cors::is_origin_valid(const HttpRequest &req) {
    auto it = req.headers.find("Origin");
    if (it == req.headers.end()) {
        return false; // No Origin header, treat as not allowed
    }

    const std::string &origin = it->second;

    if (config_.allowed_origins.find("*") != config_.allowed_origins.end()) {
        return true; // Allow all origins
    }

    return config_.allowed_origins.find(origin) != config_.allowed_origins.end();

}

/// Handle CORS preflight requests.
/// Assumes that the request is method Options, and has the necessary headers for a preflight. It will check the Origin and Access-Control-Request-Method headers.
/// Creates a response with appropriate CORS headers if it's a valid preflight, or returns std::nullopt if not handled.
std::optional<HttpResponse> Cors::handle_preflight(const HttpRequest &request) {

    if (!is_origin_valid(request)) {
        return std::nullopt; // Origin not allowed, let the main handler return an error
    }

    HttpResponse response = HttpResponse::NoContent();

    auto it_origin = request.headers.find("Origin");
    const std::string &origin = it_origin->second;

    //TODO: alr just finish this straight line. get ur mind into it, itsn ot that deep, but requires foc
    // Start status line
    std::string header = "HTTP/1.1 " + std::to_string(response.status_code) + " " +
                         response.status_text + "\r\n";


    // Add preflight response headers: replace with actual values stores in the cors config.
    response.headers[ACCESS_CONTROL_ALLOW_ORIGIN_KEY] = origin;
    response.headers[ACCESS_CONTROL_ALLOW_METHODS_KEY] = allowed_method_header_;
    response.headers[ACCESS_CONTROL_ALLOW_HEADERS_KEY] = allowed_headers_header_;
    response.headers[ACCESS_CONTROL_ALLOW_CREDENTIALS_KEY] = config_.allow_credentials ? "true" : "false";
    response.headers[ACCESS_CONTROL_MAX_AGE_KEY] = std::to_string(config_.max_age_seconds);


    return response;
}

/// Apply CORS headers to the response
/// Returns bool to indicate if sucessfull in modyfing with cors.
bool Cors::apply_cors_headers(const HttpRequest &req, HttpResponse &res) {

    if (!is_origin_valid(req)) {
        return false; // Origin not allowed, don't add CORS headers
    }
    auto it = req.headers.find("Origin");
    const std::string &origin = it->second;

    res.headers[ACCESS_CONTROL_ALLOW_ORIGIN_KEY] = origin;

    if (!allowed_method_header_.empty()) {
        res.headers[ACCESS_CONTROL_ALLOW_METHODS_KEY] = allowed_method_header_;
    }

    if (!allowed_headers_header_.empty()) {
        res.headers[ACCESS_CONTROL_ALLOW_HEADERS_KEY] = allowed_headers_header_;
    }

    if (config_.allow_credentials) {
        res.headers[ACCESS_CONTROL_ALLOW_CREDENTIALS_KEY] = "true";
    }

    if (config_.max_age_seconds > 0) {
        res.headers[ACCESS_CONTROL_MAX_AGE_KEY] = std::to_string(config_.max_age_seconds);
    }

    return true;
}
