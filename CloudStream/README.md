# 云手机SDK集成指引

## 1. 前后端架构概览
站在SDK的角度整体架构由**业务后台**、**云 API**和**客户端 SDK**三个部分组成
- **客户端 SDK**：业务客户端集成 SDK，实现云手机实例的截图展示、实时串流、事件监听等功能。
- **业务后台**: 帮客户端转发CreateAndroidInstancesAccessToken请求以便获取Token及AccessInfo
- **云 API**：业务后台通过 HTTP/RESTful API 实现AccessToken的创建和返回。
![arch](https://cg-sdk-1258344699.cos.ap-nanjing.myqcloud.com/CloudDeviceWinSDK/docs/images/cloud_stream_arch.png)


**业务接入流程：**

1. 业务后台对接云 API。(开发团队会提供业务后台的源码)
2. 业务客户端集成 SDK，完成实例展示、串流和触控、按键事件等交互。

本指南面向需要在 **Windows / macOS / Linux C++ 项目**（如 Qt Quick 工程）中集成云手机 PaaS SDK 的开发者，如果您的客户端使用跨端的Web技术进行开发，请参考[前端JS-SDK接入文档](https://ex.cloud-gaming.myqcloud.com/cloud_gaming_web/docs/index.html)。

为了便于您快速接入 SDK，我们提供了一个基于 **QtQuick** 的 Demo 工程，可以通过[CloudStream_QtQuick_Demo](CloudStream_QtQuick_Demo/README.md)下载和了解 Demo 运行流程。

---
## 2. 工程环境准备

**TcrSdk 为纯 C 接口，无 Qt 依赖，可在任意 C++ 工程中集成**。本文以 Qt Quick 工程为例，介绍典型的集成流程和注意事项。

### 2.1 依赖环境

#### Windows
- 从[TcrSdk发布记录](https://github.com/tencentyun/cloudgame-windows-sdk/blob/main/Docs/Release_Note.md)下载最新版本 Windows SDK（含头文件、lib、dll），并拷贝到 `third_party/TcrSdk/win` 目录
- 若您的工程使用 Visual Studio 开发可以直接集成，若您的客户端是 Qt 工程，您需要确认编译器使用的是 `MSVC 2022 64-bit`

#### macOS
- 从[TcrSdk发布记录](https://github.com/tencentyun/cloudgame-windows-sdk/blob/main/Docs/Release_Note.md)下载最新版本 macOS SDK（Universal Binary，支持 Apple Silicon 与 Intel），并拷贝到 `third_party/TcrSdk/macos` 目录
- 需要 **Xcode 14.0+**、**CMake 3.16+**、**Qt 6.8+**（macOS Kit）
- 应用访问摄像头/麦克风时需在 `Info.plist` 中声明 `NSCameraUsageDescription` / `NSMicrophoneUsageDescription`

#### Linux
- 从[TcrSdk发布记录](https://github.com/tencentyun/cloudgame-windows-sdk/blob/main/Docs/Release_Note.md)下载最新版本 Linux SDK，并拷贝到 `third_party/TcrSdk/linux` 目录
- 前提条件: 当前支持 x86 架构硬件上的 Linux 系统，其中 glibc 2.28+

### 2.2 目录结构建议

```
CloudPhone_QtQuick/
├── src/                   # 业务代码
├── qml/                   # QML 资源
├── shaders/               # 着色器
├── cmake/                 # 平台特定 CMake 配置
│   ├── TcrSdkWindows.cmake
│   ├── TcrSdkMacOS.cmake
│   └── TcrSdkLinux.cmake
├── third_party/
│   └── TcrSdk/
│       ├── include/           # 公共头文件（跨平台）
│       │   ├── tcr_c_api.h
│       │   ├── tcr_types.h
│       │   └── tcr_export.h
│       ├── win/               # Windows 平台
│       │   ├── Release/
│       │   │   ├── x64/       # TcrSdk.dll / TcrSdk.lib / TcrSdk.pdb
│       │   │   └── Win32/
│       │   └── Debug/
│       │       ├── x64/       # TcrSdk.dll / TcrSdk.lib / TcrSdk.pdb
│       │       └── Win32/
│       │
│       ├── linux/             # Linux 平台
│       │   └── lib/           # libTcrSdk.so
│       │
│       └── macos/             # macOS 平台
│           └── lib/           # libTcrSdk.dylib
└── CMakeLists.txt
```

---

## 3. CMake 工程配置

推荐将各平台的 SDK 配置拆分到独立的 cmake 文件中，通过主 `CMakeLists.txt` 按平台引入：

```cmake
set(TCRSDK_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/TcrSdk)

# 添加头文件目录（跨平台共用）
target_include_directories(CloudPhone_QtQuick PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${TCRSDK_BASE_DIR}/include
)

if(WIN32)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/TcrSdkWindows.cmake)
elseif(APPLE)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/TcrSdkMacOS.cmake)
else()
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/TcrSdkLinux.cmake)
endif()
```

各平台 cmake 文件示例如下：

### Windows（cmake/TcrSdkWindows.cmake）

```cmake
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(TCRSDK_ARCH x64)
else()
    set(TCRSDK_ARCH Win32)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(TCRSDK_DIR "${TCRSDK_BASE_DIR}/win/Debug/${TCRSDK_ARCH}")
else()
    set(TCRSDK_DIR "${TCRSDK_BASE_DIR}/win/Release/${TCRSDK_ARCH}")
endif()

target_link_libraries(CloudPhone_QtQuick PRIVATE ${TCRSDK_DIR}/TcrSdk.lib)

# 拷贝 DLL 到输出目录
add_custom_command(TARGET CloudPhone_QtQuick POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${TCRSDK_DIR}/TcrSdk.dll"
        "$<TARGET_FILE_DIR:CloudPhone_QtQuick>"
)
```

### macOS（cmake/TcrSdkMacOS.cmake）

```cmake
set(TCRSDK_DYLIB "${TCRSDK_BASE_DIR}/macos/lib/libTcrSdk.dylib")
target_link_libraries(CloudPhone_QtQuick PRIVATE ${TCRSDK_DYLIB})

# 设置 rpath，让应用在运行时能找到 dylib
set_target_properties(CloudPhone_QtQuick PROPERTIES
    INSTALL_RPATH "@executable_path/../Frameworks"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# 将 dylib 拷贝进 .app 包
add_custom_command(TARGET CloudPhone_QtQuick POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_BUNDLE_DIR:CloudPhone_QtQuick>/Contents/Frameworks"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${TCRSDK_DYLIB}"
        "$<TARGET_BUNDLE_DIR:CloudPhone_QtQuick>/Contents/Frameworks/"
)
```

### Linux（cmake/TcrSdkLinux.cmake）

```cmake
set(TCRSDK_DIR "${TCRSDK_BASE_DIR}/linux")
find_library(TCRSDK_LIB TcrSdk PATHS "${TCRSDK_DIR}/lib" NO_DEFAULT_PATH)

target_link_libraries(CloudPhone_QtQuick PRIVATE ${TCRSDK_LIB})

# 设置 rpath，确保运行时能找到 .so
set_target_properties(CloudPhone_QtQuick PROPERTIES
    INSTALL_RPATH "$ORIGIN;$ORIGIN/../lib"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# 拷贝 .so 到输出目录
add_custom_command(TARGET CloudPhone_QtQuick POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${TCRSDK_DIR}/lib/libTcrSdk.so"
        "$<TARGET_FILE_DIR:CloudPhone_QtQuick>"
)
```

---

## 4. SDK 典型使用流程

### 4.1 初始化 Client

```cpp
#include "tcr_c_api.h"

TcrConfig config = tcr_config_default();
config.token = tokenResult.token.c_str();           // 业务后台获取的 Token
config.accessInfo = tokenResult.accessInfo.c_str(); // 业务后台获取的 AccessInfo

TcrClientHandle tcrClient = tcr_client_get_instance();

// 可选：提前建立连接以优化首帧（在 tcr_client_init 之前某个时机调用）
// tcr_client_prepare(tcrClient);

TcrErrorCode err = tcr_client_init(tcrClient, &config);
if (err != TCR_SUCCESS) {
    // 处理初始化失败
}
```

### 4.2 创建串流会话并连接实例

#### 多实例串流（小流）

```cpp
// 创建会话配置
TcrSessionConfig config = tcr_session_config_default();

// 自定义串流参数
config.stream_profile.video_width = 288;       // 指定短边的宽度（长边根据云机分辨率等比拉伸）
config.stream_profile.fps = 1;                 // 帧率
config.stream_profile.max_bitrate = 100;       // 最大码率 Kbps
config.stream_profile.min_bitrate = 50;        // 最小码率 Kbps
config.enable_audio = false;                   // 禁用音频
config.concurrentStreamingInstances = 10;      // 最大同时拉流实例数（默认1，最大500）

// 创建会话
TcrSessionHandle tcrSession = tcr_client_create_session(tcrClient, &config);

// 连接多个实例，每个实例产生独立的视频流
const char* instances[] = {"cai-xxx-001", "cai-xxx-002", "cai-xxx-003"};
tcr_session_access_multi_stream(tcrSession, instances, 3);

// 连接后可动态切换拉流实例列表
const char* new_streaming[] = {"cai-xxx-002", "cai-xxx-003"};
tcr_session_switch_streaming_instances(tcrSession, new_streaming, 2);

// 暂停/恢复指定实例的出流
const char* pause_ids[] = {"cai-xxx-001"};
tcr_session_pause_streaming(tcrSession, "video", pause_ids, 1);
tcr_session_resume_streaming(tcrSession, "video", pause_ids, 1);
```

##### 在视频帧回调中区分不同实例的流
```cpp
static void VideoFrameCallback(void* user_data, TcrVideoFrameHandle frame_handle) {
    const TcrVideoFrameBuffer* buffer = tcr_video_frame_get_buffer(frame_handle);
    if (!buffer) return;
    const char* instance_id = buffer->instance_id; // 通过 instance_id 区分不同实例
    // 根据 instance_id 处理不同实例的视频帧
}
```

#### 单实例连接（大流）

```cpp
TcrSessionConfig session_config = tcr_session_config_default();
TcrSessionHandle tcrSession = tcr_client_create_session(tcrClient, &session_config);

const char* instanceIds[] = { instanceId.c_str() };
tcr_session_access(tcrSession, instanceIds, 1, false); // false 表示单实例
```

#### 单实例串流并群控（大流）

```cpp
TcrSessionConfig session_config = tcr_session_config_default();
TcrSessionHandle tcrSession = tcr_client_create_session(tcrClient, &session_config);

std::vector<const char*> idPtrs;
for (const auto& id : instanceIds) idPtrs.push_back(id.c_str());
tcr_session_access(tcrSession, idPtrs.data(), idPtrs.size(), true); // true 表示群控
```

> **说明**：如需动态加入群控实例，可通过 `tcr_instance_join_group`。

### 4.3 设置回调

> **重要**：Observer 结构体必须在整个 session 生命周期内保持有效（建议使用 `static` 或成员变量），销毁 session 前必须先将 observer 置空。

#### 视频帧回调

```cpp
static void VideoFrameCallback(void* user_data, TcrVideoFrameHandle frame_handle) {
    // 获取视频帧缓冲区数据
    const TcrVideoFrameBuffer* buffer = tcr_video_frame_get_buffer(frame_handle);
    if (!buffer) return;

    switch (buffer->type) {
        case TCR_VIDEO_BUFFER_TYPE_D3D11:
            // 硬件解码（仅 Windows）：使用 RGBA 格式的 D3D11 纹理渲染
            // SDK 已完成 YUV 到 RGB 转换，可直接作为颜色纹理使用
            render_d3d11_rgba_texture(&buffer->buffer.d3d11);
            break;
        case TCR_VIDEO_BUFFER_TYPE_I420:
            // 软件解码（Windows/macOS/Linux）：使用 I420 数据渲染
            // 需要在 shader 中进行 YUV 到 RGB 的颜色空间转换
            render_i420_buffer(&buffer->buffer.i420);
            break;
    }

    // 如果需要在回调外（如渲染线程）继续使用 frame_handle，需要增加引用计数
    // tcr_video_frame_add_ref(frame_handle);
    // 使用完毕后记得释放：tcr_video_frame_release(frame_handle);
}

// 设置视频帧观察者
static TcrVideoFrameObserver video_observer = tcr_video_frame_observer_default();
video_observer.user_data = this; // 可传 this 指针
video_observer.on_frame = VideoFrameCallback;
tcr_session_set_video_frame_observer(tcrSession, &video_observer);
```

#### 事件回调

```cpp
static void SessionEventCallback(void* user_data, TcrSessionEvent event, const char* eventData) {
    const char* event_name = tcr_session_event_to_string(event);
    printf("Session event: %s, data: %s\n", event_name, eventData ? eventData : "");

    switch (event) {
        case TCR_SESSION_EVENT_STATE_CONNECTED:
            // 会话已连接
            break;
        case TCR_SESSION_EVENT_STATE_CLOSED:
            // 会话已关闭
            break;
        case TCR_SESSION_EVENT_CLIENT_STATS:
            // 性能数据更新（JSON 格式）
            break;
        // 处理其他事件...
    }
}

// 多实例场景下，可通过 on_instance_event 区分各实例的事件(通常不需要使用)
static void InstanceEventCallback(void* user_data, const char* instanceId,
                                  TcrSessionEvent event, const char* eventData) {
    // 根据 instanceId 处理对应实例的事件
}

// 设置会话观察者
static TcrSessionObserver session_observer = tcr_session_observer_default();
session_observer.user_data = this;
session_observer.on_event = SessionEventCallback;
session_observer.on_instance_event = InstanceEventCallback; // 多实例场景下设置
tcr_session_set_observer(tcrSession, &session_observer);
```

### 4.4 操作及交互

#### 实时控制

```cpp
// 模拟返回键
tcr_session_send_keyboard_event(tcrSession, 158, true);  // 按下
tcr_session_send_keyboard_event(tcrSession, 158, false); // 抬起

// 单点触摸（eventType: 0=DOWN, 1=MOVE, 2=UP）
tcr_session_touchscreen_touch(tcrSession, 100, 200, 0, 720, 1280, 0);

// 多点触摸（支持多个触摸点同时操作）
tcr_session_touchscreen_touch_ex(tcrSession, 100, 200, 0, /*finger_id=*/0, 1080, 1920, timestamp, nullptr, 0);
```

#### 麦克风控制

```cpp
// 启用/禁用麦克风（使用系统默认设备）
tcr_session_enable_local_microphone(tcrSession, true);

// 枚举可用麦克风设备
int count = tcr_session_get_microphone_device_count(tcrSession);
for (int i = 0; i < count; i++) {
    TcrMicrophoneDeviceInfo info;
    if (tcr_session_get_microphone_device(tcrSession, i, &info)) {
        printf("设备[%d]: name=%s, id=%s\n", i, info.device_name, info.device_id);
    }
}

// 指定麦克风设备启用（Windows 用 device_id，macOS 用 device_name）
TcrMicrophoneConfig mic_config;
// Windows: 填入 TcrMicrophoneDeviceInfo.device_id
// macOS:   填入 TcrMicrophoneDeviceInfo.device_name
strncpy(mic_config.device_id, selected_device_id, sizeof(mic_config.device_id) - 1);
tcr_session_enable_microphone_with_config(tcrSession, &mic_config);
```

### 4.5 网络探测与连接优化

SDK 提供网络探测能力，支持**主动探测**和**被动探测**两种模式，帮助业务评估各加速节点的网络质量（RTT、抖动、丢包率等），从而选择最优节点以优化连接体验。

#### 主动探测 vs 被动探测

| | 主动探测 | 被动探测 |
|---|---|---|
| **触发方式** | 业务主动调用 `tcr_client_start_probe` | 创建会话时通过 `TcrSessionConfig.enable_passive_probe = true` 开启 |
| **使用时机** | 会话建立前，用于预选最优节点 | 会话串流过程中，持续评估当前连接质量 |
| **是否需要 Session** | 不需要，仅需 Client 初始化完成 | 需要，随会话生命周期自动运行 |
| **结果获取方式** | 通过回调函数返回 `TcrProbeResult` | SDK 内部使用，自动优化连接路径 |
| **适用场景** | 展示网络质量面板、让用户选择节点、指定 `preferred_domain` | 自动感知网络变化并优化 |

#### 探测结果数据结构

```cpp
// 单个节点的探测质量信息
typedef struct TcrProbeNodeInfo {
    const char* zone;         // 节点标识，如 "ap-shanghai-3"
    const char* domain;       // 节点域名
    double rtt_ms;            // 往返延迟 (ms)
    double jitter_ms;         // 抖动 (ms)
    double packet_loss_rate;  // 丢包率 (0.0~1.0)
    double quality_score;     // 综合评分 (0~100，越高越好)
    int64_t connect_time_ms;  // 连接建立耗时 (ms)
} TcrProbeNodeInfo;

// 探测结果
typedef struct TcrProbeResult {
    TcrProbeNodeInfo* nodes;  // 节点数组（按 quality_score 降序排列）
    int node_count;           // 节点数量
    int64_t timestamp_ms;     // 结果生成时间戳
    bool is_ready;            // 是否所有节点都已完成探测
} TcrProbeResult;
```

#### 主动探测

主动探测用于在创建会话前探测各加速节点的网络质量，业务可根据探测结果选择最优节点。

**前提条件：**
- 需先调用 `tcr_client_init` 完成 Client 初始化
- 无需创建 Session 即可发起探测
- 同一时刻只能有一次主动探测在进行

**启动探测：**

```cpp
// 探测结果回调（在 SDK 内部线程触发，注意线程安全）
void OnProbeResult(void* user_data, const TcrProbeResult* result) {
    printf("探测结果: %d 个节点, is_ready=%d\n", result->node_count, result->is_ready);

    for (int i = 0; i < result->node_count; i++) {
        const TcrProbeNodeInfo* node = &result->nodes[i];
        printf("  [%d] zone=%s, domain=%s, rtt=%.1fms, jitter=%.1fms, loss=%.2f%%, score=%.1f\n",
               i, node->zone, node->domain, node->rtt_ms, node->jitter_ms,
               node->packet_loss_rate * 100, node->quality_score);
    }

    // 如需在回调外持有结果，需要克隆
    if (result->is_ready) {
        TcrProbeResult* cloned = tcr_probe_result_clone(result);
        // 将 cloned 传递到其他线程使用...
        // 使用完毕后释放：tcr_probe_result_release(cloned);
    }
}

// 启动探测（首次结果就绪后回调，之后每 2 秒更新一次）
TcrErrorCode err = tcr_client_start_probe(tcrClient, OnProbeResult, nullptr);
if (err != TCR_SUCCESS) {
    // 启动失败（如已在探测中、Client 未初始化）
}
```

**停止探测：**

```cpp
// 停止探测并释放探测连接资源，停止后不再触发回调
tcr_client_stop_probe(tcrClient);
```

**将探测结果应用到会话（指定首选节点）：**

```cpp
// 根据主动探测结果，将最优节点的 domain 设置为会话的首选加速节点
TcrSessionConfig session_config = tcr_session_config_default();
session_config.preferred_domain = best_node_domain;  // 探测结果中 quality_score 最高的节点 domain
TcrSessionHandle tcrSession = tcr_client_create_session(tcrClient, &session_config);
```

#### 被动探测

被动探测在会话串流期间自动运行，SDK 内部持续评估连接质量并自动优化传输路径，业务无需手动处理探测结果。

```cpp
TcrSessionConfig session_config = tcr_session_config_default();
session_config.enable_passive_probe = true;  // 启用被动探测

// 可选：结合主动探测结果指定首选节点
session_config.preferred_domain = "best-node.example.com";

TcrSessionHandle tcrSession = tcr_client_create_session(tcrClient, &session_config);
```

#### 完整使用流程示例

```cpp
// 1. 初始化 Client
TcrClientHandle tcrClient = tcr_client_get_instance();
TcrConfig config = tcr_config_default();
config.token = token;
config.accessInfo = accessInfo;
tcr_client_init(tcrClient, &config);

// 2. 启动主动探测，评估各节点网络质量
const char* best_domain = nullptr;
tcr_client_start_probe(tcrClient, [](void* user_data, const TcrProbeResult* result) {
    if (result->is_ready && result->node_count > 0) {
        // nodes[0] 即为评分最高的节点（数组按 quality_score 降序排列）
        const char** best = (const char**)user_data;
        *best = result->nodes[0].domain;
    }
}, &best_domain);

// 3. 等待探测完成后停止探测
// ...（业务逻辑等待 is_ready=true）
tcr_client_stop_probe(tcrClient);

// 4. 创建会话时应用探测结果，并开启被动探测
TcrSessionConfig session_config = tcr_session_config_default();
session_config.preferred_domain = best_domain;    // 指定最优加速节点
session_config.enable_passive_probe = true;       // 串流期间持续优化连接
TcrSessionHandle tcrSession = tcr_client_create_session(tcrClient, &session_config);

// 5. 连接实例并开始串流
const char* instanceIds[] = {"cai-xxx-001"};
tcr_session_access(tcrSession, instanceIds, 1, false);
```

#### 注意事项

- **主动探测回调线程安全**：回调在 SDK 内部线程触发，如需更新 UI 请切换到主线程
- **探测结果生命周期**：回调中的 `result` 指针仅在回调期间有效，如需持有需调用 `tcr_probe_result_clone()` 克隆，使用完毕后必须调用 `tcr_probe_result_release()` 释放内存
- **带宽开销**：探测会占用一定网络带宽
- **被动探测开销**：被动探测在会话期间自动运行，开销极小，适合长连接场景持续优化
- **preferred_domain 使用**：该字段为可选项，NULL 表示不指定，SDK 会自动选择节点；设置后 SDK 优先连接指定节点

### 4.6 资源释放

```cpp
// 1. 先将 observer 置空，避免悬空回调
tcr_session_set_video_frame_observer(tcrSession, nullptr);
tcr_session_set_observer(tcrSession, nullptr);

// 2. 销毁会话
tcr_client_destroy_session(tcrClient, tcrSession);

// 3. 释放全局资源（程序退出前调用）
tcr_client_release(tcrClient);
```

---

## 5. 注意事项

- **会话管理**：每一个 `TcrSessionHandle` 只能调用一次 `tcr_session_access` 或 `tcr_session_access_multi_stream`，如需多个会话请创建多个实例并释放使用过的实例。
- **日志调试**：您需要通过 `tcr_set_log_callback`、`tcr_set_log_level` 配置日志，以便在出现问题之后将日志反馈给开发团队定位问题。
- **Observer 生命周期**：Observer 结构体需在整个 session 生命周期内保持有效。销毁 session 前必须先通过传入 `nullptr` 将 observer 置空。
- **硬件解码**：硬件解码（`enable_hardware_decode = true`）仅在 Windows D3D11 平台生效，macOS 和 Linux 始终使用软件解码，视频帧回调类型为 `TCR_VIDEO_BUFFER_TYPE_I420`。
- **macOS 权限声明**：应用访问摄像头/麦克风时，需在 `Info.plist` 中声明 `NSCameraUsageDescription` 和 `NSMicrophoneUsageDescription`，否则系统将拒绝访问。
- **macOS dylib rpath**：需将 `libTcrSdk.dylib` 拷贝至 `.app/Contents/Frameworks/` 目录，并配置 `@executable_path/../Frameworks` rpath，否则运行时将无法加载库。
- **Linux 可执行文件输出目录**：Linux 平台下可执行文件输出到 `bin/` 子目录，以避免与 QML 模块的同名目录冲突。
- Demo [CloudStream_QtQuick_Demo](CloudStream_QtQuick_Demo/README.md) 演示了串流场景下SDK能力（截图展示及串流交互等），[SDK接口](https://cloud.tencent.com/document/product/1162/122588) 支持的其他功能为云手机PaaS功能。

---

如有更多问题，请联系开发团队提供技术支持。
