#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief Simple thread-safe in-memory cache mapping instanceId -> raw image bytes.
 *
 * Header-only implementation.
 */
class ImageCache {
public:
    ImageCache() = default;

    /// Store image data for an instance (replaces any existing entry)
    void put(const std::string& instanceId, const std::vector<uint8_t>& imageData) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cache[instanceId] = imageData;
    }

    /// Store image data (move version)
    void put(const std::string& instanceId, std::vector<uint8_t>&& imageData) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cache[instanceId] = std::move(imageData);
    }

    /// Get image data for an instance. Returns empty vector if not found.
    std::vector<uint8_t> get(const std::string& instanceId) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_cache.find(instanceId);
        if (it != m_cache.end()) {
            return it->second;
        }
        return {};
    }

    /// Check if an instance has cached image data
    bool has(const std::string& instanceId) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_cache.find(instanceId) != m_cache.end();
    }

    /// Remove a specific entry
    void remove(const std::string& instanceId) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cache.erase(instanceId);
    }

    /// Clear all cached images
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cache.clear();
    }

    /// Get current cache size (number of entries)
    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_cache.size();
    }

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::vector<uint8_t>> m_cache;
};
