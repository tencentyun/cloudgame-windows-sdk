// tcr_c_api.h
#pragma once

/**
 * @file tcr_c_api.h
 * @brief TcrSdk 接口头文件
 *
 * 本文件定义了 TcrSdk 的 C 语言接口，支持云手机实例的批量管理、串流会话、视频帧回调、事件监听等功能。
 *
 * ==================== 典型使用流程示例 ====================
 *
 * 1. 初始化 TcrClient
 *    - 获取 TcrClientHandle
 *    - 配置 Token 和 AccessInfo
 *    - 调用 tcr_client_init 进行初始化
 *
 * 2. 获取实例截图（可选）
 *    - 通过 tcr_client_get_android_instance 获取 AndroidInstance 句柄
 *    - 调用 tcr_instance_get_image 获取指定实例的小流截图
 *
 * 3. 创建串流会话
 *    - 通过 tcr_client_create_session 创建会话句柄
 *    - 调用 tcr_session_access 连接到指定实例, 或连接多实例(群控)
 *
 * 4. 设置回调
 *   - 设置视频帧回调（tcr_session_set_video_frame_observer），用于获取解码后的视频帧
 *    - 设置事件回调（tcr_session_set_observer），用于监听会话内部或云端下发的消息
 *
 * 5. 业务操作
 *    - 可通过 tcr_instance_request 批量操作云手机实例（如修改分辨率、粘贴文本、应用管理等）
 *   - 可通过 tcr_session_paste_text、tcr_session_send_keyboard_event、tcr_session_touchscreen_touch 等接口进行实时控制
 *
 * 6. 资源释放
 *    - 使用完毕后，调用 tcr_client_destroy_session 销毁会话实例
 *
 * -------------------- 代码示例 --------------------
 * 
 *   TcrConfig config;
 *   config.Token = tokenResult.token.c_str();
 *   config.AccessInfo = tokenResult.accessInfo.c_str();
 * 
 *   // 1. 初始化
 *   TcrClientHandle tcrClient = tcr_client_get_instance();
 *   tcr_client_init(tcrClient, &config);
 * 
 *   // 2. 获取小流截图
 *   char buffer[2048];
 *   TcrAndroidInstance instance = tcr_client_get_android_instance(tcrClient);
 *   tcr_instance_get_image(instance, buffer, 2048, instanceIds[0].c_str(), 0, 0, 0);
 * 
 *   // 3. 创建串流会话
 *   TcrSessionHandle tcrSession = tcr_client_create_session(tcrClient);
 * 
 *   // 4. 连接实例
 *   tcr_session_access(tcrSession, instanceIds, 1, false);
 * 
 *   // 5. 设置视频帧回调
 *   static TcrVideoFrameObserver video_observer;
 *   video_observer.user_data = this;
 *   video_observer.on_frame = VideoFrameCallback;
 *   tcr_session_set_video_frame_observer(tcrSession, &video_observer);
 * 
 *   // 6. 设置事件回调
 *   static TcrSessionObserver session_observer;
 *   session_observer.user_data = this;
 *   session_observer.on_event = SessionEventCallback;
 *   tcr_session_set_observer(tcrSession, &session_observer);
 * 
 *   // ... 业务操作 ...
 * 
 *   // 7. 销毁会话
 *   tcr_client_destroy_session(tcrClient, tcrSession);
 *
 * --------------------------------------------------
 *
 * 详细接口说明请参见各函数注释。
 */

#include <stddef.h>
#include <stdint.h>
#include "tcr_export.h"
#include "tcr_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* TcrClientHandle;
typedef void* TcrSessionHandle;
typedef void* TcrAndroidInstance;
typedef void* TcrDataChannelHandle;

// 视频帧观察者结构体
typedef struct TcrVideoFrameObserver {
    void* user_data;
    // 视频帧回调函数，解码后的视频帧数据通过此回调返回
    void (*on_frame_buffer)(void* user_data, const TcrVideoFrameBuffer* frame_buffer, 
                           int64_t timestamp_us, TcrVideoRotation rotation);
} TcrVideoFrameObserver;

// 会话事件观察者结构体
typedef struct TcrSessionObserver {
    void* user_data;
    // 会话事件回调函数，所有会话相关事件通过此回调返回
    void (*on_event)(void* user_data, TcrSessionEvent event, const char* eventData);
} TcrSessionObserver;

// ==================== Client 相关接口 ====================

/**
 * @brief 获取TcrClientHandle实例
 * @return TcrClientHandle句柄，后续所有客户端相关操作均需传入该句柄
 */
TCRSDK_API TcrClientHandle tcr_client_get_instance();

/**
 * @brief 初始化TcrClientHandle
 * @param client TcrClientHandle
 * @param config TcrConfig参数指针
 * @return 错误码，TCR_SUCCESS 表示初始化成功，TCR_ERR_INVALID_PARAMS 表示参数有误
 * @note 这个函数可以重复调用以便更新TcrConfig中的Token和AccessInfo
 */
TCRSDK_API TcrErrorCode tcr_client_init(TcrClientHandle client, const TcrConfig* config);

/**
 * @brief 创建会话对象, 以便进行串流和控制云手机实例
 * @param client TcrClientHandle
 * @return 会话句柄，成功返回非空，若Client未初始化（即未调用tcr_client_init或初始化失败），则返回nullptr。
 */
