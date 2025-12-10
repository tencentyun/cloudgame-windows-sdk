#ifndef TCRSDK_TCR_TYPES_H_
#define TCRSDK_TCR_TYPES_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tcr 通用错误码枚举，定义了 SDK 各模块的错误返回值。
 *
 * 该枚举用于标识 SDK 操作的结果状态，便于上层应用根据错误码进行相应处理。
 * 每个错误码均有详细的中文说明，便于开发者理解和排查问题。
 */
typedef enum {
    /**
     * @brief 操作成功。
     * 一切正常，操作已成功完成。
     */
    TCR_SUCCESS = 0,

    /**
     * @brief 操作超时。
     * 请求在规定时间内未完成，可能由于网络延迟或服务端无响应导致。
     */
    TCR_ERR_TIMEOUT = 100010,

    /**
     * @brief 参数无效。
     * 传入的参数不符合要求或缺失，需检查调用接口时的参数设置。
     */
    TCR_ERR_INVALID_PARAMS = 100011,

    /**
     * @brief 未知错误。
     * 发生了未被明确分类的异常错误，建议收集日志进一步排查。
     */
    TCR_ERR_UNKNOWN = 100012,

    /**
     * @brief 内部错误。
     * SDK 内部发生异常，通常为不可预期的系统级错误。
     */
    TCR_ERR_INTERNAL_ERROR = 100013,

    /**
     * @brief 状态非法。
     * 当前上下文或对象状态不允许执行该操作，需检查调用时机或状态流转。
     */
    TCR_ERR_STATE_ILLEGAL = 100014,

    ////////////////////////   DataChannel status code   ////////////////////////

    /**
     * @brief 数据通道模块错误码起始值。
     * 用于区分数据通道相关的错误类型。
     */
    TCR_ERR_DATA_CHANNEL_BASE_CODE = 102000,

    /**
     * @brief 数据通道创建失败。
     * 可能由于资源不足、参数错误或底层网络异常导致无法创建数据通道。
     */
    TCR_ERR_CREATE_FAILURE = 102001,

    /**
     * @brief 数据通道已关闭。
     * 当前操作的数据通道已被关闭或已失效，无法继续使用。
     */
    TCR_ERR_CLOSED = 102002,
} TcrErrorCode;

/**
 * @brief SDK类型枚举
 */
typedef enum {
    CloudPhonePaas = 0,  ///< 云手机PaaS类型
    CloudStream = 1      ///< 云串流类型
} TcrSdkType;

/**
 * @brief 会话配置信息, 该信息由业务后台调用云API CreateAndroidInstancesAccessToken 获取
 */
typedef struct {
    const char* token;      ///< 业务后台返回的token
    const char* accessInfo; ///< 业务后台返回的accessInfo
    TcrSdkType sdkType;     ///< SDK类型，默认为CloudStream
    bool streamOpt;         ///< 是否开启串流优化(仅用于测试, 正式版本不可开启)，默认为false
    bool hardwareDecode;    ///< 是否启用硬件解码，默认为false
} TcrConfig;

/**
 * @brief 创建默认的会话配置
 * 
 * 该函数返回一个预设了合理默认值的TcrConfig结构体，
 * 使用者可以在此基础上修改需要自定义的字段。
 * 
 * 默认配置说明：
 * - token: 设为NULL，使用者必须设置此字段
 * - accessInfo: 设为NULL，使用者必须设置此字段
 * - sdkType: SDK类型, 默认为CloudStream
 * - streamOpt: 设为false，默认不开启串流优化
 * - hardwareDecode: 设为false，默认不启用硬件解码
 * 
 * @return 返回带有默认值的TcrConfig结构体
 * 
 * @note 使用示例：
 * @code
 * TcrConfig config = tcr_config_default();
 * config.token = tokenStr.c_str();           // 必须设置：访问令牌
 * config.accessInfo = accessInfoStr.c_str(); // 必须设置：访问信息
 * config.hardwareDecode = true;              // 可选：启用硬件解码
 * @endcode
 */
static inline TcrConfig tcr_config_default(void) {
    TcrConfig config;
    
    // token和accessInfo必须由使用者设置
    config.token = NULL;
    config.accessInfo = NULL;
    
    // SDK类型默认为CloudStream（云串流）
    config.sdkType = CloudStream;
    
    // 默认不开启串流优化
    config.streamOpt = false;
    
    // 默认不启用硬件解码
    config.hardwareDecode = false;
    
    return config;
}

