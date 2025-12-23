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

本指南面向需要在**Windows C++项目**（如 Qt Quick 工程）中集成云手机 PaaS Windows SDK 的开发者，如果您的客户端使用跨端的Web技术进行开发，请参考[前端JS-SDK接入文档](https://ex.cloud-gaming.myqcloud.com/cloud_gaming_web/docs/index.html)。

为了便于您快速接入Windows SDK，我们提供了一个基于**QtQuick**的Demo工程，可以通过[CloudStream_QtQuick_Demo](CloudStream_QtQuick_Demo/README.md)下载和了解Demo运行流程。

---
## 2. 工程环境准备

**TcrSdk为纯 C 接口，无 Qt 依赖，可在任意 C++ 工程中集成**。本文以 Qt Quick 工程为例，介绍典型的集成流程和注意事项。

### 2.1 依赖环境

- 从[TcrSdk发布记录](https://github.com/tencentyun/cloudgame-windows-sdk/blob/main/Docs/Release_Note.md)下载最新版本SDK(含头文件、lib、dll), 并拷贝到`third_party/TcrSdk` 目录
- 若您的工程使用Visual Studio开发可以直接集成，若您的客户端是Qt工程，您需要确认编译器使用的是`MSVC 2022 64-bit`

### 2.2 目录结构建议

```
CloudPhone_QtQuick/
├── src/                   # 业务代码
├── qml/                   # QML 资源
├── shaders/               # 着色器
├── third_party/
│   └── TcrSdk/
│       ├── Release/
│       └── Debug/
└── CMakeLists.txt
```

---

## 3. CMake 工程配置

参考如下 CMake 配置片段，确保 SDK 头文件、lib、dll 正确集成:


```cmake
# 源码与头文件
target_include_directories(CloudPhone_QtQuick PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/TcrSdk/Release/include
)

# TcrSdk 库路径（以Release为例）
set(TCRSDK_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/TcrSdk)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(TCRSDK_PLATFORM x64)
else()
    set(TCRSDK_PLATFORM Win32)
endif()
set(TCRSDK_DLL "${TCRSDK_BASE_DIR}/Release/${TCRSDK_PLATFORM}/TcrSdk.dll")
set(TCRSDK_LIB "${TCRSDK_BASE_DIR}/Release/${TCRSDK_PLATFORM}/TcrSdk.lib")

target_link_libraries(CloudPhone_QtQuick PRIVATE ${TCRSDK_LIB})

# 拷贝 DLL 到输出目录
add_custom_command(TARGET CloudPhone_QtQuick POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${TCRSDK_DLL}
        $<TARGET_FILE_DIR:CloudPhone_QtQuick>
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
TcrErrorCode err = tcr_client_init(tcrClient, &config);

if (err != TCR_SUCCESS) {
    // 处理初始化失败
}
```

### 4.2 创建串流会话并连接实例

#### 多实例串流(小流)

```cpp
// 创建会话配置
TcrSessionConfig config = tcr_session_config_default();

// 自定义串流参数
config.stream_profile.video_width = 288;   // 指定短边的宽度(长边根据云机分辨率等比拉伸)
config.stream_profile.fps = 1;             // 帧率
config.stream_profile.max_bitrate = 100;   // 最大码率
config.stream_profile.min_bitrate = 50;    // 最小码率
config.enable_audio = false;               // 禁用音频

// 创建会话
TcrSessionHandle tcrSession = tcr_client_create_session(tcrClient, &config);

// 连接多个实例，每个实例产生独立的视频流
const char* instances[] = {"cai-xxx-001", "cai-xxx-002", "cai-xxx-003"};
const char* default_streaming[] = {"cai-xxx-001"};  // 指定默认出流实例
tcr_session_access_multi_stream(tcrSession, instances, 3, default_streaming, 1);

// 根据需要控制其他实例的出流
const char* streaming_instances[] = {"cai-xxx-002"};
tcr_session_resume_streaming(tcrSession, "video", streaming_instances, 1);
```

##### 在视频帧回调中区分不同实例的流
```cpp
void VideoFrameCallback(void* user_data, TcrVideoFrameHandle frame_handle) {
    const TcrVideoFrameBuffer* buffer = tcr_video_frame_get_buffer(frame_handle);
    const char* instance_id = buffer->instance_id;
    // 根据instance_id处理不同实例的视频帧
}
```

#### 单实例连接(大流)

```cpp
// 创建默认会话配置
TcrSessionConfig session_config = tcr_session_config_default();

// 创建会话
TcrSessionHandle tcrSession = tcr_client_create_session(tcrClient, &session_config);

// 连接单个实例
const char* instanceIds[] = { instanceId.c_str() };
tcr_session_access(tcrSession, instanceIds, 1, false); // false 表示单实例
```

#### 单实例串流并群控(大流)

```cpp
// 创建会话配置
TcrSessionConfig session_config = tcr_session_config_default();

// 创建会话
TcrSessionHandle tcrSession = tcr_client_create_session(tcrClient, &session_config);

// 连接多个实例进行群控
std::vector<const char*> idPtrs;
for (const auto& id : instanceIds) idPtrs.push_back(id.c_str());
tcr_session_access(tcrSession, idPtrs.data(), idPtrs.size(), true); // true 表示群控
```

> **说明**：如需动态加入群控实例，可通过 `tcr_instance_join_group`。

### 4.3 设置回调

#### 视频帧回调

```cpp
static void VideoFrameCallback(void* user_data, TcrVideoFrameHandle frame_handle) {
    // 获取视频帧缓冲区数据
    const TcrVideoFrameBuffer* buffer = tcr_video_frame_get_buffer(frame_handle);
    if (!buffer) return;
    
    // 根据缓冲区类型处理视频帧
    switch (buffer->type) {
        case TCR_VIDEO_BUFFER_TYPE_D3D11:
            // 硬件解码：使用RGBA格式的D3D11纹理渲染
            // SDK已完成YUV到RGB转换，可直接作为颜色纹理使用
            render_d3d11_rgba_texture(&buffer->buffer.d3d11);
            break;
        case TCR_VIDEO_BUFFER_TYPE_I420:
            // 软件解码：使用I420数据渲染
            // 需要在shader中进行YUV到RGB的颜色空间转换
            render_i420_buffer(&buffer->buffer.i420);
            break;
    }
    
    // 如果需要在回调外使用frame_handle，需要增加引用计数
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
    // 处理会话事件
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
            // 性能数据更新（JSON格式）
            break;
        // 处理其他事件...
    }
}

// 设置会话观察者
static TcrSessionObserver session_observer = tcr_session_observer_default();
session_observer.user_data = this; // 可传 this 指针
session_observer.on_event = SessionEventCallback;
tcr_session_set_observer(tcrSession, &session_observer);
```

### 4.4 操作及交互

#### 实时控制

```cpp
// 模拟返回键
tcr_session_send_keyboard_event(tcrSession, 158, true);  // 按下
tcr_session_send_keyboard_event(tcrSession, 158, false); // 抬起

// 触摸事件（按下）
tcr_session_touchscreen_touch(tcrSession, 100, 200, 0, 720, 1280, 0); // eventType: 0=DOWN, 1=MOVE, 2=UP
```
### 4.5 资源释放

每一个创建的会话实例在使用完成后必须释放：

```cpp
// 清理回调
tcr_session_set_video_frame_observer(tcrSession, NULL);
tcr_session_set_observer(tcrSession, NULL);

// 销毁会话
tcr_client_destroy_session(tcrClient, tcrSession);
```

---

## 5. 注意事项

- 会话管理: 每一个`TcrClientHandle`只能调用一次`tcr_session_access`或`tcr_session_access_multi_stream`, 如需多个会话请创建多个实例并释放使用过的实例。
- **日志调试**：您需要通过 `tcr_set_log_callback`、`tcr_set_log_level` 配置日志，以便在出现问题之后将日志反馈给开发团队定位问题。
- Demo [CloudStream_QtQuick_Demo](CloudStream_QtQuick_Demo/README.md) 演示了串流场景下SDK能力(截图展示及串流交互等)，[SDK接口](https://cloud.tencent.com/document/product/1162/122588) 支持的其他功能为云手机Paas功能。

---

如有更多问题，请联系开发团队提供技术支持。