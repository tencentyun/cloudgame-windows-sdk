#include "ApiService.h"

#include <thread>

#ifdef _WIN32
#include <objbase.h>  // CoCreateGuid
#endif

#include "utils/Logger.h"

ApiService::ApiService(NetworkService* networkService) : m_networkService(networkService) {
    Logger::info("API service initialized");
}

std::string ApiService::generateRequestId() {
#ifdef _WIN32
    GUID guid;
    CoCreateGuid(&guid);
    char buf[64] = {};
    snprintf(buf, sizeof(buf), "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", guid.Data1, guid.Data2,
             guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4],
             guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    return std::string("{") + buf + "}";
#else
    return "{00000000-0000-0000-0000-000000000000}";
#endif
}

ApiService::ApiResult ApiService::processResponse(const HttpResponse& httpResponse) {
    ApiResult result;

    if (!httpResponse.success) {
        result.errorCode = "NetworkError";
        result.errorMessage = httpResponse.error.empty()
                                  ? ("HTTP " + std::to_string(httpResponse.statusCode))
                                  : httpResponse.error;
        Logger::error("Network error: " + result.errorMessage);
        return result;
    }

    Json::CharReaderBuilder reader;
    Json::Value root;
    std::string errors;
    std::istringstream stream(httpResponse.body);

    if (!Json::parseFromStream(reader, stream, &root, &errors)) {
        result.errorCode = "InvalidResponse";
        result.errorMessage = "Invalid JSON format";
        Logger::error("Invalid JSON response: " + errors);
        return result;
    }

    // Check API-level error
    if (root.isMember("Error")) {
        Json::Value error = root["Error"];
        std::string requestId = root.get("RequestId", "").asString();
        result.errorCode = error.get("Code", "").asString();
        result.errorMessage = error.get("Message", "").asString();
        Logger::error("API error: code=" + result.errorCode + ", message=" + result.errorMessage +
                      ", requestId=" + requestId);
        return result;
    }

    result.success = true;
    result.response = root["Response"];
    return result;
}

std::vector<AndroidInstance> ApiService::parseAndroidInstances(const Json::Value& instances) {
    std::vector<AndroidInstance> result;
    for (const auto& obj : instances) {
        AndroidInstance inst;
        inst.AndroidInstanceId = obj.get("AndroidInstanceId", "").asString();
        inst.AndroidInstanceRegion = obj.get("AndroidInstanceRegion", "").asString();
        inst.Name = obj.get("Name", "").asString();
        inst.State = obj.get("State", "").asString();
        inst.UserId = obj.get("UserId", "").asString();
        result.push_back(inst);
    }
    return result;
}

// Extract authorization cookie from raw response headers
static std::string extractAuthCookie(const std::string& headers) {
    // Look for Set-Cookie header containing "authorization="
    std::istringstream stream(headers);
    std::string line;
    while (std::getline(stream, line)) {
        // Remove trailing \r
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        // Case-insensitive check for Set-Cookie
        if (line.size() > 12) {
            std::string prefix = line.substr(0, 12);
            // Convert to lowercase for comparison
            for (auto& c : prefix)
                c = static_cast<char>(tolower(c));
            if (prefix == "set-cookie: ") {
                std::string cookiePart = line.substr(12);
                // Find "authorization=" in the cookie
                size_t pos = cookiePart.find("authorization=");
                if (pos != std::string::npos) {
                    std::string value = cookiePart.substr(pos + 14);  // len("authorization=") = 14
                    // Token ends at ';' or end of string
                    size_t end = value.find(';');
                    if (end != std::string::npos)
                        value = value.substr(0, end);
                    return value;
                }
            }
        }
    }
    return {};
}

void ApiService::login(const std::string& userId, const std::string& password, LoginCallback callback) {
    Json::Value data;
    data["RequestId"] = generateRequestId();
    data["UserId"] = userId;
    data["Password"] = password;

    Logger::info("API request sent: /Login");

    std::thread([this, data, callback]() {
        HttpResponse httpResp = m_networkService->postRequest("/Login", data);

        // Extract authorization token from response headers
        if (httpResp.success) {
            std::string token = extractAuthCookie(httpResp.headers);
            if (!token.empty()) {
                m_networkService->setToken(token);
                Logger::info("Authorization token obtained");
            } else {
                Logger::warning("Login response did not contain an authorization token in Set-Cookie header");
                Logger::debug("Response headers: " + httpResp.headers);
            }
        }

        ApiResult result = processResponse(httpResp);

        if (result.success) {
            std::string userType = result.response.get("UserType", "").asString();
            Logger::info("Login success, user type: " + userType);
            callback(true, userType);
        } else {
            callback(false, result.errorMessage);
        }
    }).detach();
}

