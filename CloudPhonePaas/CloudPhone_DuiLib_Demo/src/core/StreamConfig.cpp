#include "StreamConfig.h"

StreamConfig* StreamConfig::s_instance = nullptr;
std::mutex StreamConfig::s_mutex;

StreamConfig* StreamConfig::instance() {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (!s_instance) {
        s_instance = new StreamConfig();
    }
    return s_instance;
}

void StreamConfig::setMainStreamWidth(int width) { m_mainStreamWidth = width; }
void StreamConfig::setMainStreamFps(int fps) { m_mainStreamFps = fps; }
void StreamConfig::setMainStreamMinBitrate(int bitrate) { m_mainStreamMinBitrate = bitrate; }
void StreamConfig::setMainStreamMaxBitrate(int bitrate) { m_mainStreamMaxBitrate = bitrate; }

void StreamConfig::setSubStreamWidth(int width) { m_subStreamWidth = width; }
void StreamConfig::setSubStreamFps(int fps) { m_subStreamFps = fps; }
void StreamConfig::setSubStreamMinBitrate(int bitrate) { m_subStreamMinBitrate = bitrate; }
void StreamConfig::setSubStreamMaxBitrate(int bitrate) { m_subStreamMaxBitrate = bitrate; }
