#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "services/ApiService.h"
#include "utils/ImageCache.h"
#include "utils/InstanceImageDownloader.h"

class BatchTaskOperator;
class NetworkService;

/**
 * @brief Manages the cloud phone instance list.
 *
 * After login, fetches instances and access token.
 * Notifies the UI when instance data changes.
 */
class AndroidInstanceModel {
public:
    AndroidInstanceModel(ApiService* apiService, BatchTaskOperator* batchOperator,
                         NetworkService* networkService);

    const std::vector<AndroidInstance>& instances() const { return m_instances; }

    /// Call after login succeeds to fetch instance list
    void onLoginSuccess(const std::string& userType);

    /// Refresh the instance list
    void refreshInstances();

    // Callbacks (invoked on worker thread — caller must post to UI thread)
    std::function<void()> onInstancesChanged;
    std::function<void(const std::string& accessInfo, const std::string& token)> onAccessTokenReceived;
    std::function<void()> onTcrSdkReady;  ///< Fired after tcr_client_init succeeds
    std::function<void(const std::string& error)> onError;

    /// Callback: image downloaded for an instance (invoked on download thread)
    std::function<void(const std::string& instanceId, const std::vector<uint8_t>& imageData)>
        onImageDownloaded;

    /// Access the image cache
    ImageCache& imageCache() { return m_imageCache; }

private:
    ApiService* m_apiService;
    BatchTaskOperator* m_batchOperator;
    NetworkService* m_networkService;
    std::vector<AndroidInstance> m_instances;
    std::unique_ptr<InstanceImageDownloader> m_imageDownloader;
    ImageCache m_imageCache;

    void onInstancesReceived(bool success, const std::vector<AndroidInstance>& instances,
                             int totalCount, const std::string& error);
    void startImageDownloading();
};
