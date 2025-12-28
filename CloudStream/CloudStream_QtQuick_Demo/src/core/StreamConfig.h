#pragma once

#include <QObject>
#include <QMutex>
#include <QMutexLocker>

class StreamConfig : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int mainStreamWidth READ mainStreamWidth WRITE setMainStreamWidth NOTIFY mainStreamWidthChanged)
    Q_PROPERTY(int mainStreamFps READ mainStreamFps WRITE setMainStreamFps NOTIFY mainStreamFpsChanged)
    Q_PROPERTY(int mainStreamMinBitrate READ mainStreamMinBitrate WRITE setMainStreamMinBitrate NOTIFY mainStreamMinBitrateChanged)
    Q_PROPERTY(int mainStreamMaxBitrate READ mainStreamMaxBitrate WRITE setMainStreamMaxBitrate NOTIFY mainStreamMaxBitrateChanged)
    
    Q_PROPERTY(int subStreamWidth READ subStreamWidth WRITE setSubStreamWidth NOTIFY subStreamWidthChanged)
    Q_PROPERTY(int subStreamFps READ subStreamFps WRITE setSubStreamFps NOTIFY subStreamFpsChanged)
    Q_PROPERTY(int subStreamMinBitrate READ subStreamMinBitrate WRITE setSubStreamMinBitrate NOTIFY subStreamMinBitrateChanged)
    Q_PROPERTY(int subStreamMaxBitrate READ subStreamMaxBitrate WRITE setSubStreamMaxBitrate NOTIFY subStreamMaxBitrateChanged)

public:
    // Get singleton instance
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

signals:
    void mainStreamWidthChanged();
    void mainStreamFpsChanged();
    void mainStreamMinBitrateChanged();
    void mainStreamMaxBitrateChanged();
    
    void subStreamWidthChanged();
    void subStreamFpsChanged();
    void subStreamMinBitrateChanged();
    void subStreamMaxBitrateChanged();

private:
    explicit StreamConfig(QObject *parent = nullptr);
    ~StreamConfig() override = default;
    
    // Disable copy
    StreamConfig(const StreamConfig&) = delete;
    StreamConfig& operator=(const StreamConfig&) = delete;
    
    static StreamConfig* s_instance;
    static QMutex s_mutex;
    
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