TCRSDK_API TcrSessionHandle tcr_client_create_session(TcrClientHandle client);

/**
 * @brief 销毁会话对象，释放相关资源
 * @param client TcrClientHandle
 * @param session 需要销毁的会话句柄
 */
TCRSDK_API void tcr_client_destroy_session(TcrClientHandle client, TcrSessionHandle session);

/**
 * @brief 设置会话事件观察者，用于接收会话生命周期及业务事件
 * @param session 会话句柄
 * @param observer 观察者结构体指针
 * @note 调用时序要求：
 *       - 当会话配置中的 TcrSdkType 为 CloudStream 时，必须在成功调用 tcr_session_init 之后方可调用本接口
 */
TCRSDK_API void tcr_session_set_observer(TcrSessionHandle session, const TcrSessionObserver* observer);

/**
 * @brief 获取 TcrAndroidInstance 操作对象
 * @param client TcrClientHandle
 * @return TcrAndroidInstance 句柄，成功返回非空，若Client未初始化（即未调用tcr_client_init或初始化失败），则返回nullptr。
 */
TCRSDK_API TcrAndroidInstance tcr_client_get_android_instance(TcrClientHandle client);

// ==================== 云手机操作接口  ====================

/**
 * @brief 将多个实例加入当前群组会话，实现多实例同步控制
 * @param session TcrAndroidInstance 句柄
 * @param instanceIds 实例 ID 字符串数组
 * @param length 实例 ID 数组长度
 */
TCRSDK_API void tcr_instance_join_group(TcrAndroidInstance op, const char** instanceIds, int32_t length);

/**
 * @brief 请求打开或关闭指定实例的视频流，并设置流质量
 * @param session TcrAndroidInstance 句柄
 * @param instanceID 实例 ID
 * @param status true 表示打开流，false 表示关闭流
 * @param level 流质量级别（"low"、"normal"、"high"）
 */
TCRSDK_API void tcr_instance_request_stream(TcrAndroidInstance op, const char* instanceID, bool status, const char* level);


/**
 * @brief 设置或取消主控实例，实现主从同步
 * @param session TcrAndroidInstance 句柄
 * @param instanceId 主控实例 ID
 * @param status true 表示设置为主控，false 表示取消主控
 */
TCRSDK_API void tcr_instance_set_master(TcrAndroidInstance op, const char* instanceId, bool status);

/**
 * @brief 设置群组会话中需要被群控的实例列表。
 * @param session TcrAndroidInstance 句柄
 * @param instanceIds 需要同步控制的实例ID字符串数组, 这些ID必须已经添加到群组会话中。
 * @param length 实例 ID 数组长度
 * @note 仅群组会话成员可同步，需先通过 join_group_session 加入群组
 */
TCRSDK_API void tcr_instance_set_sync_list(TcrAndroidInstance op, const char** instanceIds, int32_t length);


/**
 * @brief 获取指定实例的截图图片 URL
 * @param op TcrAndroidInstance 句柄
 * @param buffer 输出缓冲区，用于接收图片 URL 字符串
 * @param bufferLen 输出缓冲区长度
 * @param instanceId 实例 ID
 * @param width 图片宽度（<=0 使用默认分辨率）
 * @param height 图片高度（<=0 使用默认分辨率）
 * @param quality 图片质量（1-100）
 * @return 是否成功获取到图片 URL，true 表示成功
 */
TCRSDK_API bool tcr_instance_get_image(TcrAndroidInstance op, char* buffer, int bufferLen, const char* instanceId, int width = 0, int height = 0, int quality = 20);

/**
 * @brief 获取文件下载URL
 * @param op TcrAndroidInstance句柄
 * @param buffer 输出缓冲区
 * @param bufferLen 缓冲区长度
 * @param instanceId 实例ID
 * @param path 文件路径
 * @return 是否成功获取到下载地址
 */
TCRSDK_API bool tcr_instance_get_download_address(TcrAndroidInstance op, char* buffer, int bufferLen, const char* instanceId, const char* path);

/**
 * 获取云手机实例logcat日志压缩包的下载地址
 * @param op TcrAndroidInstance句柄
 * @param buffer 输出缓冲区
 * @param bufferLen 缓冲区长度
 * @param instanceId 实例ID
 * @param recentDays 表示下载几天内修改过的logcat日志文件
 * @return 是否成功获取到下载地址
 */
TCRSDK_API bool tcr_instance_get_download_logcat_address(TcrAndroidInstance op, char* buffer, int bufferLen, const char* instanceId, int recentDays);