/**
 * @brief I420(YUV420P)格式的视频帧缓冲区
 * 常用的YUV格式，包含亮度(Y)和色度(U,V)分量
 * 
 * @note 该格式在以下情况下使用：
 * 1. 配置中禁用硬件解码（enable_hardware_decode = false）
 * 2. 启用硬件解码但设备不支持，SDK自动回退到软件解码
 * 3. 硬件解码过程中出现错误，临时回退到软件解码
 */
typedef struct {
    const uint8_t* data_y;    ///< 亮度(Y)分量数据指针
    const uint8_t* data_u;    ///< 色度(U)分量数据指针
    const uint8_t* data_v;    ///< 色度(V)分量数据指针
    int32_t stride_y;         ///< Y分量的行跨度(字节数)
    int32_t stride_u;         ///< U分量的行跨度(字节数)
    int32_t stride_v;         ///< V分量的行跨度(字节数)
    int32_t width;            ///< 视频帧的宽度(像素)
    int32_t height;           ///< 视频帧的高度(像素)
} TcrI420Buffer;

/**
 * @brief 视频缓冲区类型枚举
 * 
 * 该枚举定义了视频帧回调时可能的缓冲区类型，类型由解码方式决定：
 * - 硬件解码成功时返回D3D11类型
 * - 软件解码或硬件解码回退时返回I420类型
 */
typedef enum {
    TCR_VIDEO_BUFFER_TYPE_I420 = 0,   ///< I420(YUV420P)格式的CPU缓冲区，用于软件解码
    TCR_VIDEO_BUFFER_TYPE_D3D11 = 1   ///< D3D11格式的GPU纹理缓冲区，用于硬件解码
} TcrVideoBufferType;

/**
 * @brief D3D11视频帧缓冲区
 * 使用Direct3D 11的GPU纹理格式，适用于硬件解码场景
 * 
 * @note 该格式仅在硬件解码成功时使用。如果设备不支持硬件解码，
 *       SDK会自动回退到软件解码，此时视频帧将以TcrI420Buffer格式回调。
 *       因此，应用程序需要同时实现D3D11和I420两种格式的渲染逻辑。
 * 
 * @note 纹理格式说明：
 *       SDK内部已将硬件解码器输出的NV12格式转换为标准的RGBA格式，
 *       因此format字段固定为DXGI_FORMAT_R8G8B8A8_UNORM (值为28)。
 *       这意味着：
 *       - 无需在shader中进行YUV到RGB的颜色空间转换
 *       - 可以直接作为标准颜色纹理使用，渲染实现简单
 *       - 纹理通道顺序为：红(R) - 绿(G) - 蓝(B) - 透明(A)
 *       - 每个通道为8位无符号归一化整数 (0-255映射到0.0-1.0)
 * 
 * @note 使用说明：
 * 1. texture指针指向ID3D11Texture2D对象，可直接用于GPU渲染
 * 2. device指针指向ID3D11Device对象，用于创建渲染资源
 * 3. array_index指定纹理数组中的索引（如果纹理是Texture2DArray）
 * 4. 纹理数据位于GPU显存中，避免了CPU-GPU数据传输开销
 * 5. 如需CPU访问，需要创建staging texture并执行CopyResource
 * 6. **纹理生命周期管理**：
 *    - 回调中收到的frame默认在回调结束后释放
 *    - 如需延长生命周期，调用 tcr_video_frame_add_ref(frame) 增加引用计数
 *    - 使用完毕后必须调用 tcr_video_frame_release(frame) 释放引用
 *    - 引用计数归零时，纹理资源会自动释放
 * 
 * @warning 跨线程使用时需要注意D3D11的线程安全性
 * 
 * @example 渲染示例
 * @code
 * void on_video_frame(void* user_data, const TcrVideoFrame* frame) {
 *     if (frame->frame_buffer.type == TCR_VIDEO_BUFFER_TYPE_D3D11) {
 *         // 增加引用计数
 *         tcr_video_frame_add_ref((TcrVideoFrame*)frame);
 *         render_queue_push(frame);
 *     }
 * }
 * 
 * void render_thread() {
 *     TcrVideoFrame* frame = render_queue_pop();
 *     const TcrD3D11Buffer* d3d11 = &frame->frame_buffer.buffer.d3d11;
 *     
 *     // 获取D3D11资源
 *     ID3D11Device* device = (ID3D11Device*)d3d11->device;
 *     ID3D11Texture2D* texture = (ID3D11Texture2D*)d3d11->texture;
 *     
 *     // 创建Shader Resource View用于渲染
 *     // 注意：format固定为DXGI_FORMAT_R8G8B8A8_UNORM
 *     ID3D11ShaderResourceView* srv = nullptr;
 *     D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
 *     srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // SDK保证为RGBA格式
 *     srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
 *     srv_desc.Texture2DArray.MipLevels = 1;
 *     srv_desc.Texture2DArray.ArraySize = 1;
 *     srv_desc.Texture2DArray.FirstArraySlice = d3d11->array_index;  // 使用array_index
 *     device->CreateShaderResourceView(texture, &srv_desc, &srv);
 *     
 *     // 使用标准的RGBA纹理shader进行渲染
 *     // 在shader中可以直接采样：vec4 color = texture(sampler, texCoord);
 *     render_rgba_texture(srv);
 *     
 *     // 清理
 *     srv->Release();
 *     tcr_video_frame_release(frame);
 * }
 * @endcode
 */
