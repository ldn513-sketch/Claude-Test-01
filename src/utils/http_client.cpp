/*
 * SODA Player - HTTP Client Implementation
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#include <soda/http_client.hpp>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace soda {

struct WriteContext {
    std::string* data;
};

struct HeaderContext {
    std::unordered_map<std::string, std::string>* headers;
};

struct ProgressContext {
    HttpClient::ProgressCallback callback;
};

struct DownloadContext {
    FILE* file;
    HttpClient::ProgressCallback progressCallback;
};

size_t HttpClient::writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    WriteContext* context = static_cast<WriteContext*>(userdata);
    size_t totalSize = size * nmemb;
    context->data->append(ptr, totalSize);
    return totalSize;
}

size_t HttpClient::headerCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
    HeaderContext* context = static_cast<HeaderContext*>(userdata);
    std::string header(buffer, size * nitems);

    size_t colonPos = header.find(':');
    if (colonPos != std::string::npos) {
        std::string key = header.substr(0, colonPos);
        std::string value = header.substr(colonPos + 1);

        // Trim whitespace
        size_t start = value.find_first_not_of(" \t");
        size_t end = value.find_last_not_of(" \t\r\n");
        if (start != std::string::npos && end != std::string::npos) {
            value = value.substr(start, end - start + 1);
        }

        (*context->headers)[key] = value;
    }

    return size * nitems;
}

int HttpClient::progressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                                  curl_off_t ultotal, curl_off_t ulnow) {
    ProgressContext* context = static_cast<ProgressContext*>(clientp);
    if (context->callback) {
        context->callback(dlnow, dltotal);
    }
    return 0;
}

HttpClient::HttpClient() {
    curl_ = curl_easy_init();
}

HttpClient::~HttpClient() {
    if (curl_) {
        curl_easy_cleanup(curl_);
    }
}

HttpResponse HttpClient::get(const std::string& url) {
    return get(url, {});
}

HttpResponse HttpClient::get(const std::string& url,
                              const std::unordered_map<std::string, std::string>& headers) {
    HttpRequest request;
    request.url = url;
    request.method = "GET";
    request.headers = headers;
    return this->request(request);
}

HttpResponse HttpClient::post(const std::string& url, const std::string& body,
                               const std::string& contentType) {
    HttpRequest request;
    request.url = url;
    request.method = "POST";
    request.body = body;
    request.headers["Content-Type"] = contentType;
    return this->request(request);
}

HttpResponse HttpClient::request(const HttpRequest& req) {
    std::lock_guard<std::mutex> lock(mutex_);

    HttpResponse response;

    if (!curl_) {
        response.error = "CURL not initialized";
        return response;
    }

    curl_easy_reset(curl_);

    // Set URL
    curl_easy_setopt(curl_, CURLOPT_URL, req.url.c_str());

    // Set method
    if (req.method == "POST") {
        curl_easy_setopt(curl_, CURLOPT_POST, 1L);
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, req.body.c_str());
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, req.body.size());
    } else if (req.method == "PUT") {
        curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, req.body.c_str());
    } else if (req.method == "DELETE") {
        curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "DELETE");
    }

    // Set headers
    struct curl_slist* headerList = nullptr;
    for (const auto& [key, value] : req.headers) {
        std::string header = key + ": " + value;
        headerList = curl_slist_append(headerList, header.c_str());
    }

    // Add user agent
    std::string uaHeader = "User-Agent: " + userAgent_;
    headerList = curl_slist_append(headerList, uaHeader.c_str());

    if (headerList) {
        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headerList);
    }

    // Response body callback
    WriteContext writeContext{&response.body};
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &writeContext);

    // Response headers callback
    HeaderContext headerContext{&response.headers};
    curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &headerContext);

    // Timeout
    int timeout = req.timeoutSeconds > 0 ? req.timeoutSeconds : timeout_;
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT, static_cast<long>(timeout));

    // Follow redirects
    if (req.followRedirects) {
        curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl_, CURLOPT_MAXREDIRS, static_cast<long>(req.maxRedirects));
    }

    // SSL verification
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 2L);

    // Proxy
    if (!proxy_.empty()) {
        curl_easy_setopt(curl_, CURLOPT_PROXY, proxy_.c_str());
    }

    // Perform request
    CURLcode res = curl_easy_perform(curl_);

    if (headerList) {
        curl_slist_free_all(headerList);
    }

    if (res != CURLE_OK) {
        response.error = curl_easy_strerror(res);
        return response;
    }

    // Get status code
    long httpCode = 0;
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &httpCode);
    response.statusCode = static_cast<int>(httpCode);

    return response;
}

Result<void> HttpClient::downloadFile(const std::string& url, const Path& destination,
                                       ProgressCallback progress) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!curl_) {
        return Result<void>("CURL not initialized");
    }

    FILE* file = fopen(destination.c_str(), "wb");
    if (!file) {
        return Result<void>("Failed to open file for writing: " + destination.string());
    }

    curl_easy_reset(curl_);
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, fwrite);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT, static_cast<long>(timeout_));

    std::string ua = "User-Agent: " + userAgent_;
    struct curl_slist* headers = curl_slist_append(nullptr, ua.c_str());
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);

    if (progress) {
        ProgressContext ctx{progress};
        curl_easy_setopt(curl_, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl_, CURLOPT_XFERINFOFUNCTION, progressCallback);
        curl_easy_setopt(curl_, CURLOPT_XFERINFODATA, &ctx);
    }

    CURLcode res = curl_easy_perform(curl_);

    fclose(file);
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        std::filesystem::remove(destination);
        return Result<void>(std::string("Download failed: ") + curl_easy_strerror(res));
    }

    return Result<void>();
}

Result<void> HttpClient::stream(const std::string& url, DataCallback callback) {
    // Streaming implementation would go here
    return Result<void>("Streaming not yet implemented");
}

void HttpClient::setUserAgent(const std::string& userAgent) {
    userAgent_ = userAgent;
}

void HttpClient::setTimeout(int seconds) {
    timeout_ = seconds;
}

void HttpClient::setProxy(const std::string& proxy) {
    proxy_ = proxy;
}

std::string HttpClient::urlEncode(const std::string& str) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return str;
    }

    char* encoded = curl_easy_escape(curl, str.c_str(), str.length());
    std::string result = encoded ? encoded : str;

    if (encoded) {
        curl_free(encoded);
    }
    curl_easy_cleanup(curl);

    return result;
}

std::string HttpClient::urlDecode(const std::string& str) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return str;
    }

    int decodedLen = 0;
    char* decoded = curl_easy_unescape(curl, str.c_str(), str.length(), &decodedLen);
    std::string result = decoded ? std::string(decoded, decodedLen) : str;

    if (decoded) {
        curl_free(decoded);
    }
    curl_easy_cleanup(curl);

    return result;
}

std::string HttpClient::buildQueryString(const std::unordered_map<std::string, std::string>& params) {
    std::stringstream ss;
    bool first = true;

    for (const auto& [key, value] : params) {
        if (!first) ss << "&";
        ss << urlEncode(key) << "=" << urlEncode(value);
        first = false;
    }

    return ss.str();
}

} // namespace soda
