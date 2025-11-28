#pragma once

#include <QObject>
#include <QStringList>
#include <QEvent>
#include <QKeyEvent>
#include <QJsonObject>
#include <QJsonDocument>
#include "core/video/Frame.h"
#include "core/video/VideoRenderPaintedItem.h"
#include "tcr_c_api.h"

// 前置声明
class ApiService;

/**
 * @brief 串流业务核心 ViewModel，演示 TcrSdk 的完整使用流程
 *
 * 本类封装了云串流的完整生命周期管理，展示了如何使用 TcrSdk C API。
 *
 * ==================== SDK 使用流程 ====================
 * 
 * 【阶段1：初始化】
 *   1. 获取 TcrClient 实例：tcr_client_get_instance()
 *   2. 初始化客户端：tcr_client_init(client, config)
 *
 * 【阶段2：创建会话】
 *   3. 创建会话对象：tcr_client_create_session(client, config)
 *   4. 设置会话观察者：tcr_session_set_observer(session, observer)
 *   5. 设置视频帧观察者：tcr_session_set_video_frame_observer(session, observer)
 *
 * 【阶段3：连接实例】
 *   6. 连接云手机实例：tcr_session_access(session, instanceIds, count, isGroupControl)
 *      - 单实例模式：isGroupControl = false
 *      - 群控模式：isGroupControl = true
 *
 * 【阶段4：业务交互】
 *   7. 接收视频帧：通过 VideoFrameCallback 获取解码后的视频数据
 *   8. 发送控制指令：
 *      - 触摸事件：tcr_session_touchscreen_touch()
 *      - 键盘事件：tcr_session_send_keyboard_event()
 *      - 文本输入：tcr_session_paste_text()
 *   9. 设备控制：
 *      - 摄像头：tcr_session_enable_local_camera()
 *      - 麦克风：tcr_session_enable_local_microphone()
 *   10. 流控制：
 *      - 暂停/恢复：tcr_session_pause_streaming() / tcr_session_resume_streaming()
 *      - 调整参数：tcr_session_set_remote_video_profile()
 *
 * 【阶段5：资源释放】
 *   11. 清理观察者：tcr_session_set_observer(session, nullptr)
 *   12. 销毁会话：tcr_client_destroy_session(client, session)
 *
 * ==================== 注意事项 ====================
 * - 观察者生命周期必须覆盖整个会话周期
 * - 销毁会话前必须先取消观察者
 * - 视频帧需要手动管理引用计数
 * - 所有回调函数都在非主线程，需要使用 QMetaObject::invokeMethod 切换到主线程
 */
class StreamingViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString clientStats READ clientStats NOTIFY clientStatsChanged)

public:
    explicit StreamingViewModel(QObject *parent = nullptr);
    ~StreamingViewModel() override;

    // ==================== 视频渲染相关 ====================
    
    /**
     * @brief 设置视频渲染组件（C++ 版本）
     * @param item VideoRenderPaintedItem 指针
     * 
     * 内部会连接 newVideoFrame 信号到 VideoRenderPaintedItem::setFrame 槽
     */
    void setVideoRenderItem(VideoRenderPaintedItem* item);

    /**
     * @brief 设置视频渲染组件（QML 版本）
     * @param item QObject 指针，会自动转换为 VideoRenderPaintedItem*
     */
    Q_INVOKABLE void setVideoRenderItem(QObject* item);

    // ==================== 统计信息 ====================
    
    /**
     * @brief 获取客户端统计数据（JSON 格式）
     * @return 包含帧率、码率、延迟等信息的 JSON 字符串
     */
    QString clientStats() const { return m_clientStats; }

    // ==================== 会话管理 ====================
    
    /**
     * @brief 连接云手机实例
     * @param instanceIds 实例 ID 列表
     * @param isGroupControl 是否启用群控模式
     * 
     * 【单实例模式】instanceIds.size() == 1 && isGroupControl == false
     *   - 连接单个实例，所有操作仅作用于该实例
     * 
     * 【群控模式】instanceIds.size() >= 1 && isGroupControl == true
     *   - 连接多个实例，操作会同步到所有实例
     *   - 第一个实例默认设为主控实例
     */
    Q_INVOKABLE void connectSession(const QVariantList& instanceIds, bool isGroupControl);

    /**
     * @brief 关闭当前会话并释放资源
     * 
     * 执行流程：
     *   1. 取消会话观察者
     *   2. 取消视频帧观察者
     *   3. 销毁会话对象
     */
    Q_INVOKABLE void closeSession();

    /**
     * @brief 更新群控实例列表（仅群控模式有效）
     * @param instanceIds 当前选中的所有实例 ID
     * @param addedInstanceIds 本次新增的实例 ID（可选）
     * 
     * 内部调用：
     *   - tcr_instance_join_group()：将新实例加入群控组
     *   - tcr_instance_set_sync_list()：更新同步列表
     */
    Q_INVOKABLE void updateCheckedInstanceIds(const QVariantList& instanceIds, 
                                              const QVariantList& addedInstanceIds = QVariantList());