/**
 * @brief 上传文件到云实例, 固定上传至/data/media/0/DCIM, 上传完毕会通知系统添加媒体文件到相册中
 *
 * @param op         TcrAndroidInstance句柄
 * @param instanceId 云手机实例ID
 * @param local_paths 本地文件路径数组（每个为绝对路径，如 "C:/tmp/a.txt", 上传前请确保文件存在且可读）
 * @param file_count 文件数量
 * @param output     输出参数，返回的JSON字符串（需调用 tcr_instance_free_result 释放）
 * @param output_size 输出参数，返回字符串的长度
 * @param callback 当前所有文件的上传进度回调（可为NULL）。
 * @return true表示调用成功（但需检查output中的Code字段是否为0），false表示调用失败
 * @par 返回结果格式（JSON字符串）:
 * @code{.json}
 * {
 *   "Code": 0,                 // 0表示成功，非0表示失败
 *   "Msg": "xxx"               // 失败时的错误信息
 * }
 * @endcode
 * @par 字段说明
 * - Code: 0表示成功，非0表示失败
 * - Message: 失败时的错误信息
 *
 * @note local_paths 必须为全英文路径，不能包含中文字符
 * @note output 由SDK内部分配，使用完毕后需调用 tcr_instance_free_result 释放
 * @note 该接口支持一次传输多个文件，但由于多个文件上传是串行进行的，因此如果文件较多，耗时会比较久，建议分批多次调用。
 */
TCRSDK_API bool tcr_instance_upload_media(
    TcrAndroidInstance op,
    const char* instanceId,
    const char** local_paths,
    int file_count,
    char** output,
    size_t* output_size,
    const TcrUploadCallback* callback
);

/**
 * @brief 上传本地文件到云手机实例
 *
 * @param op         TcrAndroidInstance句柄
 * @param instanceId 云手机实例ID
 * @param local_paths 本地文件路径数组（每个为绝对路径，如 "C:/tmp/a.txt", 上传前请确保文件存在且可读）
 * @param cloud_paths 云端目标路径数组（可为NULL，表示上传到默认/sdcard/Download目录；否则每个路径为"/sdcard/xxx", 文件放在该路径下）
 * @param file_count 文件数量
 * @param output     输出参数，返回的JSON字符串（需调用 tcr_instance_free_result 释放）
 * @param output_size 输出参数，返回字符串的长度
 * @param callback 当前所有文件的上传进度回调（可为NULL）。
 * @return true表示调用成功（但需检查output中的Code字段是否为0），false表示调用失败
 * @par 返回结果格式（JSON字符串）:
 * @code{.json}
 * {
 *   "Code": 0,
 *   "Message": "xxx",
 *   "FileStatus": [
 *     {
 *       "FileName":"test_demo.py",
 *       "CloudPath":"/sdcard/123456/test_demo.py"
 *     },
 *     ...
 *   ]
 * }
 * @endcode
 * @par 字段说明
 * - Code: 0表示成功，非0表示失败
 * - Message: 失败时的错误信息
 * - FileStatus: 每个文件的上传状态
 *
 * @note local_paths 必须为全英文路径，不能包含中文字符, cloud_paths 可为 NULL，表示所有文件上传到默认目录/sdcard/Download
 * @note output 由SDK内部分配，使用完毕后需调用 tcr_instance_free_result 释放
 * @note 该接口支持一次传输多个文件，但由于多个文件上传是串行进行的，因此如果文件较多，耗时会比较久，建议分批多次调用。
 * 
 */
TCRSDK_API bool tcr_instance_upload_files(
    TcrAndroidInstance op,
    const char* instanceId,
    const char** local_paths,
    const char** cloud_paths,
    int file_count,
    char** output,
    size_t* output_size,
    const TcrUploadCallback* callback
);

