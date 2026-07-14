#pragma once

#include <mutex>

class StreamConfig {
public:
    static StreamConfig* instance();

    // Main stream getters
    int mainStreamWidth() const { return m_mainStreamWidth; }
    int mainStreamFps() const { return m_mainStreamFps; }
    int mainStreamMinBitrate() const { return m_mainStreamMinBitrate; }
    int mainStreamMaxBitrate() const { return m_mainStreamMaxBitrate; }

    // Sub stream getters
    int subStreamWidth() const { return m_subStreamWidth; }
    int subStreamFps() const { return m_subStreamFps; }
    int subStreamMinBitrate() const { return m_subStreamMinBitrate; }
    int subStreamMaxBitrate() const { return m_subStreamMaxBitrate; }

    // Main stream setters
    void setMainStreamWidth(int width);
    void setMainStreamFps(int fps);
    void setMainStreamMinBitrate(int bitrate);
    void setMainStreamMaxBitrate(int bitrate);

    // Sub stream setters
    void setSubStreamWidth(int width);
    void setSubStreamFps(int fps);
    void setSubStreamMinBitrate(int bitrate);
    void setSubStreamMaxBitrate(int bitrate);

private:
    StreamConfig() = default;
    ~StreamConfig() = default;
    StreamConfig(const StreamConfig&) = delete;
    StreamConfig& operator=(const StreamConfig&) = delete;

    static StreamConfig* s_instance;
    static std::mutex s_mutex;

    // Main stream parameters (default values)
    int m_mainStreamWidth = 720;
    int m_mainStreamFps = 30;
    int m_mainStreamMinBitrate = 300;
    int m_mainStreamMaxBitrate = 600;

    // Sub stream parameters (default values)
    int m_subStreamWidth = 288;
    int m_subStreamFps = 1;
    int m_subStreamMinBitrate = 100;
    int m_subStreamMaxBitrate = 200;
};
