#pragma once

#include <string>

#include <json/json.h>

/**
 * @brief HTTP response structure returned by NetworkService.
 */
struct HttpResponse {
    int statusCode = 0;
    std::string body;
    std::string headers;  // Raw response headers (for extracting Set-Cookie etc.)
    std::string error;
    bool success = false;
};

/**
 * @brief HTTP network service using libcurl.
 *
 * Provides synchronous POST requests with JSON body.
 * Designed to be called from worker threads (ApiService handles threading).
 * Thread-safe: each call creates its own CURL easy handle.
 */
class NetworkService {
public:
    NetworkService();
    ~NetworkService();

    /**
     * @brief Send a synchronous HTTP POST request.
     * @param endpoint API endpoint path (e.g. "/Login")
     * @param data JSON request body
     * @return HttpResponse with status code, body, headers, and error info
     */
    HttpResponse postRequest(const std::string& endpoint, const Json::Value& data);

    void setToken(const std::string& token);
    std::string getToken() const;
    void setEnvironment(bool isProduction);
    void setRequestHost(const std::string& requestHost);
    void setOrigin(const std::string& origin);

private:
    std::string m_baseUrl;
    std::string m_token;
    std::string m_requestHost;
    std::string m_origin;

    // libcurl write callback
    static size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
    // libcurl header callback
    static size_t headerCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
};
