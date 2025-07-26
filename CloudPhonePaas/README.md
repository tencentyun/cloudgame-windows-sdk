# 云手机SDK集成指引

## 1. 云手机 PaaS 架构概览

云手机 PaaS 由**云 API**和**客户端 SDK**两大部分组成：

- **云 API**：业务后台通过 HTTP/RESTful API 实现云手机实例、镜像、应用、命令、文件分发、运维调试等管理功能。
- **客户端 SDK**：业务客户端集成 SDK，实现云手机实例的截图展示、批量控制、实时串流、事件监听等功能。
![arch](https://cg-sdk-1258344699.cos.ap-nanjing.myqcloud.com/CloudDeviceWinSDK/docs/images/cloud_phone_arch.png)
**业务接入流程：**

1. 业务后台对接云 API，完成镜像、实例、应用等管理。
2. 业务客户端集成 SDK，完成实例展示、批量控制、串流和触控、按键事件等交互。

业务后台接入请参考[后台接入文档](https://cloud.tencent.com/document/product/1162/113726)，本指南面向需要在**Windows C++项目**（如 Qt Quick 工程）中集成云手机 PaaS Windows SDK 的开发者，如果您的客户端使用跨端的Web技术进行开发，请参考[前端JS-SDK接入文档](https://ex.cloud-gaming.myqcloud.com/cloud_gaming_web/docs/index.html)。

为了便于您快速接入Windows SDK，我们提供了一个基于**QtQuick**的Demo工程，可以通过[Cloud Phone QtQuick Demo快速上手指南](https://cg-sdk-1258344699.cos.ap-nanjing.myqcloud.com/CloudDeviceWinSDK/docs/Demo/Qt_Quick_Demo_Guide/index.html)下载和了解Demo运行流程。

---
## 2. 工程环境准备

**SDK 为纯 C 接口，无 Qt 依赖，可在任意 C++ 工程中集成**。本文以 Qt Quick 工程为例，介绍典型的集成流程和注意事项。
### 2.1 依赖环境

- **TcrSdk**（云手机 C/C++ SDK，含头文件、lib、dll）
- 若您的工程使用Visual Studio开发可以直接集成，若您的工程使用Qt或者其他项目，您需要确认编译器使用的是`MSVC 2022 64-bit`
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

参考如下 CMake 配置片段，确保 SDK 头文件、lib、dll 正确集成：

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

TcrConfig config;
config.Token = tokenResult.token.c_str();           // 业务后台获取的 Token
config.AccessInfo = tokenResult.accessInfo.c_str(); // 业务后台获取的 AccessInfo

TcrClientHandle tcrClient = tcr_client_get_instance();
TcrErrorCode err = tcr_client_init(tcrClient, &config);

if (err != TCR_SUCCESS) {
    // 处理初始化失败
}
```

### 4.2 获取实例截图（按需可选）

```cpp
char buffer[2048];
TcrAndroidInstance instance = tcr_client_get_android_instance(tcrClient);
bool ok = tcr_instance_get_image(instance, buffer, 2048, instanceId.c_str(), 0, 0, 0);
if (ok) {
    // buffer 返回图片 URL，可用于加载图片
}
```

### 4.3 创建串流会话并连接实例

#### 单实例连接

```cpp
TcrSessionHandle tcrSession = tcr_client_create_session(tcrClient);
const char* instanceIds[] = { instanceId.c_str() };
tcr_session_access(tcrSession, instanceIds, 1, false); // false 表示单实例
```

#### 多实例群控

```cpp
TcrSessionHandle tcrSession = tcr_client_create_session(tcrClient);
std::vector<const char*> idPtrs;
for (const auto& id : instanceIds) idPtrs.push_back(id.c_str());
tcr_session_access(tcrSession, idPtrs.data(), idPtrs.size(), true); // true 表示群控
```

> **说明**：如需动态加入群控实例，可通过 `tcr_instance_join_group`。

### 4.4 设置回调

#### 视频帧回调

```cpp
static void VideoFrameCallback(void* user_data, const TcrVideoFrame* video_frame) {
    // 处理视频帧
}
static TcrVideoFrameObserver video_observer;
video_observer.user_data = nullptr; // 可传 this 指针
video_observer.on_frame = VideoFrameCallback;
tcr_session_set_video_frame_observer(tcrSession, &video_observer);
```

#### 事件回调

```cpp
static void SessionEventCallback(void* user_data, TcrSessionEvent event, const char* eventData) {
    // 处理事件
}
static TcrSessionObserver session_observer;
session_observer.user_data = nullptr; // 可传 this 指针
session_observer.on_event = SessionEventCallback;
tcr_session_set_observer(tcrSession, &session_observer);
```

### 4.5 业务操作

#### 实时控制

```cpp
// 模拟返回键
tcr_session_send_keyboard_event(tcrSession, 158, true);  // 按下
tcr_session_send_keyboard_event(tcrSession, 158, false); // 抬起

// 触摸事件（按下）
tcr_session_touchscreen_touch(tcrSession, 100, 200, 0, 720, 1280, 0); // eventType: 0=DOWN, 1=MOVE, 2=UP

// 透传消息
tcr_session_send_trans_message(tcrSession, "com.example.app", "your_message");

// 粘贴文本
tcr_session_paste_text(tcrSession, "hello world");

// 切换输入法
tcr_session_switch_ime(tcrSession, "cloud"); // "cloud" 或 "local"
```

#### 批量任务

对云手机进行批量任务操作（如修改分辨率、应用管理等）：

```cpp
const char* input_json = R"({
    "TaskType": "ModifyResolution",
    "Params": {
        "cai-xxxx-xxx": { "Width": 1080, "Height": 1920 }
    }
})";
char* output = nullptr;
size_t output_size = 0;
TcrAndroidInstance instance = tcr_client_get_android_instance(tcrClient);
TcrErrorCode code = tcr_instance_request(instance, input_json, &output, &output_size);

if (code == TCR_SUCCESS && output) {
    // output 为 JSON 字符串, 具体内容需解析
    // ... 业务处理 ...
    tcr_instance_free_result(instance, output);
}
```

### 4.6 资源释放

每一个创建的会话实例在使用完成后必须释放：

```cpp
tcr_client_destroy_session(tcrClient, tcrSession);
```

---

## 5. 批量任务接口说明

### 5.1 批量任务调用

```cpp
const char* input_json = R"({
    "TaskType": "ModifyResolution",
    "Params": {
        "cai-xxxx-xxx": { "Width": 1080, "Height": 1920 }
    }
})";
char* output = nullptr;
size_t output_size = 0;
TcrErrorCode code = tcr_instance_request(instance, input_json, &output, &output_size);

