#include "InstanceImageDownloader.h"

#include <chrono>

#include <curl/curl.h>

#include "core/BatchTaskOperator.h"
#include "utils/Logger.h"

// curl write callback for downloading image bytes
static size_t imageWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* buf = reinterpret_cast<std::vector<uint8_t>*>(userdata);
    size_t total = size * nmemb;
    buf->insert(buf->end(), reinterpret_cast<uint8_t*>(ptr),
                reinterpret_cast<uint8_t*>(ptr) + total);
    return total;
}

InstanceImageDownloader::InstanceImageDownloader(BatchTaskOperator* op,
                                                 NetworkService* networkService)
    : m_operator(op), m_networkService(networkService) {}

InstanceImageDownloader::~InstanceImageDownloader() {
    stopDownloading();
}

void InstanceImageDownloader::startDownloading(const std::vector<std::string>& instanceIds) {
    stopDownloading();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pendingIds = instanceIds;
    }

    m_running.store(true);
    m_paused.store(false);
    m_downloadThread = std::thread(&InstanceImageDownloader::downloadLoop, this);
}

void InstanceImageDownloader::stopDownloading() {
    m_running.store(false);
    m_paused.store(false);
    m_cv.notify_all();

    if (m_downloadThread.joinable()) {
        m_downloadThread.join();
    }
}

void InstanceImageDownloader::pauseDownloading() {
    m_paused.store(true);
}

void InstanceImageDownloader::resumeDownloading() {
    m_paused.store(false);
    m_cv.notify_all();
}

void InstanceImageDownloader::updateInstanceList(const std::vector<std::string>& instanceIds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pendingIds = instanceIds;
    m_cv.notify_all();
}

void InstanceImageDownloader::downloadLoop() {
    Logger::info("[InstanceImageDownloader] download loop started");

    while (m_running.load()) {
        // Wait if paused
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]() { return !m_paused.load() || !m_running.load(); });
        }

        if (!m_running.load())
            break;

        // Get current pending IDs
        std::vector<std::string> ids;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            ids = m_pendingIds;
        }

        for (const auto& instanceId : ids) {
            if (!m_running.load())
                break;

            // Get screenshot URL via BatchTaskOperator
            std::string url = m_operator->getInstanceImage(instanceId);
            if (url.empty()) {
                Logger::debug("[InstanceImageDownloader] no URL for " + instanceId);
                continue;
            }

            // Download image bytes
            std::vector<uint8_t> imageData = downloadImageFromUrl(url);
            if (imageData.empty()) {
                Logger::debug("[InstanceImageDownloader] download failed for " + instanceId);
                continue;
            }

            // Invoke callback
            if (onImageDownloaded) {
                onImageDownloaded(instanceId, imageData);
            }

            // Brief sleep between downloads to avoid overwhelming
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Sleep between full rounds
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait_for(lock, std::chrono::seconds(3),
                          [this]() { return !m_running.load(); });
        }
    }

    Logger::info("[InstanceImageDownloader] download loop stopped");
}

std::vector<uint8_t> InstanceImageDownloader::downloadImageFromUrl(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl)
        return {};

    std::vector<uint8_t> buf;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, imageWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        Logger::warning("[InstanceImageDownloader] curl error: " + std::string(curl_easy_strerror(res)));
        return {};
    }

    return buf;
}
