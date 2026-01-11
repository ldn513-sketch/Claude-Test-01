/*
 * SODA Player - HTTP Client
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#ifndef SODA_HTTP_CLIENT_HPP
#define SODA_HTTP_CLIENT_HPP

#include "soda.hpp"
#include <curl/curl.h>

namespace soda {

struct HttpResponse {
    int statusCode = 0;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
    std::string error;
    bool success() const { return statusCode >= 200 && statusCode < 300; }
};

struct HttpRequest {
    std::string url;
    std::string method = "GET";
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    int timeoutSeconds = 30;
    bool followRedirects = true;
    int maxRedirects = 10;
};

class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    // Simple GET request
    HttpResponse get(const std::string& url);
    HttpResponse get(const std::string& url,
                     const std::unordered_map<std::string, std::string>& headers);

    // POST request
    HttpResponse post(const std::string& url, const std::string& body,
                      const std::string& contentType = "application/json");

    // Full request control
    HttpResponse request(const HttpRequest& request);

    // Download file with progress
    using ProgressCallback = std::function<void(int64_t downloaded, int64_t total)>;
    Result<void> downloadFile(const std::string& url, const Path& destination,
                              ProgressCallback progress = nullptr);

    // Streaming download
    using DataCallback = std::function<void(const char* data, size_t size)>;
    Result<void> stream(const std::string& url, DataCallback callback);

    // Configuration
    void setUserAgent(const std::string& userAgent);
    void setTimeout(int seconds);
    void setProxy(const std::string& proxy);

    // URL utilities
    static std::string urlEncode(const std::string& str);
    static std::string urlDecode(const std::string& str);
    static std::string buildQueryString(const std::unordered_map<std::string, std::string>& params);

private:
    static size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
    static size_t headerCallback(char* buffer, size_t size, size_t nitems, void* userdata);
    static int progressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                                curl_off_t ultotal, curl_off_t ulnow);

    CURL* curl_ = nullptr;
    std::string userAgent_ = "SODA-Player/0.1.0";
    int timeout_ = 30;
    std::string proxy_;

    std::mutex mutex_;
};

} // namespace soda

#endif // SODA_HTTP_CLIENT_HPP
