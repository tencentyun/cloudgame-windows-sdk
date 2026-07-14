#pragma once

#include <functional>
#include <string>
#include <vector>

#include <json/json.h>

#include "NetworkService.h"

/**
 * @brief Represents a single cloud phone instance (subset of API fields).
 */
struct AndroidInstance {
    std::string AndroidInstanceId;
    std::string AndroidInstanceRegion;
    std::string Name;
    std::string State;
    std::string UserId;
};

// Callback type definitions
using LoginCallback = std::function<void(bool success, const std::string& userTypeOrError)>;
using InstancesCallback = std::function<void(
    bool success, const std::vector<AndroidInstance>& instances, int totalCount, const std::string& error)>;
using ConnectCallback = std::function<void(bool success, const std::string& serverSessionOrError)>;
using GroupConnectCallback =
    std::function<void(bool success, const std::vector<std::string>& serverSessions, const std::string& error)>;
using AccessTokenCallback = std::function<void(
    bool success, const std::string& accessInfo, const std::string& token, const std::string& error)>;

/**
 * @brief API service class for cloud phone API interactions.
 *
 * All API methods execute asynchronously on worker threads.
 * Callbacks are invoked on the worker thread — callers must use
 * UiThreadHelper::postToUiThread() to switch back to the UI thread.
 */
class ApiService {
public:
    explicit ApiService(NetworkService* networkService);

    void login(const std::string& userId, const std::string& password, LoginCallback callback);

    void describeAndroidInstances(int offset, int limit, const std::vector<std::string>& instanceIds,
                                  const std::string& region, InstancesCallback callback);

    void connectAndroidInstance(const std::string& instanceId, const std::string& clientSession,
                                ConnectCallback callback);

    void connectAndroidGroupInstances(const std::vector<std::string>& instanceIds,
                                      const std::vector<std::string>& clientSessions,
                                      GroupConnectCallback callback);

    void createAndroidInstancesAccessToken(const std::vector<std::string>& instanceIds,
                                           AccessTokenCallback callback);

    std::vector<AndroidInstance> parseAndroidInstances(const Json::Value& instances);

private:
    NetworkService* m_networkService;

    struct ApiResult {
        bool success = false;
        Json::Value response;
        std::string errorCode;
        std::string errorMessage;
    };

    ApiResult processResponse(const HttpResponse& httpResponse);
    static std::string generateRequestId();
};