signals:
    /**
     * @brief 新视频帧到达信号
     * @param frame 封装的 YUV(I420) 视频帧数据
     * 
     * 由 VideoFrameCallback 触发，在主线程中发射
     */
    void newVideoFrame(VideoFrameDataPtr frame);

    /**
     * @brief 统计数据更新信号
     * 
     * 当收到 TCR_SESSION_EVENT_CLIENT_STATS 事件时触发
     */
    void clientStatsChanged();

    /**
     * @brief 屏幕方向变化信号
     * @param isLandscape true=横屏, false=竖屏
     * 
     * 当收到 TCR_SESSION_EVENT_SCREEN_CONFIG_CHANGE 事件时触发
     */
    void screenOrientationChanged(bool isLandscape);

public slots:
    // ==================== 触摸输入 ====================
    
    /**
     * @brief 发送触摸事件到云端
     * @param x, y 触摸点坐标
     * @param width, height 屏幕分辨率
     * @param eventType 事件类型：0=按下, 1=移动, 2=抬起
     * @param timestamp 事件时间戳（毫秒）
     * 
     * 对应 SDK API：tcr_session_touchscreen_touch()
     */
    void sendTouchEvent(int x, int y, int width, int height,
                          int eventType, qint64 timestamp);

    // ==================== 系统按键 ====================
    
    /**
     * @brief 发送返回键（Keycode: 158）
     * 对应 SDK API：tcr_session_send_keyboard_event(session, 158, true/false)
     */
    void onBackClicked();
    
    /**
     * @brief 发送 Home 键（Keycode: 172）
     */
    void onHomeClicked();
    
    /**
     * @brief 发送菜单键（Keycode: 139）
     */
    void onMenuClicked();
    
    /**
     * @brief 发送音量加键（Keycode: 58）
     */
    void onVolumUp();
    
    /**
     * @brief 发送音量减键（Keycode: 59）
     */
    void onVolumDown();

    // ==================== 流控制 ====================
    
    /**
     * @brief 暂停远端视频推流
     * 对应 SDK API：tcr_session_pause_streaming()
     */
    void onPauseVideoStreamClicked();
    
    /**
     * @brief 恢复远端视频推流
     * 对应 SDK API：tcr_session_resume_streaming()
     */
    void onResumeVideoStreamClicked();

    /**
     * @brief 暂停远端音频推流
     * 对应 SDK API：tcr_session_pause_streaming()
     */
    void onPauseAudioStreamClicked();
    
    /**
     * @brief 恢复远端音频推流
     * 对应 SDK API：tcr_session_resume_streaming()
     */
    void onResumeAudioStreamClicked();


    // ==================== 设备控制 ====================
    
    /**
     * @brief 开启本地摄像头
     * 对应 SDK API：tcr_session_enable_local_camera(session, true)
     */
    void onEnableCameraClicked();
    
    /**
     * @brief 关闭本地摄像头
     * 对应 SDK API：tcr_session_disable_local_camera()
     */
    void onDisableCameraClicked();

    /**
     * @brief 开启本地麦克风
     * 对应 SDK API：tcr_session_enable_local_microphone(session, true)
     */
    void onEnableMicrophoneClicked();
    
    /**
     * @brief 关闭本地麦克风
     * 对应 SDK API：tcr_session_enable_local_microphone(session, false)
     */
    void onDisableMicrophoneClicked();

    /**
     * @brief 设置远端视频流配置
     * 对应 SDK tcr_session_set_remote_video_profile()
     */
    void onVideoStreamSettingsClicked();

    // ==================== 摄像头设备管理 ====================
    
    /**
     * @brief 获取本地摄像头设备列表
     * @return 返回摄像头设备ID列表
     * 
     * 对应 SDK API：
     *   - tcr_session_get_camera_device_count()：获取设备数量
     *   - tcr_session_get_camera_device()：获取设备信息
     */
    Q_INVOKABLE QStringList getCameraDeviceList();

    /**
     * @brief 启用指定的摄像头设备
     * @param deviceId 摄像头设备ID
     * 
     * 对应 SDK API：
     *   - tcr_session_enable_local_camera_with_config()：使用配置启用摄像头
     */
    Q_INVOKABLE void enableCameraWithDevice(const QString& deviceId);

