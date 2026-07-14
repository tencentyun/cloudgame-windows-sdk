#include "NetworkService.h"

#include <curl/curl.h>

#include "utils/Logger.h"

NetworkService::NetworkService() {
    // Default to test environment
    setEnvironment(false);
}

NetworkService::~NetworkService() = default;

void NetworkService::setEnvironment(bool isProduction) {
    m_baseUrl = isProduction ? "https://cai-server.cloud-device.crtrcloud.com/external"
                             : "https://test-cai-experience-server.crtrcloud.com/external";
}

void NetworkService::setToken(const std::string& token) { m_token = token; }
std::string NetworkService::getToken() const { return m_token; }
void NetworkService::setRequestHost(const std::string& requestHost) { m_requestHost = requestHost; }
void NetworkService::setOrigin(const std::string& origin) { m_origin = origin; }

size_t NetworkService::writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* response = static_cast<std::string*>(userdata);
    size_t totalSize = size * nmemb;
    response->append(ptr, totalSize);
    return totalSize;
}

size_t NetworkService::headerCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* headers = static_cast<std::string*>(userdata);
    size_t totalSize = size * nmemb;
    headers->append(ptr, totalSize);
    return totalSize;
}

HttpResponse NetworkService::postRequest(const std::string& endpoint, const Json::Value& data) {
    HttpResponse response;

    std::string url = m_baseUrl + endpoint;
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string jsonBody = Json::writeString(writer, data);

    Logger::debug("Request URL: " + url);
    Logger::debug("Request data: " + jsonBody);

    CURL* curl = curl_easy_init();
    if (!curl) {
        response.error = "Failed to initialize CURL";
        Logger::error(response.error);
        return response;
    }

    std::string responseBody;
    std::string responseHeaders;

    // Set URL
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    // POST method with JSON body
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(jsonBody.size()));

    // Set headers
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    if (!m_token.empty()) {
        // Send as both Authorization Bearer AND Cookie, to match browser behavior
        std::string authHeader = "Authorization: Bearer " + m_token;
        headers = curl_slist_append(headers, authHeader.c_str());
        std::string cookieHeader = "Cookie: authorization=" + m_token;
        headers = curl_slist_append(headers, cookieHeader.c_str());
        std::string tokenPreview = m_token.size() > 30
                                       ? m_token.substr(0, 30) + "..."
                                       : m_token;
        Logger::debug("Sending Authorization token (preview): " + tokenPreview);
    } else {
        Logger::warning("No token — request sent without Authorization header: " + endpoint);
    }
    if (!m_requestHost.empty()) {
        std::string hostHeader = "Request-Host: " + m_requestHost;
        headers = curl_slist_append(headers, hostHeader.c_str());
    }
    if (!m_origin.empty()) {
        std::string originHeader = "Origin: " + m_origin;
        headers = curl_slist_append(headers, originHeader.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Response callbacks
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &responseHeaders);

    // SSL settings
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    // Timeout
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

    // Perform request
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        response.error = curl_easy_strerror(res);
        Logger::error("Network error: " + response.error);
    } else {
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        response.statusCode = static_cast<int>(httpCode);
        response.body = responseBody;
        response.headers = responseHeaders;
        response.success = (httpCode >= 200 && httpCode < 300);
        Logger::debug("API response (" + std::to_string(httpCode) + "): " + responseBody);
        // Log full response headers to help debug Set-Cookie / auth issues
        Logger::debug("Response headers:\n" + responseHeaders);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return response;
}