typedef struct {
    void* texture;            ///< 转换为ID3D11Texture2D*使用
    void* device;             ///< ID3D11Device指针，用于创建渲染资源
    int32_t width;            ///< 视频帧的宽度(像素)
    int32_t height;           ///< 视频帧的高度(像素)
    int32_t array_index;      ///< 纹理数组索引（如果纹理是Texture2DArray，通常为0表示单个纹理）
    int32_t format;           ///< 纹理格式(DXGI_FORMAT枚举值)
                              ///< SDK保证该值固定为 28 (DXGI_FORMAT_R8G8B8A8_UNORM)
                              ///< 表示标准的RGBA格式，每通道8位，通道顺序为R-G-B-A
                              ///< 
                              ///< @note SDK内部实现说明（使用者无需关心）：
                              ///< 硬件解码器输出的NV12格式已通过GPU加速转换为RGBA格式，
                              ///< 转换使用D3D11 VideoProcessor实现，性能开销极小
} TcrD3D11Buffer;

/**
 * @brief 视频旋转角度枚举
 */
typedef enum {
    TCR_VIDEO_ROTATION_0 = 0,    ///< 不旋转(0度)
    TCR_VIDEO_ROTATION_90 = 90,  ///< 顺时针旋转90度
    TCR_VIDEO_ROTATION_180 = 180,///< 旋转180度
    TCR_VIDEO_ROTATION_270 = 270 ///< 顺时针旋转270度
} TcrVideoRotation;

/**
 * @brief 通用视频帧缓冲区联合体
 * 支持多种视频缓冲区类型
 * 
 * @note 使用说明：
 * 1. 必须先检查type字段，确定实际的缓冲区类型
 * 2. 根据type访问对应的buffer成员：
 *    - type == TCR_VIDEO_BUFFER_TYPE_I420: 使用buffer.i420
 *    - type == TCR_VIDEO_BUFFER_TYPE_D3D11: 使用buffer.d3d11
 * 3. 即使配置中启用了硬件解码，也可能因设备不支持而回退到I420格式
 * 4. 渲染实现需要同时支持两种格式的处理
 * 
 * @example 视频帧处理示例
 * @code
 * void on_video_frame(void* user_data, const TcrVideoFrame* frame) {
 *     switch (frame->frame_buffer.type) {
 *         case TCR_VIDEO_BUFFER_TYPE_D3D11:
 *             // 硬件解码：使用RGBA格式的D3D11纹理渲染
 *             // SDK已完成YUV到RGB转换，可直接作为颜色纹理使用
 *             render_d3d11_rgba_texture(&frame->frame_buffer.buffer.d3d11);
 *             break;
 *         case TCR_VIDEO_BUFFER_TYPE_I420:
 *             // 软件解码：使用I420数据渲染
 *             // 需要在shader中进行YUV到RGB的颜色空间转换
 *             render_i420_buffer(&frame->frame_buffer.buffer.i420);
 *             break;
 *     }
 * }
 * @endcode
 */
