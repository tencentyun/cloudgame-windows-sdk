#pragma once

#include <QObject>
#include <QStringList>
#include <QEvent>
#include <QKeyEvent>
#include <QJsonObject>
#include <QJsonDocument>
#include "core/video/Frame.h"
#include "core/BatchTaskOperator.h"
#include "core/video/VideoRenderItem.h"

class StreamingViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString imeType READ imeType NOTIFY imeTypeChanged)

public:
    explicit StreamingViewModel(QObject *parent = nullptr);
    ~StreamingViewModel() override;

    void setTcrOperator(BatchTaskOperator* tcrOperator);
    void setVideoRenderItem(VideoRenderItem* item);

    Q_INVOKABLE void setVideoRenderItem(QObject* item);
    Q_INVOKABLE void closeSession();
    Q_INVOKABLE void updateCheckedInstanceIds(const QVariantList& instanceIds);

    // QML中输入完成后回传输入的文本
    Q_INVOKABLE void onInputTextFromQml(const QString& text);
    Q_INVOKABLE void setLocalIME();
    Q_INVOKABLE void setCloudIME();

    // imeType属性
    QString imeType() const { return m_imeType; }

    // 全局事件过滤
    bool eventFilter(QObject* obj, QEvent* event) override;

signals:
    // 新的视频帧到达
    void newVideoFrame(VideoFrameDataPtr frame);
    // 通知QML弹出输入框
    void requestShowInputBox();
    void imeTypeChanged();
    // QML输入完成后回传输入的文本
    void inputTextReceived(const QString& text);

private:
    BatchTaskOperator* m_tcrOperator = nullptr;
    VideoRenderItem* m_videoRenderItem = nullptr;
    QStringList m_groupInstanceIds;

    TcrClientHandle m_tcrClient = nullptr;
    TcrAndroidInstance m_instance = nullptr;
    TcrSessionHandle m_session = nullptr;
    TcrSessionObserver m_sessionObserver = {};
    TcrVideoFrameObserver m_videoFrameObserver = {};

    QString m_imeType = "cloud"; // 默认输入法为云端输入法(云端画面展示软键盘)

    void createAndInitSession();
    void setSessionObservers();

    static void SessionEventCallback(void* user_data, TcrSessionEvent event, const char* eventData);
    static void VideoFrameCallback(void* user_data, const TcrVideoFrame* video_frame);

public slots:
    void handleMouseEvent(int x, int y, int width, int height, int eventType, qint64 timestamp);
    void connectSession(const QString& instanceId);
    void connectGroupSession(const QVariantList& instanceIds);
    void onBackClicked();
    void onHomeClicked();
    void onMenuClicked();
    void onVolumUp();
    void onVolumDown();

    void onPauseStreamClicked();
    void onResumeStreamClicked();
    void onChangeBitrateClicked(int framerate, int minBitrate, int maxBitrate);
    void onClickPaste(const QString& inputText);
};