if (code == TCR_SUCCESS && output) {
    // output 为 JSON 字符串, 具体内容需要根据不同任务进行解析(不关注结果则可忽略)
    // ... 业务处理 ...
    tcr_instance_free_result(tcrOperator, output);
}
```

### 5.2 支持的任务类型

- 修改分辨率: ModifyResolution
- 修改 GPS: ModifyGPS
- 粘贴文本: Paste
- 发送剪贴板: SendClipboard
- 修改传感器: ModifySensor
- 摇动/吹气: Shake/Blow
- 透传消息: SendTransMessage
- 修改实例属性: ModifyInstanceProperties
- 应用管理：StartApp、StopApp、UnInstallByPackageName、ClearAppData、EnableApp、DisableApp
- 保活管理：Add/Remove/Set/Describe/ClearKeepAliveList、ModifyKeepFrontAppStatus、DescribeKeepFrontAppStatus
- 摄像头：Start/Stop/DescribeCameraMediaPlay、DisplayCameraImage
- 应用黑名单：Add/Remove/Set/Describe/ClearAppInstallBlackList
- 查询应用列表: ListUserApps/ListAllApps
- 静音: Mute
- 媒体库搜索: MediaSearch
- 重启实例: Reboot
- 应用后台: MoveAppBackground

详见头文件，每个任务类型的参数和返回格式均有详细说明。

---
## 6. 常见问题与建议

- **内存释放**：批量任务返回的 `output` 需调用 `tcr_operator_free_result` 释放。
- 会话管理: 每一个`TcrClientHandle`只能调用一次`tcr_session_access`, 如需多个会话请创建多个实例并释放使用过的实例。
- **日志调试**：您需要通过 `tcr_set_log_callback`、`tcr_set_log_level` 配置日志，以便在出现问题之后将日志反馈给开发团队定位问题。

---

## 8. 参考资料

- [云手机PaaS技术接入](https://cloud.tencent.com/document/product/1162/113726)

---

如有更多问题，请联系云手机 PaaS 技术支持团队。