/**
 * @brief 批量请求云端任务（如修改分辨率、粘贴文本、修改 GPS、应用管理等）
 * @param op TcrAndroidInstance 句柄
 * @param output 输出参数，返回的 JSON 字符串（需调用 tcr_instance_free_result 释放）
 * @param output_size 输出参数，返回字符串的长度
 * @param input_json 输入参数，JSON 字符串，格式如下：
 * @par 输入JSON格式:
 * @code{.json}
 * {
 *   "TaskType": "任务类型字符串",
 *   "Params": {
 *      "cai-xxxx-xxx(实例ID)": { ... },
 *      "cai-yyyy-yyy(实例ID)": { ... }
 *   }
 * }
 * @endcode
 * @par 字段说明
 * - TaskType: 必填，支持的任务类型见下列说明
 * - Params: 必填，包含实例ID到任务参数的映射
 *   - 每个实例ID对应的对象结构取决于任务类型
 * 
 * @section 支持的任务类型及参数格式
 * - ModifyResolution: 修改分辨率
 *   - 参数: { "Width": 整数, "Height": 整数, "DPI": 整数(可选) }
 * 
 * - ModifyGPS: 修改GPS位置
 *   - 参数: { "Longitude": 浮点数, "Latitude": 浮点数 }
 * 
 * - Paste: 粘贴文本
 *   - 参数: { "Text": "字符串" }
 * 
 * - SendClipboard: 发送剪贴板内容
 *   - 参数: { "Text": "字符串" }
 * 
 * - ModifySensor: 修改传感器数据
 *   - 参数结构:
 *     {
 *       "Type": "字符串",         // 传感器类型，必填。支持类型：
 *         - "accelerometer"      // 加速度计，x/y/z轴加速度
 *         - "gyroscope"          // 陀螺仪，x/y/z轴角速度
 *         - "significant_motion" // 重要运动检测
 *         - "step_counter"       // 计步器
 *         - "step_detector"      // 步态检测
 *         - "pressure"           // 气压计
 *         - "proximity"          // 距离传感器
 *       "Values": [浮点数, ...], // 数值数组，长度和含义依赖Type
 *       "Accuracy": 整数         // 精度级别，目前仅支持3（高精度）
 *     }
 * 
 * - Shake: 摇动设备 (无参数)
 *   - 参数: {}
 * 
 * - Blow: 吹气 (无参数)
 *   - 参数: {}
 * 
 * - SendTransMessage: 发送透传消息
 *   - 参数: { "PackageName": "字符串", "Msg": "字符串" }
 *   - 备注: 目标app需实现Messenger服务，否则无法收到消息
 * 
 * - ModifyInstanceProperties: 修改实例属性
 *   - 参数结构:
 *     {
 *       "DeviceInfo": {        // 设备信息
 *         "Brand": "字符串",
 *         "Model": "字符串"
 *       },
 *       "ProxyInfo": {         // 代理信息（可选）
 *         "Enabled": 布尔值,
 *         "Protocol": "字符串", // 例如"socks5"
 *         "Host": "字符串",
 *         "Port": 整数,
 *         "User": "字符串",
 *         "Password": "字符串"
 *       },
 *       "GPSInfo": {           // GPS信息（可选）
 *         "Longitude": 浮点数,
 *         "Latitude": 浮点数
 *       },
 *       "SIMInfo": {           // SIM卡信息（可选）
 *         "State": 整数,
 *         "PhoneNumber": "字符串",
 *         "IMSI": "字符串",
 *         "ICCID": "字符串"
 *       },
 *       "LocaleInfo": {        // 时区信息（可选）
 *         "Timezone": "字符串"
 *       },
 *       "LanguageInfo": {      // 语言信息（可选）
 *         "Language": "字符串",
 *         "Country": "字符串"
 *       },
 *       "ExtraProperties": [   // 额外自定义属性（可选）
 *         { "Key": "字符串", "Value": "字符串" },
 *         ...
 *       ]
 *     }
 * 
 * - ModifyKeepFrontAppStatus: 修改保活应用状态
 *   - 参数: { "PackageName": "字符串", "Enable": 布尔值, "RestartInterValSeconds": 整数(重新拉起最长间隔，单位秒，可选) }
 * 
 * - UnInstallByPackageName: 卸载应用
 *   - 参数: { "PackageName": "字符串" }
 * 
 * - StartApp: 启动应用
 *   - 参数: { "PackageName": "字符串", "ActivityName": "字符串" }
 * 
 * - StopApp: 停止应用
 *   - 参数: { "PackageName": "字符串" }
 * 
 *  - ClearAppData: 清除应用数据
 *   - 参数: { "PackageName": "字符串" }
 * 
 * - EnableApp: 启用应用
 *   - 参数: { "PackageName": "字符串" }
 * 
 * - DisableApp: 禁用应用
 *   - 参数: { "PackageName": "字符串" }
 * 
 * - StartCameraMediaPlay: 开始摄像头媒体播放
 *   - 参数: { "FilePath": "字符串", "Loops": 整数(循环次数，负数表示无限循环，可选) }
 * 
 * - DisplayCameraImage: 显示摄像头图像
 *   - 参数: { "FilePath": "字符串" }
 *   - 说明: 通过指定的文件路径显示摄像头图像，FilePath为本地存储的图片文件路径，例如 "/sdcard/image.jpg"。
 * 
 * - AddKeepAliveList: 添加保活列表
 *   - 参数: { "AppList": ["字符串1", "字符串2", ...] }
 *   - 说明: 向保活列表中添加指定的应用包名列表，AppList为字符串数组，包含需要保活的应用包名，例如 ["com.wechat", "com.alipay", "com.dingtalk"]。
 * 
 * - RemoveKeepAliveList: 移除保活列表
 *   - 参数: { "AppList": ["字符串1", "字符串2", ...] }
 *   - 说明: 从保活列表中移除指定的应用包名列表，AppList为字符串数组，包含需要移除保活的应用包名，例如 ["com.wechat", "com.alipay", "com.dingtalk"]。
 * 
 * - SetKeepAliveList: 设置保活列表
 *   - 参数: { "AppList": ["字符串1", "字符串2", ...] }
 *   - 说明: 设置保活列表为指定的应用包名列表，覆盖之前的保活列表，AppList为字符串数组，包含需要保活的应用包名，例如 ["com.wechat", "com.alipay", "com.dingtalk"]。
 * 
 * - DescribeInstanceProperties: 获取实例属性 (无参数)
 *   - 参数: {}
 *   - 返回示例:
 *     {
 *       "DeviceInfo": { "Brand": "Samsung", "Model": "Galaxy S23" },
 *       "ProxyInfo": {
 *         "Enabled": true,
 *         "Protocol": "socks5",
 *         "Host": "proxy.example.com",
 *         "Port": 1080,
 *         "User": "username",
 *         "Password": "password123"
 *       },
 *       "GPSInfo": { "Longitude": 121.4737, "Latitude": 31.2304 },
 *       "SIMInfo": {
 *         "State": 1,
 *         "PhoneNumber": "13800138000",
 *         "IMSI": "460001234567890",
 *         "ICCID": "89860123456789012345"
 *       },
 *       "LocaleInfo": { "Timezone": "Asia/Shanghai" },
 *       "LanguageInfo": { "Language": "zh", "Country": "CN" }
 *     }
 * 
 * - DescribeKeepFrontAppStatus: 获取保活应用状态 (无参数)
 *   - 参数: {}
 *   - 返回示例:
 *     {
 *       "PackageName": "com.example.app",           // 保活应用的包名
 *       "Enable": true,                             // 保活状态，true表示启用保活，false表示禁用
 *       "RestartInterValSeconds": 3                  // 应用重启的最长间隔时间，单位秒
 *     }
 *   - 说明: 返回当前保活应用的状态信息，包括包名、是否启用保活以及重启间隔时间，便于监控和管理保活策略。
 * 
 * - StopCameraMediaPlay: 停止摄像头播放 (无参数)
 *   - 参数: {}
 * 
 * - DescribeCameraMediaPlayStatus: 获取摄像头播放状态 (无参数)
 *   - 参数: {}
 *   - 返回示例:
 *     {
 *       "FilePath": "/sdcard/video.mp4",
 *       "Loops": 3
 *     }
 *   - 说明: 返回当前摄像头媒体播放的状态信息，包括正在播放的媒体文件路径FilePath和循环播放次数Loops，Loops为负数表示无限循环。
 * 
 * - DescribeKeepAliveList: 获取保活列表 (无参数)
 *   - 参数: {}
 *   - 返回示例:
 *     {
 *       "AppList": ["com.wechat", "com.alipay", "com.dingtalk"]
 *     }
 *   - 说明: 返回当前保活列表中的应用包名数组，便于查询和管理保活应用。
 * 
 * - ClearKeepAliveList: 清除保活列表 (无参数)
 *   - 参数: {}
 * 
 * - ListUserApps: 列出用户应用 (无参数)
 *   - 参数: {}
 *   - 返回示例:
 *     {
 *       "AppList": [
 *         {
 *           "PackageName": "com.android.chrome",       // 应用包名
 *           "LastUpdateTimeMs": 1667232000000,         // 应用最后更新时间，单位毫秒时间戳
 *           "FirstInstallTimeMs": 1635724800000,       // 应用首次安装时间，单位毫秒时间戳
 *           "VersionName": "115.0.5790.166",           // 应用版本名称
 *           "Label": "Chrome"                          // 应用显示名称
 *         },
 *         ...
 *       ]
 *     }
 *   - 说明: 返回当前实例中已安装的用户应用列表，每个应用包含包名、版本信息、安装及更新时间等详细信息。
 * 
 * - Mute: 静音开关
 *   - 参数: { "Mute": 布尔值 }
 * 
 * - MediaSearch: 媒体库文件搜索
 *   - 参数: { "Keyword": "字符串" }
 *   - 返回示例:
 *     {
 *       "MediaList": [
 *         {
 *           "FileName": "abc123.mp4",           // 文件名
 *           "FilePath": "/sdcard/xxxx",         // 文件路径
 *            "FileBytes": 12345,                 // 文件大小，单位字节
 *           "FileModifiedTime": 1749712950000   // 文件最后修改时间，时间戳（毫秒）
 *         },
 *         ...
 *       ]
 *     }
 *   - 说明: 根据关键字搜索媒体库中的文件，返回匹配的文件列表，每个文件包含名称、路径、大小及最后修改时间等详细信息。
 * 
 * - Reboot: 重启实例 (无参数)
 *   - 参数: {}
 * 
 * - ListAllApps: 查询所有应用列表 (无参数)
 *   - 参数: {}
 *   - 返回示例:
 *     {
 *       "AppList": [
 *         {
 *           "PackageName": "com.android.chrome",       // 应用包名
 *           "LastUpdateTimeMs": 1667232000000,         // 应用最后更新时间，单位毫秒时间戳
 *           "FirstInstallTimeMs": 1635724800000,       // 应用首次安装时间，单位毫秒时间戳
 *           "VersionName": "115.0.5790.166",           // 应用版本名称
 *           "Label": "Chrome"                          // 应用显示名称
 *         },
 *         ...
 *       ]
 *     }
 *   - 说明: 返回当前实例中所有已安装的应用列表，包括系统应用和用户应用。每个应用包含包名、版本信息、安装及更新时间、应用名称等详细信息，便于全面管理和查询。
 * 
 * - MoveAppBackground: 关闭应用至后台 (无参数)
 *   - 参数: {}
 * 
 * - AddAppInstallBlackList: 新增应用安装黑名单列表
 *   - 参数: { "AppList": ["字符串1", "字符串2", ...] }
 *   - 说明: 向应用安装黑名单中添加指定的应用包名列表，AppList为字符串数组，包含需要禁止安装的应用包名，例如 ["com.wechat", "com.alipay", "com.dingtalk"]。
 * 
 *  - RemoveAppInstallBlackList: 移除应用安装黑名单列表
 *   - 参数: { "AppList": ["字符串1", "字符串2", ...] }
 *   - 说明: 从应用安装黑名单中移除指定的应用包名列表，AppList为字符串数组，包含需要解除禁止安装的应用包名，例如 ["com.wechat", "com.alipay", "com.dingtalk"]。
 * 
 * - SetAppInstallBlackList: 覆盖应用安装黑名单列表
 *   - 参数: { "AppList": ["字符串1", "字符串2", ...] }
 *   - 说明: 设置应用安装黑名单为指定的应用包名列表，覆盖之前的黑名单，AppList为字符串数组，包含需要禁止安装的应用包名，例如 ["com.wechat", "com.alipay", "com.dingtalk"]。
 * 
 * - DescribeAppInstallBlackList: 查询应用安装黑名单列表 (无参数)
 *   - 参数: {}
 *   - 返回示例:
 *     {
 *       "AppList": ["com.wechat", "com.alipay", "com.dingtalk"]
 *     }
 *   - 说明: 返回当前应用安装黑名单中的应用包名数组，便于查询和管理被禁止安装的应用列表。
 * 
 * - ClearAppInstallBlackList: 清空应用安装黑名单列表 (无参数)
 *   - 参数: {}
 * 
 *  @note 每个任务类型对应的返回示例均已在上方说明中给出；如果某个任务类型未明确给出返回示例，则默认返回示例为
 *   - 返回示例:
 *     {
 *       "Code": 0,
 *       "Msg": "错误信息"
 *     }
 * 
 * 
 * @param output 输出缓冲区，用于接收返回的JSON字符串
 * @note output 由内部分配内存，使用完毕后需调用 tcr_instance_free_result 释放
 * 
 * @section 返回格式为一个JSON对象，key为实例ID，value为结果对象，示例：
 * {
 *   "cai-xxxx": {
 *       "Code": 10100,
 *       "Msg": "cai opeartion denied"
 *   },
 *   "cai-yyyy": {
 *       "Code": 0
 *   },
 *   "cai-zzzz": {
 *       "Code": -101,  // 本地网络请求错误示例，Code < -100
 *       "Msg": "HTTP request failed"
 *   }
 * }
 * @note 返回的结果中实例对应的Code为0表示成功，其他值表示失败，Msg为失败原因。
 * 
 * @param output_size 输出缓冲区大小
 * 
 * @return TcrErrorCode 返回错误码：
 *   - TCR_SUCCESS: 任务请求成功
 *   - TCR_ERR_INVALID_PARAMS: 输入参数无效
 */