typedef struct TcrVideoFrameBuffer {
    TcrVideoBufferType type;    ///< 缓冲区类型
    union {
        TcrI420Buffer i420;    ///< I420格式缓冲区
        TcrD3D11Buffer d3d11;  ///< D3D11格式缓冲区
    } buffer;                  ///< 具体缓冲区数据
    int64_t timestamp_us;      ///< 视频帧时间戳(微秒)
    TcrVideoRotation rotation; ///< 该字段暂不支持
    const char* instance_id;   ///< 实例ID
    int instance_index;        ///< 实例索引，在非群控模式下表示当前视频流对应instanceIds数组中的第几路(从0开始)，用于区分相同ID的多路视频流
} TcrVideoFrameBuffer;

/**
 * @brief 视频帧结构体
 */
typedef struct {
    TcrVideoFrameBuffer frame_buffer;///< 通用视频帧缓冲区
    int64_t timestamp_us;            ///< 视频帧时间戳(微秒)
    TcrVideoRotation rotation;       ///< 视频帧需要旋转的角度
} TcrVideoFrame;

/**
 * @brief 拉流参数配置结构体
 * 用于设置视频流的参数配置。可以单独指定视频宽高，或者帧率，或者码率。
 * 设定码率时必须同时指定min_bitrate、max_bitrate，否则不生效。
 */
typedef struct {
    int32_t video_width;    ///< 视频宽度（像素）, 0表示不指定视频宽度
    int32_t video_height;   ///< 视频高度（像素）, 0表示不指定视频高度
    int32_t fps;            ///< 帧率, 0表示不指定帧率
    int32_t max_bitrate;    ///< 最大码率Kbps, 0表示不指定最大码率
    int32_t min_bitrate;    ///< 最小码率Kbps, 0表示不指定最小码率
} TcrStreamProfile;

/**
 * @brief 会话配置结构体
 * 用于配置会话的各种参数
 */
typedef struct {
    TcrStreamProfile stream_profile;  ///< 拉流参数配置
    const char* user_id;              ///< 用户ID，用于标识当前会话的用户
    bool enable_audio;                ///< 是否启用音频功能，默认启用, 若不需要音频功能建议关闭以降低cpu使用率
    bool enable_hardware_decode;      ///< 是否启用硬件解码，默认启用
                                      ///< 启用后的行为说明：
                                      ///< - 硬件解码成功：视频帧回调类型为TCR_VIDEO_BUFFER_TYPE_D3D11
                                      ///<   纹理格式为DXGI_FORMAT_R8G8B8A8_UNORM (RGBA格式)
                                      ///< - 硬件解码失败或设备不支持：SDK会自动回退到软件解码
                                      ///<   视频帧回调类型为TCR_VIDEO_BUFFER_TYPE_I420
                                      ///< - 禁用硬件解码：视频帧回调类型始终为TCR_VIDEO_BUFFER_TYPE_I420
                                      ///< 
                                      ///< 渲染时必须根据frame_buffer.type字段判断实际的缓冲区类型：
                                      ///< - D3D11类型：使用标准RGBA纹理shader渲染，无需YUV转换
                                      ///< - I420类型：需要在shader中进行YUV到RGB的颜色空间转换
} TcrSessionConfig;

/**
 * @brief 创建默认的会话配置
 * 
 * 该函数返回一个预设了合理默认值的TcrSessionConfig结构体，
 * 使用者可以在此基础上修改需要自定义的字段。
 * 
 * 默认配置说明：
 * - stream_profile: 所有参数设为0，表示不指定具体的流参数
 * - user_id: 设为NULL，使用者需要根据实际情况设置
 * - enable_audio: 设为true，默认启用音频功能
 * 
 * @return 返回带有默认值的TcrSessionConfig结构体
 * 
 * @note 使用示例：
 * @code
 * TcrSessionConfig config = tcr_session_config_default();
 * config.user_id = "user123";  // 设置用户ID
 * config.stream_profile.video_width = 1280;  // 可选：自定义视频宽度
 * config.stream_profile.video_height = 720;  // 可选：自定义视频高度
 * @endcode
 */
