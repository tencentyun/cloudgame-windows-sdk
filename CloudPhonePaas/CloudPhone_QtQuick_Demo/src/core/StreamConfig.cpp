#include "StreamConfig.h"

StreamConfig* StreamConfig::s_instance = nullptr;
QMutex StreamConfig::s_mutex;

StreamConfig::StreamConfig(QObject *parent)
    : QObject(parent)
{
}

StreamConfig* StreamConfig::instance()
{
    if (!s_instance) {
        QMutexLocker locker(&s_mutex);
        if (!s_instance) {
            s_instance = new StreamConfig();
        }
    }
    return s_instance;
}

// Main stream setters
void StreamConfig::setMainStreamWidth(int width)
{
    if (m_mainStreamWidth != width) {
        m_mainStreamWidth = width;
        emit mainStreamWidthChanged();
    }
}

void StreamConfig::setMainStreamFps(int fps)
{
    if (m_mainStreamFps != fps) {
        m_mainStreamFps = fps;
        emit mainStreamFpsChanged();
    }
}

void StreamConfig::setMainStreamMinBitrate(int bitrate)
{
    if (m_mainStreamMinBitrate != bitrate) {
        m_mainStreamMinBitrate = bitrate;
        emit mainStreamMinBitrateChanged();
    }
}

void StreamConfig::setMainStreamMaxBitrate(int bitrate)
{
    if (m_mainStreamMaxBitrate != bitrate) {
        m_mainStreamMaxBitrate = bitrate;
        emit mainStreamMaxBitrateChanged();
    }
}

// Sub stream setters
void StreamConfig::setSubStreamWidth(int width)
{
    if (m_subStreamWidth != width) {
        m_subStreamWidth = width;
        emit subStreamWidthChanged();
    }
}

void StreamConfig::setSubStreamFps(int fps)
{
    if (m_subStreamFps != fps) {
        m_subStreamFps = fps;
        emit subStreamFpsChanged();
    }
}

void StreamConfig::setSubStreamMinBitrate(int bitrate)
{
    if (m_subStreamMinBitrate != bitrate) {
        m_subStreamMinBitrate = bitrate;
        emit subStreamMinBitrateChanged();
    }
}

void StreamConfig::setSubStreamMaxBitrate(int bitrate)
{
    if (m_subStreamMaxBitrate != bitrate) {
        m_subStreamMaxBitrate = bitrate;
        emit subStreamMaxBitrateChanged();
    }
}