TCRSDK_API TcrErrorCode tcr_instance_request(TcrAndroidInstance op, const char* input_json, char** output, size_t* output_size);

/**
 * @brief 释放由 tcr_instance_request 分配的结果内存
 * @param op TcrAndroidInstance 句柄
 * @param output 需要释放的内存指针
 */
TCRSDK_API void tcr_instance_free_result(TcrAndroidInstance op, char* output);

// ==================== 音视频会话接口 ====================

/**
 * @brief 创建自定义数据通道
 * @param session 会话句柄
 * @param port 云端唯一标识数据通道的端口号
 * @param observer 观察者结构体指针（生命周期由调用方保证有效，回调时会传入 user_data）
 * @return 数据通道句柄，失败返回 NULL
 */
TCRSDK_API TcrDataChannelHandle tcr_session_create_data_channel(
    TcrSessionHandle session,
    int32_t port,
    const TcrDataChannelObserver* observer
);

/**
 * @brief 通过自定义数据通道发送数据
 * @param channel 数据通道句柄
 * @param data 数据指针
 * @param size 数据长度
 * @return 错误码，TCR_SUCCESS 表示成功
 */
TCRSDK_API TcrErrorCode tcr_data_channel_send(
    TcrDataChannelHandle channel,
    const uint8_t* data, 
    size_t size
);

