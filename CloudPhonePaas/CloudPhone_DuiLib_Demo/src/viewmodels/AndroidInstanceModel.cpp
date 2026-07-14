#include "AndroidInstanceModel.h"

#include "core/BatchTaskOperator.h"
#include "tcr_c_api.h"
#include "utils/InstanceImageDownloader.h"
#include "utils/Logger.h"

AndroidInstanceModel::AndroidInstanceModel(ApiService* apiService,
                                           BatchTaskOperator* batchOperator,
                                           NetworkService* networkService)
    : m_apiService(apiService), m_batchOperator(batchOperator), m_networkService(networkService) {
    Logger::info("AndroidInstanceModel initialized");
}

void AndroidInstanceModel::onLoginSuccess(const std::string& /*userType*/) {
    // Fetch all instances belonging to this user (no ID filter)
    m_apiService->describeAndroidInstances(
        0, 200, {}, "",
        [this](bool success, const std::vector<AndroidInstance>& instances, int totalCount,
               const std::string& error) {
            onInstancesReceived(success, instances, totalCount, error);
        });
}

void AndroidInstanceModel::refreshInstances() {
    Logger::info("Refreshing instance list");
    m_instances.clear();
    if (onInstancesChanged)
        onInstancesChanged();

    m_apiService->describeAndroidInstances(
        0, 200, {}, "",
        [this](bool success, const std::vector<AndroidInstance>& instances, int totalCount,
               const std::string& error) {
            onInstancesReceived(success, instances, totalCount, error);
        });
}

void AndroidInstanceModel::onInstancesReceived(bool success,
                                               const std::vector<AndroidInstance>& instances,
                                               int totalCount, const std::string& error) {
    if (!success) {
        Logger::error("Failed to fetch instances: " + error);
        if (onError)
            onError(error);
        return;
    }

    m_instances = instances;
    Logger::info("Received " + std::to_string(instances.size()) + " instances (total: " +
                 std::to_string(totalCount) + ")");

    // Extract instance IDs and request access token
    std::vector<std::string> instanceIds;
    for (const auto& inst : m_instances)
        instanceIds.push_back(inst.AndroidInstanceId);

    m_apiService->createAndroidInstancesAccessToken(
        instanceIds,
        [this](bool success, const std::string& accessInfo, const std::string& token,
               const std::string& error) {
            if (success) {
                Logger::info("Access token received, initializing TcrSdk");

                // Initialize TcrSdk with the access token
                TcrConfig tcrConfig = {token.c_str(), accessInfo.c_str()};
                TcrClientHandle client = tcr_client_get_instance();
                TcrErrorCode result = tcr_client_init(client, &tcrConfig);
                if (result != TCR_SUCCESS) {
                    Logger::error("tcr_client_init failed, code=" + std::to_string(result));
                } else {
                    Logger::info("TcrSdk initialized successfully");
                    if (onTcrSdkReady)
                        onTcrSdkReady();

                    // Start downloading instance images now that TcrSdk is ready
                    startImageDownloading();
                }

                if (onAccessTokenReceived)
                    onAccessTokenReceived(accessInfo, token);
            } else {
                Logger::error("Failed to get access token: " + error);
            }
        });

    if (onInstancesChanged)
        onInstancesChanged();
}

void AndroidInstanceModel::startImageDownloading() {
    if (!m_batchOperator)
        return;

    // Collect first 10 instance IDs
    std::vector<std::string> ids;
    size_t count = std::min(m_instances.size(), static_cast<size_t>(10));
    for (size_t i = 0; i < count; ++i) {
        ids.push_back(m_instances[i].AndroidInstanceId);
    }

    if (ids.empty())
        return;

    if (!m_imageDownloader) {
        m_imageDownloader =
            std::make_unique<InstanceImageDownloader>(m_batchOperator, m_networkService);

        m_imageDownloader->onImageDownloaded =
            [this](const std::string& instanceId, const std::vector<uint8_t>& imageData) {
                m_imageCache.put(instanceId, imageData);
                Logger::debug("Image cached for " + instanceId + " (" +
                              std::to_string(imageData.size()) + " bytes)");
                if (onImageDownloaded)
                    onImageDownloaded(instanceId, imageData);
            };
    }

    m_imageDownloader->startDownloading(ids);
    Logger::info("Started image downloading for " + std::to_string(ids.size()) + " instances");
}