static inline TcrSessionConfig tcr_session_config_default(void) {
    TcrSessionConfig config;
    
    // 初始化流配置参数
    // 所有参数设为0表示不指定，由服务端根据实际情况决定
    config.stream_profile.video_width = 0;
    config.stream_profile.video_height = 0;
    config.stream_profile.fps = 0;
    config.stream_profile.max_bitrate = 0;
    config.stream_profile.min_bitrate = 0;
    
    // 用户ID默认为NULL，使用者必须设置此字段
    config.user_id = NULL;
    
    // 默认启用音频功能
    config.enable_audio = true;
    
    // 默认启用硬件解码（性能更好，但会根据设备能力自动回退）
    // 注意：即使启用，SDK也会在检测到设备不支持时自动回退到软件解码
    config.enable_hardware_decode = false;
    
    return config;
}

/**
 * @brief 所有文件上传进度回调函数指针定义。
 *
 * @param instance_id 当前操作的实例ID。
 * @param total_bytes 文件总大小（字节）。
 * @param transferred_bytes 已传输的大小（字节）。
 * @param user_data 用户自定义数据指针。
 */
typedef void (*TcrUploadProgressCallback)(
    const char* instance_id,
    double total_bytes,
    double transferred_bytes,
    void* user_data);

/**
 * @brief 上传回调结构体，包含回调函数和用户数据。
 */
typedef struct {
    TcrUploadProgressCallback on_progress;
    void* user_data;
} TcrUploadCallback;

/**
 * @brief 上行视频配置结构体
 */
typedef struct TcrVideoConfig {
    int width;              // 视频宽度  这里只是期望的宽高，实际取决于摄像头支持的情况
    int height;             // 视频高度
    int max_fps;            // 最大帧率
    int max_bitrate_bps;    // 最大码率
    const char* device_id;  // 摄像头设备ID，为空则使用默认摄像头
} TcrVideoConfig;

/**
 * @brief 创建默认的上行视频配置
 * 
 * 该函数返回一个预设了合理默认值的TcrVideoConfig结构体，
 * 使用者可以在此基础上修改需要自定义的字段。
 * 
 * 默认配置说明：
 * - width: 设为640，默认视频宽度
 * - height: 设为480，默认视频高度
 * - max_fps: 设为30，默认最大帧率
 * - max_bitrate_bps: 设为1000000 (1Mbps)，默认最大码率
 * - device_id: 设为NULL，使用默认摄像头
 * 
 * @return 返回带有默认值的TcrVideoConfig结构体
 * 
 * @note 使用示例：
 * @code
 * TcrVideoConfig config = tcr_video_config_default();
 * config.width = 1280;           // 可选：自定义视频宽度
 * config.height = 720;           // 可选：自定义视频高度
 * config.device_id = "camera1";  // 可选：指定摄像头设备ID
 * @endcode
 */
static inline TcrVideoConfig tcr_video_config_default(void) {
    TcrVideoConfig config;
    
    // 默认视频分辨率：640x480 (VGA)
    config.width = 640;
    config.height = 480;
    
    // 默认最大帧率：30fps
    config.max_fps = 30;
    
    // 默认最大码率：1Mbps
    config.max_bitrate_bps = 1000000;
    
    // 默认使用系统默认摄像头
    config.device_id = NULL;
    
    return config;
}

/**
 * 摄像头能力结构体
 */
typedef struct TcrVideoCapability {
    int width;      // 视频宽度
    int height;     // 视频高度
} TcrVideoCapability;

/**
 * 摄像头设备详细信息
 */
typedef struct TcrCameraDeviceInfo {
    char device_name[256];  // 设备名称
    char device_id[256];    // 设备ID
    TcrVideoCapability capabilities[16]; // 支持的分辨率能力（最多16个）
    int capability_count;   // 能力数量
} TcrCameraDeviceInfo;

/**
 * @brief 自定义数据通道观察者结构体
 */
typedef struct TcrDataChannelObserver {
    void* user_data;
    void (*on_connected)(void* user_data, const int32_t port);
    void (*on_error)(void* user_data, const int32_t port, const TcrErrorCode& code, const char* msg);
    void (*on_message)(void* user_data, const int32_t port, const uint8_t* data, size_t size);
} TcrDataChannelObserver;