/**
 * @brief 关闭自定义数据通道
 * @param channel 数据通道句柄
 */
TCRSDK_API void tcr_data_channel_close(TcrDataChannelHandle channel);

/**
 * @brief 初始化会话。
 *
 * 调用此方法初始化会话，初始化成功后会通过TcrSessionObserver回调
 * 发送 TCR_SESSION_EVENT_STATE_INITED 事件通知。
 */
TCRSDK_API void tcr_session_init(TcrSessionHandle session);

/**
 * 开始会话.
 * 注意这个函数对于每个会话只能调用一次，
 */
TCRSDK_API void tcr_session_start(TcrSessionHandle session, const char* serverSession);


/**
 * @brief 设置远端视频帧观察者，用于接收解码后的视频帧数据
 * @param session 会话句柄
 * @param observer 视频帧观察者结构体指针
 * @note 在释放 observer 前，必须先调用本接口传入 NULL，否则可能导致崩溃
 * @note 调用时序要求：
 *       - 当会话配置中的 TcrSdkType 为 CloudStream 时，必须在成功调用 tcr_session_init 之后方可调用本接口
 */
TCRSDK_API void tcr_session_set_video_frame_observer(TcrSessionHandle session, const TcrVideoFrameObserver* observer);

/**
 * @brief 启动会话并连接云手机。
 * @param session 会话句柄
 * @param instanceIds 云端实例 ID
 * @param length 实例ID列表的长度
 * @param isGroupControl 是否群控
 */
TCRSDK_API void tcr_session_access(TcrSessionHandle session, const char** instanceIds, int32_t length, bool isGroupControl);

/**
 * @brief 暂停媒体流（如视频流），通常用于临时挂起
 * @param session 会话句柄
 */
