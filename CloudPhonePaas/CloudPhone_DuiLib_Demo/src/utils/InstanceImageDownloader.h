#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class BatchTaskOperator;
class NetworkService;

/**
 * @brief Downloads instance screenshots asynchronously in a background thread.
 *
 * Uses BatchTaskOperator::getInstanceImage() to get a screenshot URL,
 * then downloads the image bytes via curl.
 */
class InstanceImageDownloader {
public:
    InstanceImageDownloader(BatchTaskOperator* op, NetworkService* networkService);
    ~InstanceImageDownloader();

    InstanceImageDownloader(const InstanceImageDownloader&) = delete;
    InstanceImageDownloader& operator=(const InstanceImageDownloader&) = delete;

    void startDownloading(const std::vector<std::string>& instanceIds);
    void stopDownloading();
    void pauseDownloading();
    void resumeDownloading();
    void updateInstanceList(const std::vector<std::string>& instanceIds);

    /// Callback: (instanceId, JPEG/PNG raw bytes)
    std::function<void(const std::string& instanceId, const std::vector<uint8_t>& imageData)>
        onImageDownloaded;

private:
    BatchTaskOperator* m_operator;
    NetworkService* m_networkService;
    std::thread m_downloadThread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_paused{false};
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::vector<std::string> m_pendingIds;

    void downloadLoop();
    std::vector<uint8_t> downloadImageFromUrl(const std::string& url);
};