/**
 * @brief 创建默认的数据通道观察者配置
 * 
 * 该函数返回一个预设了合理默认值的TcrDataChannelObserver结构体，
 * 所有回调函数指针初始化为NULL，使用者可以在此基础上设置需要的回调。
 * 
 * 默认配置说明：
 * - user_data: 设为NULL，使用者可根据需要设置
 * - on_connected: 设为NULL，使用者可设置此回调以接收连接成功事件
 * - on_error: 设为NULL，使用者可设置此回调以接收错误事件
 * - on_message: 设为NULL，使用者需要设置此回调以接收数据消息
 * 
 * @return 返回带有默认值的TcrDataChannelObserver结构体
 * 
 * @note 使用示例：
 * @code
 * // 创建默认数据通道观察者配置
 * TcrDataChannelObserver observer = tcr_data_channel_observer_default();
 * observer.user_data = this;
 * observer.on_connected = OnDataChannelConnected;
 * observer.on_error = OnDataChannelError;
 * observer.on_message = OnDataChannelMessage;
 * 
 * data_channel_ = tcr_session_create_data_channel(tcr_session_, 8080, &observer);
 * @endcode
 */
static inline TcrDataChannelObserver tcr_data_channel_observer_default(void) {
    TcrDataChannelObserver observer;
    observer.user_data = NULL;
    observer.on_connected = NULL;
    observer.on_error = NULL;
    observer.on_message = NULL;
    return observer;
}

/**
 * @brief 流媒体请求项结构体
 * 用于多用户流媒体请求
 */
typedef struct {
    const char* instanceId; ///< 实例ID
    const char* status;     ///< 状态："open" 或 "close"
} TcrStreamingRequestItem;

/**
 * @brief 会话事件类型枚举，定义了客户端与云端会话过程中可能产生的所有事件类型。
 */