void ApiService::describeAndroidInstances(int offset, int limit,
                                          const std::vector<std::string>& instanceIds,
                                          const std::string& region, InstancesCallback callback) {
    Json::Value data;
    data["Offset"] = offset;
    data["Limit"] = limit;

    if (!instanceIds.empty()) {
        Json::Value idsArray(Json::arrayValue);
        for (const auto& id : instanceIds)
            idsArray.append(id);
        data["AndroidInstanceIds"] = idsArray;
    }
    if (!region.empty()) {
        data["AndroidInstanceRegion"] = region;
    }

    Logger::info("API request sent: /DescribeAndroidInstances");

    std::thread([this, data, callback]() {
        HttpResponse httpResp = m_networkService->postRequest("/DescribeAndroidInstances", data);
        ApiResult result = processResponse(httpResp);

        if (result.success) {
            int totalCount = result.response.get("TotalCount", 0).asInt();
            auto instances = parseAndroidInstances(result.response["AndroidInstances"]);
            callback(true, instances, totalCount, "");
        } else {
            callback(false, {}, 0, result.errorMessage);
        }
    }).detach();
}

void ApiService::connectAndroidInstance(const std::string& instanceId, const std::string& clientSession,
                                        ConnectCallback callback) {
    Json::Value data;
    data["AndroidInstanceId"] = instanceId;
    data["ClientSession"] = clientSession;

    Logger::info("API request sent: /ConnectAndroidInstance");

    std::thread([this, data, callback]() {
        HttpResponse httpResp = m_networkService->postRequest("/ConnectAndroidInstance", data);
        ApiResult result = processResponse(httpResp);

        if (result.success) {
            std::string serverSession = result.response.get("ServerSession", "").asString();
            callback(true, serverSession);
        } else {
            callback(false, result.errorMessage);
        }
    }).detach();
}

void ApiService::connectAndroidGroupInstances(const std::vector<std::string>& instanceIds,
                                              const std::vector<std::string>& clientSessions,
                                              GroupConnectCallback callback) {
    Json::Value data;
    Json::Value idsArray(Json::arrayValue);
    Json::Value sessionsArray(Json::arrayValue);
    for (const auto& id : instanceIds)
        idsArray.append(id);
    for (const auto& session : clientSessions)
        sessionsArray.append(session);
    data["AndroidInstanceIds"] = idsArray;
    data["ClientSessions"] = sessionsArray;

    Logger::info("API request sent: /ConnectAndroidGroupInstances");

    std::thread([this, data, callback]() {
        HttpResponse httpResp = m_networkService->postRequest("/ConnectAndroidGroupInstances", data);
        ApiResult result = processResponse(httpResp);

        if (result.success) {
            std::vector<std::string> serverSessions;
            for (const auto& session : result.response["ServerSessions"])
                serverSessions.push_back(session.asString());
            callback(true, serverSessions, "");
        } else {
            callback(false, {}, result.errorMessage);
        }
    }).detach();
}

void ApiService::createAndroidInstancesAccessToken(const std::vector<std::string>& instanceIds,
                                                   AccessTokenCallback callback) {
    Json::Value data;
    data["RequestId"] = generateRequestId();
    Json::Value idsArray(Json::arrayValue);
    for (const auto& id : instanceIds)
        idsArray.append(id);
    data["AndroidInstanceIds"] = idsArray;
    data["ExpirationDuration"] = "24h";
    data["Mode"] = "STANDARD";

    Logger::info("API request sent: /CreateAndroidInstancesAccessToken");

    std::thread([this, data, callback]() {
        HttpResponse httpResp = m_networkService->postRequest("/CreateAndroidInstancesAccessToken", data);
        ApiResult result = processResponse(httpResp);

        if (result.success) {
            std::string accessInfo = result.response.get("AccessInfo", "").asString();
            std::string token = result.response.get("Token", "").asString();
            callback(true, accessInfo, token, "");
        } else {
            callback(false, "", "", result.errorMessage);
        }
    }).detach();
}