private:
    // ==================== 成员变量 ====================
    
    // 渲染相关
    VideoRenderPaintedItem* m_videoRenderItem = nullptr;  ///< 视频渲染组件
    
    // SDK 句柄
    TcrClientHandle      m_tcrClient   = nullptr;  ///< TcrSdk 客户端句柄（单例）
    TcrSessionHandle     m_session     = nullptr;  ///< 当前会话句柄
    TcrAndroidInstance   m_instance    = nullptr;  ///< Android 实例操作句柄
    TcrDataChannelHandle m_dataChannel = nullptr;  ///< 自定义数据通道句柄
    
    // SDK 回调结构体（生命周期必须覆盖整个会话）
    TcrSessionObserver    m_sessionObserver    = {};  ///< 会话事件回调
    TcrVideoFrameObserver m_videoFrameObserver = {};  ///< 视频帧回调
    
    // 业务数据
    ApiService*  m_apiService       = nullptr;  ///< API 服务（用于获取 Token 等）
    QStringList  m_groupInstanceIds;            ///< 群控模式下的实例 ID 列表
    bool         m_sessionConnected = false;    ///< 会话连接状态标志
    QString      m_clientStats;                 ///< 客户端统计数据（JSON 格式）

    // ==================== 内部方法 ====================
    
    /**
     * @brief 创建并初始化会话
     * 
     * 执行流程：
     *   1. 关闭旧会话（如果存在）
     *   2. 获取 TcrClient 实例
     *   3. 创建新会话：tcr_client_create_session()
     *   4. 设置观察者：setSessionObservers()
     */
    void createAndInitSession();

    /**
     * @brief 设置会话观察者
     * 
     * 注册两个核心回调：
     *   - 会话事件：tcr_session_set_observer()
     *   - 视频帧：tcr_session_set_video_frame_observer()
     */
    void setSessionObservers();

    /**
     * @brief 请求改变推流状态（内部辅助方法）
     * @param paused 是否暂停推流
     */
    void requestStreaming(bool paused);

    // ==================== SDK 回调函数（静态方法） ====================
    
    /**
     * @brief 会话事件回调（C 风格静态函数）
     * @param user_data 用户数据指针（指向 StreamingViewModel 实例）
     * @param event 事件类型
     * @param eventData 事件数据（JSON 字符串）
     * 
     * 处理的事件类型：
     *   - TCR_SESSION_EVENT_STATE_CONNECTED：连接成功
     *   - TCR_SESSION_EVENT_STATE_CLOSED：连接断开
     *   - TCR_SESSION_EVENT_CAMERA_STATUS：摄像头状态变化
     *   - TCR_SESSION_EVENT_CLIENT_STATS：统计数据更新
     * 
     * 注意：此函数在 SDK 内部线程调用，需要通过 QMetaObject::invokeMethod 切换到主线程
     */
    static void SessionEventCallback(void* user_data,
                                     TcrSessionEvent event,
                                     const char* eventData);

    /**
     * @brief 视频帧回调（C 风格静态函数）
     * @param user_data 用户数据指针（指向 StreamingViewModel 实例）
     * @param frame 视频帧句柄
     * 
     * 执行流程：
     *   1. 增加帧引用计数：tcr_video_frame_add_ref()
     *   2. 获取帧数据：tcr_video_frame_get_buffer()
     *   3. 封装为 VideoFrameDataPtr（智能指针，自动释放, 释放时会调用tcr_video_frame_release）
     *   4. 通过信号发送到主线程
     * 
     * 注意：此函数在解码线程调用，频率较高（通常 30-60 FPS）
     */
    static void VideoFrameCallback(void* user_data,
                                   TcrVideoFrameHandle frame);

    qreal m_currentRotationAngle = 0.0;  // 保存当前旋转角度，用于新实例创建时应用
    int m_currentVideoWidth = 0;    // 保存当前视频流宽度，用于新实例创建时应用
    int m_currentVideoHeight = 0;   // 保存当前视频流高度，用于新实例创建时应用
};