typedef enum {
    /**
     * @brief 未定义事件类型。
     */
    TCR_SESSION_EVENT_UNDEFINED = -1,
    /**
     * @brief 会话已初始化。
     * @note 状态已废弃。
     */
    TCR_SESSION_EVENT_STATE_INITED = 0,

    /**
     * @brief 会话已连接。
     */
    TCR_SESSION_EVENT_STATE_CONNECTED = 1,

    /**
     * @brief 会话已关闭，无法再使用。
     * 事件数据类型字符串，表示关闭原因。
     */
    TCR_SESSION_EVENT_STATE_CLOSED = 2,

    /**
     * @brief 性能数据更新。
     * 事件数据类型JSON格式字符串：
     * @code{.json}
     * {
     *      "audio_packet_recv":number,    // 接收到的音频包数量
     *      "audio_packet_sent":number,    // 发送的音频包数量
     *      "bitrate":number,              // 接收到的音视频码率
     *      "fps":number,                  // 视频接收帧率
     *      "fps_sent":number,             // 视频发送帧率
     *      "frame_decode":number,         // 解码帧数
     *      "frame_drop":number,           // 丢帧数
     *      "frame_encoded":number,        // 编码帧数
     *      "frame_recv":number,           // 接收帧数
     *      "frame_recv_res":string,       // 视频接收分辨率(如"1080x2408")
     *      "frame_sent":number,           // 视频发送帧数
     *      "nack_count":number,           // NACK重传请求数量
     *      "rtt":number,                  // 客户端到云机实例的往返时延RTT(毫秒)
     *      "raw_rtt":number,              // 客户端到边缘服务器的往返时延RTT(毫秒)
     *      "video_packet_lost":number,    // 视频丢包数
     *      "video_packet_recv":number,    // 接收到的视频包数量
     *      "video_packet_sent":number     // 发送的视频包数量
     * }
     * @endcode
     */
    TCR_SESSION_EVENT_CLIENT_STATS = 3,

    /**
     * @brief 云端屏幕配置变更。
     * 事件数据类型JSON格式字符串：
     * @code{.json}
     * {
     *      "degree":string, // 云端设备的旋转角度, 取值(字符串):  0_degree, 90_degree, 180_degree, 270_degree
     *      "width":number,  // 屏幕的宽度
     *      "height":number  // 屏幕的高度
     * }
     * @endcode
     */
    TCR_SESSION_EVENT_SCREEN_CONFIG_CHANGE = 4,

    /**
     * @brief 请求云端流媒体推流结果(调用tcr_session_request_stream函数)。
     * 事件数据类型JSON格式字符串：
     * @code{.json}
     * {
     *     "user":string,    // 请求的实例ID
     *     "code":int,       // 0表示成功
     *     "status":string   // 请求后的状态信息(成功或失败的具体信息)
     * }
     * @endcode
     */
    TCR_SESSION_EVENT_REQUEST_STREAMING_STATUS = 5,

    /**
     * @brief 远端输入框状态变更。
     * 事件数据类型JSON格式字符串：
     * @code{.json}
     * {
     *     "field_type": string,  // 输入框类型: "normal_input" (点击输入框) 或 "unfocused" (输入框失去焦点)
     *     "text": string         // 输入框的文本内容
     * }
     * @endcode
     */
    TCR_SESSION_EVENT_HIT_INPUT = 6,

    /**
     * @brief 摄像头状态变更。
     * 事件数据类型JSON格式字符串：
     * @code{.json}
     * {
     *     "status": string,  // 摄像头状态取值(字符串): "open_back" (开启后置摄像头), 
     *                                                "open_front" (开启前置摄像头), 
     *                                                "close" (关闭摄像头)
     *     "height": number,  // 视频高度
     *     "width": number    // 视频宽度
     * }
     * @endcode
     */
    TCR_SESSION_EVENT_CAMERA_STATUS = 7,

    /**
     * @brief 接收到云端应用的透传消息。
     * 事件数据类型JSON格式字符串：
     * @code{.json}
     * {
     *      "package_name":string, // 应用包名
     *      "msg":string           // 消息内容
     * }
     * @endcode
     */
    TCR_SESSION_EVENT_TRANSMISSION_MESSAGE = 8,

    /**
     * @brief 云端系统资源使用率信息。
     * 事件数据类型JSON格式字符串：
     * @code{.json}
     * {
     *      "cpu_usage":number, // CPU使用率
     *      "mem_usage":number, // 内存使用率
     *      "gpu_usage":number  // GPU使用率
     * }
     * @endcode
     */
    TCR_SESSION_EVENT_SYSTEM_USAGE = 9,

    /**
     * @brief 云端剪贴板内容变更。
     * 事件数据类型JSON格式字符串：
     * @code{.json}
     * {
     *      "text":string // 剪贴板的最新内容
     * }
     * @endcode
     */
    TCR_SESSION_EVENT_CLIPBOARD_EVENT = 10,

    /**
     * @brief 云端设备通知栏新通知事件。
     * 事件数据类型JSON格式字符串：
     * @code{.json}
     * {
     *      "package_name":string,  // 应用包名
     *      "title":string,         // 标题
     *      "text":string           // 消息内容
     * }
     * @endcode
     */
    TCR_SESSION_EVENT_NOTIFICATION_EVENT = 11,

    /**
     * @brief 云端输入法状态变更。
     * 事件数据类型JSON格式字符串：
     * @code{.json}
     * {
     *     "ime_type": string  // "cloud"表示使用云端输入法，"local"表示使用本地输入法
     * }
     * @endcode
     */
    TCR_SESSION_EVENT_IME_STATUS_CHANGE = 12,

    /**
     * @brief 云端桌面屏幕信息。
     * 事件数据类型JSON格式字符串：
     * @code{.json}
     * {
     *     "screen_width": number,   // 屏幕宽度(像素)
     *     "screen_height": number,  // 屏幕高度(像素)
     *     "screen_left": number,    // 屏幕左边界坐标
     *     "screen_top": number      // 屏幕上边界坐标
     * }
     * @endcode
     * 
     * @note 该事件常用于云端桌面场景。
     *       如果是云手机场景，请使用 TCR_SESSION_EVENT_SCREEN_CONFIG_CHANGE 事件。
     */
    TCR_SESSION_EVENT_REMOTE_DESKTOP_INFO = 13
} TcrSessionEvent;

/**
 * @brief 日志级别
 */
typedef enum TcrLogLevel {
    TCR_LOG_LEVEL_TRACE = 0,
    TCR_LOG_LEVEL_DEBUG = 1,
    TCR_LOG_LEVEL_INFO  = 2,
    TCR_LOG_LEVEL_WARN  = 3,
    TCR_LOG_LEVEL_ERROR = 4
} TcrLogLevel;

/**
 * @brief 日志回调
 */
typedef struct TcrLogCallback {
    void* user_data;
    void (*on_log)(void* user_data, TcrLogLevel level, const char* tag, const char* log);
} TcrLogCallback;

#ifdef __cplusplus
}
#endif

#endif // TCRSDK_TCR_TYPES_H_