TCRSDK_API void tcr_session_pause_streaming(TcrSessionHandle session);

/**
 * @brief 恢复媒体流（如视频流），与暂停配合使用
 * @param session 会话句柄
 */
TCRSDK_API void tcr_session_resume_streaming(TcrSessionHandle session);

/**
 * @brief 设置远端视频流参数（帧率、码率等）
 * @param session 会话句柄
 * @param fps 视频帧率（建议 30）
 * @param minBitrate 最小码率（单位 kbps，建议 512）
 * @param maxBitrate 最大码率（单位 kbps，建议 1024）
 */
TCRSDK_API void tcr_session_set_remote_video_profile(TcrSessionHandle session, int32_t fps, int32_t minBitrate, int32_t maxBitrate);

/**
 * @brief 启用本地摄像头
 * @param session 会话句柄
 * @param config 视频配置参数
 * @return 是否成功启用，true表示成功
 * 
 * @note TcrVideoConfig中的配置参数为推荐配置，内部会自动查找并选择最匹配的摄像头。
 *       客户端可通过调用tcr_session_get_camera_device_count和tcr_session_get_camera_device
 *       查询当前设备支持的摄像头配置能力，以便选择合适的配置参数。
 */
TCRSDK_API bool tcr_session_enable_local_camera_with_config(TcrSessionHandle session, const TcrVideoConfig* config);

/**
 * @brief 启用或禁用本地摄像头
 * @param session 会话句柄
 * @param enable true表示启用摄像头，false表示禁用摄像头
 * @return 是否操作成功，true表示成功
 */
TCRSDK_API bool tcr_session_enable_local_camera(TcrSessionHandle session, bool enable);

/**
 * @brief 禁用本地摄像头
 * @param session 会话句柄
 */
TCRSDK_API void tcr_session_disable_local_camera(TcrSessionHandle session);

/**
 * @brief 检查本地摄像头是否已启用
 * @param session 会话句柄
 * @return 是否已启用，true表示已启用
 */
TCRSDK_API bool tcr_session_is_local_camera_enabled(TcrSessionHandle session);

/**
 * @brief 配置本地摄像头参数
 * @param session 会话句柄
 * @param max_bitrate_bps 最大码率（单位 bps）
 * @param max_fps 最大帧率（单位 fps）
 * @return 是否配置成功，true表示成功
 * @note 这个函数需要在启用本地摄像头(tcr_session_enable_local_camera)后调用，否则无效。
 */

TCRSDK_API bool tcr_session_local_camera_config(TcrSessionHandle session, int max_bitrate_bps, int max_fps);

/**
 * @brief 获取可用的摄像头设备数量.
 * 
 * @param session   会话句柄.
 * @return          可用设备数量 (>=0), 负值表示出错.
 */
TCRSDK_API int tcr_session_get_camera_device_count(TcrSessionHandle session);


/**
 * @brief 获取指定摄像头设备的详细信息.
 * 
 * @param session       会话句柄.
 * @param device_index  设备索引, 范围: [0, 设备数量-1].
 * @param info          输出参数, 指向 TcrCameraDeviceInfo 结构体, 由调用方分配.
 * @return              成功返回 true, 失败返回 false.
 *
 * @note 典型用法:
 *   int count = tcr_session_get_camera_device_count(session);
 *   for (int i = 0; i < count; ++i) {
 *     TcrCameraDeviceInfo info;
 *     bool ok = tcr_session_get_camera_device(session, i, &info);
 *     if (ok) {
 *         // 用 info.device_name / device_id / capability 列表等
 *     }
 *   }
 */
TCRSDK_API bool tcr_session_get_camera_device(TcrSessionHandle session, int device_index, TcrCameraDeviceInfo* info);

/**
 * @brief 向云端应用发送透传消息
 * @param session 会话句柄
 * @param package_name 应用包名
 * @param msg 消息内容
 * @note 目标应用需实现 Messenger 服务，否则无法收到消息
 */
TCRSDK_API void tcr_session_send_trans_message(TcrSessionHandle session, const char* package_name, const char* msg);

/**
 * @brief 将文本写入剪贴板并粘贴到输入框
 * @param msg 消息内容
 * @note 此操作会将文本写入云手机的剪贴板，并模拟粘贴操作将文本输入到当前焦点输入框。
 */
TCRSDK_API void tcr_session_paste_text(TcrSessionHandle session, const char* msg);

/**
 * @brief 将文本直接输入到输入框
 * @param session 会话句柄
 * @param content 要输入的文本内容
 * @param mode 输入模式："append"(追加)或"override"(覆盖)
 * @note 调用此接口前，客户端必须已收到TCR_SESSION_EVENT_IME_STATUS_CHANGE事件，
 *       且事件中的ime_type为"local"（表示使用本地输入法）。否则输入会失败。
 */
TCRSDK_API void tcr_session_input_text(TcrSessionHandle session, const char* content, const char* mode);

/**
 * @brief 切换输入法
 * @param session 会话句柄
 * @param ime 输入法类型："cloud"或"local"
 */
TCRSDK_API void tcr_session_switch_ime(TcrSessionHandle session, const char* ime);

// ==================== 外设控制接口接口 ====================

