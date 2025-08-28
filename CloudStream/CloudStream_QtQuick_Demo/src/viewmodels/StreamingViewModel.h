#pragma once

#include <QObject>
#include <QStringList>
#include <QEvent>
#include <QKeyEvent>
#include <QJsonObject>
#include <QJsonDocument>
#include "core/video/Frame.h"
#include "core/video/VideoRenderItem.h"
#include "tcr_c_api.h"

// 前置声明
class ApiService;

/**
 * @brief 串流业务核心 ViewModel，演示了 TcrSdk 的完整使用流程。
 *
 * 使用步骤（典型流程）：
 * 1. 创建会话：tcr_client_create_session(client)
 * 2. 连接实例：tcr_session_access(session, instanceIds, count, isGroupControl)
 * 3. 设置回调：
 *    - 视频帧：tcr_session_set_video_frame_observer(...)
 *    - 会话事件：tcr_session_set_observer(...)
 * 4. 云机交互：键盘/触摸/摄像头/麦克风等
 * 5. 释放资源：tcr_client_destroy_session(...)
 *
 * 本类已把上述步骤封装成 connectSession / connectGroupSession / closeSession
 * 等 Q_INVOKABLE 函数，QML 侧可直接调用。
 */
class StreamingViewModel : public QObject
{
    Q_OBJECT
public:
    explicit StreamingViewModel(QObject *parent = nullptr);
    ~StreamingViewModel() override;

    /**
     * @brief 将 QML 侧的 VideoRenderItem 传入 C++，用于渲染视频帧。
     * @param item 已在 QML 中实例化的 VideoRenderItem 指针
     *
     * 内部会把 newVideoFrame 信号与 VideoRenderItem::setFrame 槽函数
     * 通过 Qt::QueuedConnection 连接，确保线程安全。
     */
    void setVideoRenderItem(VideoRenderItem* item);

    /**
     * @brief QML 侧调用的重载版本，自动做类型转换。
     */
    Q_INVOKABLE void setVideoRenderItem(QObject* item);

    /**
     * @brief 主动关闭当前会话，释放 TcrSdk 资源。
     *
     * 应在窗口关闭或切换页面时调用，防止内存泄漏。
     */
    Q_INVOKABLE void closeSession();

    /**
     * @brief 群控场景下动态更新需要同步的实例列表。
     * @param instanceIds 勾选的实例 ID 列表
     *
     * 内部会调用 tcr_instance_join_group(...) 把实例加入群控组。
     */
    Q_INVOKABLE void updateCheckedInstanceIds(const QVariantList& instanceIds);

signals:
    /**
     * @brief 每当 TcrSdk 产生新的解码帧时触发。
     * @param frame 封装好的 YUV(I420) 数据，可直接送入渲染管线。
     */
    void newVideoFrame(VideoFrameDataPtr frame);

private:
    VideoRenderItem* m_videoRenderItem = nullptr;
    TcrClientHandle  m_tcrClient  = nullptr;        ///< TcrSdk 客户端句柄
    TcrAndroidInstance m_instance = nullptr;        ///< Android 实例操作句柄
    TcrSessionHandle   m_session  = nullptr;        ///< 当前会话句柄
    TcrSessionObserver m_sessionObserver = {};      ///< 会话事件回调结构体
    TcrVideoFrameObserver m_videoFrameObserver = {};///< 视频帧回调结构体
    ApiService* m_apiService = nullptr;
    QStringList m_groupInstanceIds; ///< 群控时缓存的实例 ID
    bool m_sessionConnected = false;///< 会话是否已建立

    /**
     * @brief 创建并初始化会话
     *
     * 会先 closeSession() 清理旧会话，再：
     * - tcr_client_create_session
     * - tcr_session_set_observer
     * - tcr_session_set_video_frame_observer
     */
    void createAndInitSession();

    /**
     * @brief 给当前会话注册 observer。
     */
    void setSessionObservers();

    /**
     * @brief 会话事件回调（C 静态函数）。
     *
     * SDK 会在以下时机回调：
     * - 连接成功：TCR_SESSION_EVENT_STATE_CONNECTED
     * - 连接断开：TCR_SESSION_EVENT_STATE_CLOSED
     * - 摄像头状态：TCR_SESSION_EVENT_CAMERA_STATUS
     * 内部通过 QMetaObject::invokeMethod 切换到主线程处理。
     */
    static void SessionEventCallback(void* user_data,
                                     TcrSessionEvent event,
                                     const char* eventData);

    /**
     * @brief 视频帧回调（C 静态函数）。
     *
     * SDK 每产生一帧解码数据即调用，内部把裸数据封装成 VideoFrameDataPtr
     * 后通过 newVideoFrame 信号抛到主线程。
     */
    static void VideoFrameCallback(void* user_data,
                                   const TcrVideoFrameBuffer* frame_buffer,
                                   int64_t timestamp_us,
                                   TcrVideoRotation rotation);

    /** 
     * @brief 请求改变推流状态
     * @param paused 是否暂停推流
     */
    void requestStreaming(bool paused);

public slots:
    /**
     * @brief 发送触摸事件到云端实例。
     * @param x,y,width,height 相对坐标及分辨率
     * @param eventType 0=按下 1=移动 2=抬起
     * @param timestamp 毫秒时间戳
     *
     * 内部直接调用 tcr_session_touchscreen_touch(...)
     */
    void handleMouseEvent(int x, int y, int width, int height,
                          int eventType, qint64 timestamp);

    /**
     * @brief 单实例连接入口（QML 调用）。
     * @param instanceId 目标实例 ID
     *
     * 内部流程：
     * 1. createAndInitSession()
     * 2. tcr_session_access(session, &idPtr, 1, false)
     */
    void connectSession(const QString& instanceId);

    /**
     * @brief 多实例(群控)连接入口。
     * @param instanceIds 实例 ID 列表
     *
     * 内部流程：
     * 1. createAndInitSession()
     * 2. tcr_session_access(session, ids, count, true)
     * 3. 连接成功后把第一个实例设为主控
     */
    void connectGroupSession(const QVariantList& instanceIds);

    /* 以下触发云机按键事件，内部通过 tcr_session_send_keyboard_event 实现 */
    void onBackClicked();
    void onHomeClicked();
    void onMenuClicked();
    void onVolumUp();
    void onVolumDown();

    /**
     * @brief 暂停/恢复远端推流，对应 SDK：
     * tcr_session_pause_streaming / resume_streaming
     */
    void onPauseStreamClicked();
    void onResumeStreamClicked();
};