/**
 * @brief 开关摄像头
 * @param session 会话句柄
 * @param status 状态："open"或"close"
 */
TCRSDK_API void tcr_session_switch_camera(TcrSessionHandle session, const char* status);

/**
 * @brief 开关麦克风
 * @param session 会话句柄
 * @param status 状态："open"或"close"
 */
TCRSDK_API void tcr_session_switch_mic(TcrSessionHandle session, const char* status);

/**
 * @brief 发送云端键盘事件（按键按下/抬起）
 *
 * 该接口用于向云端实例发送单个按键事件，通常需要成对调用（即先发送按下事件，再发送抬起事件），
 * 类似于真实物理键盘的操作。例如，模拟输入字母“A”时，应先调用一次 down=true，再调用一次 down=false。
 *
 * @param session 会话句柄
 * @param keycode 按键码（可参考 https://www.toptal.com/developers/keycode 或 Android KeyEvent 定义）
 * @param down    true 表示按下（KeyDown），false 表示抬起（KeyUp）
 *
 * 常用云手机按键 keycode 对照表（十进制）：
 *   - KEY_BACK           = 158   // 返回键
 *   - KEY_MENU           = 139   // 菜单键
 *   - KEY_HOME           = 172   // Home 键
 *   - KEYCODE_VOLUME_UP  = 58    // 音量加
 *   - KEYCODE_VOLUME_DOWN= 59    // 音量减
 *
 * 示例：模拟点击返回键
 *   tcr_session_send_keyboard_event(session, 158, true);   // 按下
 *   tcr_session_send_keyboard_event(session, 158, false);  // 抬起
 *
 * @note
 * - keycode 建议使用十进制，部分特殊按键可参考上表。
 * - 若需模拟连续输入，请依次发送对应的按下/抬起事件。
 */
TCRSDK_API void tcr_session_send_keyboard_event(TcrSessionHandle session, int32_t keycode, bool down);


/**
 * @brief 发送触摸屏触摸事件
 *
 * 该接口用于向云端实例发送触摸事件，包括按下、移动、抬起等操作。
 *
 * @param session 会话句柄
 * @param x 触摸点的横坐标（相对于屏幕左上角，单位：像素）
 * @param y 触摸点的纵坐标（相对于屏幕左上角，单位：像素）
 * @param eventType 触摸事件类型，取值参考 Android MotionEvent 事件类型：
 *   - 0: ACTION_DOWN（按下）
 *   - 1: ACTION_MOVE（移动）
 *   - 2: ACTION_UP（抬起）
 * @param width 屏幕宽度（单位：像素），用于坐标归一化
 * @param height 屏幕高度（单位：像素），用于坐标归一化
 * @param timestamp 事件发生的时间戳（单位：毫秒）
 *
 */
TCRSDK_API void tcr_session_touchscreen_touch(TcrSessionHandle session, float x, float y, int eventType, int width, int height, long timestamp);


// ==================== 日志相关接口 ====================

/**
 * @brief 设置日志回调函数，用于接收 SDK 日志
 * @param callback 日志回调结构体指针
 */
TCRSDK_API void tcr_set_log_callback(const TcrLogCallback* callback);

/**
 * @brief 设置日志输出级别
 * @param level 日志级别（如 TRACE、DEBUG、INFO、WARN、ERROR）
 */
TCRSDK_API void tcr_set_log_level(TcrLogLevel level);

/**
 * @brief 获取当前日志输出级别
 * @return 当前日志级别
 */
TCRSDK_API TcrLogLevel tcr_get_log_level();

/**
 * @brief 将视频帧缓冲区转换为I420格式
 * @param buffer 输入的视频帧缓冲区
 * @param output_i420 输出的I420缓冲区指针
 * @return 转换是否成功
 */
TCRSDK_API bool tcr_video_frame_buffer_to_i420(const TcrVideoFrameBuffer* buffer, TcrI420Buffer* output_i420);

/**
 * @brief 检查视频帧缓冲区是否为D3D11格式
 * @param buffer 视频帧缓冲区
 * @return 是否为D3D11格式
 */
TCRSDK_API bool tcr_video_frame_buffer_is_d3d11(const TcrVideoFrameBuffer* buffer);

/**
 * @brief 获取D3D11纹理指针
 * @param buffer 视频帧缓冲区（必须是D3D11格式）
 * @return ID3D11Texture2D指针，失败返回NULL
 */
TCRSDK_API void* tcr_video_frame_buffer_get_d3d11_texture(const TcrVideoFrameBuffer* buffer);

/**
 * @brief 启用本地麦克风
 * @param session 会话句柄
 * @return 是否成功启用，true表示成功
 */
TCRSDK_API bool tcr_session_enable_local_microphone(TcrSessionHandle session);

/**
 * @brief 禁用本地麦克风
 * @param session 会话句柄
 */
TCRSDK_API void tcr_session_disable_local_microphone(TcrSessionHandle session);

/**
 * @brief 检查本地麦克风是否已启用
 * @param session 会话句柄
 * @return 是否已启用，true表示已启用
 */
TCRSDK_API bool tcr_session_is_local_microphone_enabled(TcrSessionHandle session);

#ifdef __cplusplus
}
#endif