## 接口概览

### 生命周期与客户端接口

| 函数名 | 说明 |
| ------ | ---- |
| [tcr_client_get_instance](#tcr_client_get_instance) | 获取客户端句柄 |
| [tcr_client_prepare](#tcr_client_prepare) | 预初始化客户端并提前建立网络连接 |
| [tcr_client_init](#tcr_client_init) | 初始化客户端 |
| [tcr_client_release](#tcr_client_release) | 释放客户端资源 |
| [tcr_client_create_session](#tcr_client_create_session) | 创建会话对象 |
| [tcr_client_destroy_session](#tcr_client_destroy_session) | 销毁会话对象 |
| [tcr_client_get_android_instance](#tcr_client_get_android_instance) | 获取安卓实例操作句柄 |
| [tcr_session_set_observer](#tcr_session_set_observer) | 设置会话事件观察者 |
| [tcr_session_set_video_frame_observer](#tcr_session_set_video_frame_observer) | 设置视频帧观察者 |

### 安卓实例操作接口

| 函数名 | 说明 |
| ------ | ---- |
| [tcr_instance_join_group](#tcr_instance_join_group) | 将实例加入当前群组会话 |
| [tcr_instance_request_stream](#tcr_instance_request_stream) | 请求打开或关闭指定实例视频流 |
| [tcr_instance_set_master](#tcr_instance_set_master) | 设置或取消主控实例 |
| [tcr_instance_set_sync_list](#tcr_instance_set_sync_list) | 设置群控同步实例列表 |
| [tcr_instance_get_image](#tcr_instance_get_image) | 获取实例截图 URL |
| [tcr_instance_get_download_address](#tcr_instance_get_download_address) | 获取文件下载 URL |
| [tcr_instance_get_download_logcat_address](#tcr_instance_get_download_logcat_address) | 获取 logcat 日志下载地址 |
| [tcr_instance_upload_media](#tcr_instance_upload_media) | 上传媒体文件到实例相册 |
| [tcr_instance_upload_files](#tcr_instance_upload_files) | 上传本地文件到实例指定路径 |
| [tcr_instance_request](#tcr_instance_request) | 批量请求云端任务 |
| [tcr_instance_free_result](#tcr_instance_free_result) | 释放任务返回结果内存 |

### 会话与串流接口

| 函数名 | 说明 |
| ------ | ---- |
| [tcr_session_init](#tcr_session_init) | 初始化会话 |
| [tcr_session_start](#tcr_session_start) | 使用服务端 session 字符串启动会话 |
| [tcr_session_get_request_id](#tcr_session_get_request_id) | 获取当前会话 RequestId |
| [tcr_session_access](#tcr_session_access) | 单实例/群控接入 |
| [tcr_session_access_multi_stream](#tcr_session_access_multi_stream) | 多实例独立多流接入 |
| [tcr_session_switch_streaming_instances](#tcr_session_switch_streaming_instances) | 动态切换多流场景拉流实例列表 |
| [tcr_session_pause_streaming](#tcr_session_pause_streaming) | 暂停指定媒体流 |
| [tcr_session_resume_streaming](#tcr_session_resume_streaming) | 恢复指定媒体流 |
| [tcr_session_set_remote_video_profile](#tcr_session_set_remote_video_profile) | 配置远端视频流参数 |

### 视频帧访问接口

| 函数名 | 说明 |
| ------ | ---- |
| [tcr_video_frame_get_buffer](#tcr_video_frame_get_buffer) | 获取视频帧缓冲区信息 |
| [tcr_video_frame_add_ref](#tcr_video_frame_add_ref) | 增加视频帧引用计数 |
| [tcr_video_frame_release](#tcr_video_frame_release) | 释放视频帧引用 |
| [tcr_video_frame_buffer_is_d3d11](#tcr_video_frame_buffer_is_d3d11) | 判断缓冲区是否为 D3D11 类型 |
| [tcr_video_frame_buffer_get_d3d11_texture](#tcr_video_frame_buffer_get_d3d11_texture) | 获取 D3D11 纹理指针 |

### 数据通道接口

| 函数名 | 说明 |
| ------ | ---- |
| [tcr_session_create_data_channel](#tcr_session_create_data_channel) | 创建自定义数据通道 |
| [tcr_data_channel_send](#tcr_data_channel_send) | 发送数据通道消息 |
| [tcr_data_channel_close](#tcr_data_channel_close) | 关闭数据通道 |

### 本地音视频设备接口

| 函数名 | 说明 |
| ------ | ---- |
| [tcr_session_enable_local_camera_with_config](#tcr_session_enable_local_camera_with_config) | 按指定配置启用本地摄像头 |
| [tcr_session_enable_local_camera](#tcr_session_enable_local_camera) | 启用/禁用本地摄像头 |
| [tcr_session_disable_local_camera](#tcr_session_disable_local_camera) | 禁用本地摄像头 |
| [tcr_session_is_local_camera_enabled](#tcr_session_is_local_camera_enabled) | 查询本地摄像头是否启用 |
| [tcr_session_local_camera_config](#tcr_session_local_camera_config) | 配置本地摄像头码率与帧率 |
| [tcr_session_get_camera_device_count](#tcr_session_get_camera_device_count) | 获取摄像头设备数量 |
| [tcr_session_get_camera_device](#tcr_session_get_camera_device) | 获取摄像头设备信息 |
| [tcr_session_enable_local_microphone](#tcr_session_enable_local_microphone) | 启用/禁用本地麦克风 |
| [tcr_session_is_local_microphone_enabled](#tcr_session_is_local_microphone_enabled) | 查询本地麦克风是否启用 |
| [tcr_session_get_microphone_device_count](#tcr_session_get_microphone_device_count) | 获取麦克风设备数量 |
| [tcr_session_get_microphone_device](#tcr_session_get_microphone_device) | 获取麦克风设备信息 |
| [tcr_session_enable_microphone_with_config](#tcr_session_enable_microphone_with_config) | 按指定设备配置启用麦克风 |

### 输入与控制接口

| 函数名 | 说明 |
| ------ | ---- |
| [tcr_session_send_trans_message](#tcr_session_send_trans_message) | 发送透传消息 |
| [tcr_session_paste_text](#tcr_session_paste_text) | 通过剪贴板粘贴文本 |
| [tcr_session_input_text](#tcr_session_input_text) | 直接输入文本 |
| [tcr_session_switch_ime](#tcr_session_switch_ime) | 切换输入法 |
| [tcr_session_switch_camera](#tcr_session_switch_camera) | 切换云端摄像头开关状态 |
| [tcr_session_switch_mic](#tcr_session_switch_mic) | 切换云端麦克风开关状态 |
| [tcr_session_send_keyboard_event](#tcr_session_send_keyboard_event) | 发送键盘按键事件 |
| [tcr_session_send_mouse_key](#tcr_session_send_mouse_key) | 发送鼠标按键事件 |
| [tcr_session_send_mouse_scroll](#tcr_session_send_mouse_scroll) | 发送鼠标垂直滚轮事件 |
| [tcr_session_send_mouse_horizontal_scroll](#tcr_session_send_mouse_horizontal_scroll) | 发送鼠标水平滚轮事件 |
| [tcr_session_send_mouse_move_to](#tcr_session_send_mouse_move_to) | 发送鼠标绝对坐标移动 |
| [tcr_session_send_mouse_delta_move](#tcr_session_send_mouse_delta_move) | 发送鼠标相对位移移动 |
| [tcr_session_set_mouse_cursor_style](#tcr_session_set_mouse_cursor_style) | 设置云端鼠标光标样式 |
| [tcr_session_touchscreen_touch](#tcr_session_touchscreen_touch) | 发送基础触摸事件 |
| [tcr_session_touchscreen_touch_ex](#tcr_session_touchscreen_touch_ex) | 发送扩展多点触摸事件 |

### 日志与工具接口

| 函数名 | 说明 |
| ------ | ---- |
| [tcr_set_log_callback](#tcr_set_log_callback) | 设置日志回调 |
| [tcr_set_log_level](#tcr_set_log_level) | 设置日志级别 |
| [tcr_get_log_level](#tcr_get_log_level) | 获取日志级别 |
| [tcr_session_event_to_string](#tcr_session_event_to_string) | 将事件枚举转为字符串 |

## 使用约束与注意事项

### 观察者生命周期

- `TcrSessionObserver`、`TcrVideoFrameObserver`、`TcrDataChannelObserver` **不会被 SDK 复制或托管释放**。
- 传入观察者后，调用方必须保证对应结构体在整个使用期间持续有效。
- 在销毁 `session` 前，应显式传 `NULL` 取消 `tcr_session_set_observer` / `tcr_session_set_video_frame_observer`。
- 数据通道使用完成后应调用 `tcr_data_channel_close`。

### 视频帧生命周期

- 回调收到的 `TcrVideoFrameHandle` 默认仅在回调期间有效。
- 如需跨线程或异步使用，必须先调用 `tcr_video_frame_add_ref`。
- 每次 `add_ref` 后都必须配对调用 `tcr_video_frame_release`。

### 多流与群控互斥

- `tcr_session_access` 用于单实例接入或群控接入。
- `tcr_session_access_multi_stream` 用于多实例独立拉流观看(该接口仅sfu)。
- 同一个 `session` 只能使用其中一种模式。

## 详细接口说明

### 生命周期与客户端接口

#### tcr_client_get_instance

获取客户端实例句柄。

```cpp
TcrClientHandle tcr_client_get_instance();
```

- **参数**：无。
- **返回值**：`TcrClientHandle`，后续所有客户端相关操作都依赖该句柄。

#### tcr_client_prepare

预初始化客户端并提前建立网络连接。

```cpp
void tcr_client_prepare(TcrClientHandle client, bool testEnv = false);
```

- **参数**：
  - `client`：客户端句柄。
  - `testEnv`：是否使用测试环境，默认 `false`。
- **说明**：可在 `tcr_client_init` 之前调用，用于提前完成网络准备。

#### tcr_client_init

初始化客户端。

```cpp
TcrErrorCode tcr_client_init(TcrClientHandle client, const TcrConfig* config);
```

- **参数**：
  - `client`：客户端句柄。
  - `config`：客户端配置，建议使用 `tcr_config_default()` 初始化后再按需修改。
- **返回值**：`TcrErrorCode`，`TCR_SUCCESS` 表示成功。
- **说明**：可重复调用，用于更新 `token`、`accessInfo` 等配置。

#### tcr_client_release

释放客户端持有的所有资源。

```cpp
void tcr_client_release(TcrClientHandle client);
```

- **参数**：
  - `client`：客户端句柄。
- **说明**：
  - 应在进程退出前调用。
  - 调用后原句柄不可继续使用；如需再次使用 SDK，请重新调用 `tcr_client_get_instance`。

#### tcr_client_create_session

创建会话对象。

```cpp
TcrSessionHandle tcr_client_create_session(
    TcrClientHandle client,
    const TcrSessionConfig* session_config = nullptr
);
```

- **参数**：
  - `client`：客户端句柄。
  - `session_config`：会话配置；可传 `nullptr` 使用 SDK 默认行为，推荐传入 `tcr_session_config_default()` 结果。
- **返回值**：`TcrSessionHandle`；若客户端尚未初始化成功，则返回空。

#### tcr_client_destroy_session

销毁会话对象。

```cpp
void tcr_client_destroy_session(TcrClientHandle client, TcrSessionHandle session);
```

- **参数**：
  - `client`：客户端句柄。
  - `session`：待销毁的会话句柄。
- **说明**：销毁前建议先取消视频帧和会话观察者。

#### tcr_session_set_observer

设置会话事件观察者。

```cpp
void tcr_session_set_observer(TcrSessionHandle session, const TcrSessionObserver* observer);
```

- **参数**：
  - `session`：会话句柄。
  - `observer`：观察者指针；传 `NULL` 表示取消当前观察者。
- **说明**：
  - `on_event` 用于单实例或通用事件回调。
  - `on_instance_event` 用于多实例场景按实例拆分回调；若为空，SDK 回退到 `on_event`。

#### tcr_client_get_android_instance

获取安卓实例操作句柄。

```cpp
TcrAndroidInstance tcr_client_get_android_instance(TcrClientHandle client);
```

- **参数**：
  - `client`：客户端句柄。
- **返回值**：`TcrAndroidInstance`；客户端未初始化成功时返回空。

### 安卓实例操作接口

#### tcr_instance_join_group

将多个实例加入当前群组会话。

```cpp
void tcr_instance_join_group(TcrAndroidInstance op, const char** instanceIds, int32_t length);
```

- **参数**：
  - `op`：实例操作句柄。
  - `instanceIds`：实例 ID 数组。
  - `length`：数组长度。

#### tcr_instance_request_stream

请求打开或关闭指定实例的视频流。

```cpp
void tcr_instance_request_stream(
    TcrAndroidInstance op,
    const char* instanceID,
    bool status,
    const char* level
);
```

- **参数**：
  - `op`：实例操作句柄。
  - `instanceID`：实例 ID。
  - `status`：`true` 打开流，`false` 关闭流。
  - `level`：流质量级别，常用值为 `"low"`、`"normal"`、`"high"`。

#### tcr_instance_set_master

设置或取消主控实例。

```cpp
void tcr_instance_set_master(TcrAndroidInstance op, const char* instanceId, bool status);
```

- **参数**：
  - `op`：实例操作句柄。
  - `instanceId`：实例 ID。
  - `status`：`true` 表示设为主控，`false` 表示取消主控。

#### tcr_instance_set_sync_list

设置群控同步实例列表。

```cpp
void tcr_instance_set_sync_list(TcrAndroidInstance op, const char** instanceIds, int32_t length);
```

- **参数**：
  - `op`：实例操作句柄。
  - `instanceIds`：需要同步控制的实例 ID 数组。
  - `length`：数组长度。
- **说明**：列表中的实例必须已经在当前群组会话中。

#### tcr_instance_get_image

获取实例截图 URL。

```cpp
bool tcr_instance_get_image(
    TcrAndroidInstance op,
    char* buffer,
    int bufferLen,
    const char* instanceId,
    int width = 0,
    int height = 0,
    int quality = 20
);
```

- **参数**：
  - `op`：实例操作句柄。
  - `buffer`：输出缓冲区。
  - `bufferLen`：缓冲区长度。
  - `instanceId`：实例 ID。
  - `width`：截图宽度，`<= 0` 表示默认分辨率。
  - `height`：截图高度，`<= 0` 表示默认分辨率。
  - `quality`：图片质量，范围通常为 `1-100`。
- **返回值**：`true` 表示成功写入 URL。

#### tcr_instance_get_download_address

获取实例文件下载 URL。

```cpp
bool tcr_instance_get_download_address(
    TcrAndroidInstance op,
    char* buffer,
    int bufferLen,
    const char* instanceId,
    const char* path
);
```

- **参数**：
  - `op`：实例操作句柄。
  - `buffer`：输出缓冲区。
  - `bufferLen`：缓冲区长度。
  - `instanceId`：实例 ID。
  - `path`：云端文件路径。
- **返回值**：`true` 表示成功。

#### tcr_instance_get_download_logcat_address

获取 logcat 日志压缩包下载地址。

```cpp
bool tcr_instance_get_download_logcat_address(
    TcrAndroidInstance op,
    char* buffer,
    int bufferLen,
    const char* instanceId,
    int recentDays
);
```

- **参数**：
  - `op`：实例操作句柄。
  - `buffer`：输出缓冲区。
  - `bufferLen`：缓冲区长度。
  - `instanceId`：实例 ID。
  - `recentDays`：下载最近多少天内修改过的日志。
- **返回值**：`true` 表示成功。

#### tcr_instance_upload_media

上传媒体文件到云实例固定相册目录 `/data/media/0/DCIM`。

```cpp
bool tcr_instance_upload_media(
    TcrAndroidInstance op,
    const char* instanceId,
    const char** local_paths,
    int file_count,
    char** output,
    size_t* output_size,
    const TcrUploadCallback* callback
);
```

- **参数**：
  - `op`：实例操作句柄。
  - `instanceId`：实例 ID。
  - `local_paths`：本地文件绝对路径数组。
  - `file_count`：文件数量。
  - `output`：返回 JSON 字符串，由 SDK 分配，使用后需 `tcr_instance_free_result` 释放。
  - `output_size`：返回字符串长度。
  - `callback`：上传进度回调，可为 `NULL`。
- **返回值**：
  - `true`：接口调用成功，但仍需检查 `output` 中业务结果。
  - `false`：调用失败。
- **说明**：
  - 上传结束后会通知系统加入媒体库/相册。
  - `local_paths` 必须为可读的绝对路径，且建议使用全英文路径。
  - 多文件上传为串行处理，文件较多时建议分批调用。
- **返回示例**：

```json
{
  "Code": 0,
  "Msg": ""
}
```

#### tcr_instance_upload_files

上传本地文件到云手机实例指定目录。

```cpp
bool tcr_instance_upload_files(
    TcrAndroidInstance op,
    const char* instanceId,
    const char** local_paths,
    const char** cloud_paths,
    int file_count,
    char** output,
    size_t* output_size,
    const TcrUploadCallback* callback
);
```

- **参数**：
  - `op`：实例操作句柄。
  - `instanceId`：实例 ID。
  - `local_paths`：本地文件绝对路径数组。
  - `cloud_paths`：云端目录数组；可为 `NULL`，表示默认上传到 `/sdcard/Download`。
  - `file_count`：文件数量。
  - `output`：返回 JSON 字符串，由 SDK 分配，使用后需 `tcr_instance_free_result` 释放。
  - `output_size`：返回字符串长度。
  - `callback`：上传进度回调，可为 `NULL`。
- **返回值**：
  - `true`：接口调用成功，但仍需检查 `output` 中业务结果。
  - `false`：调用失败。
- **说明**：
  - `cloud_paths` 中每个路径通常形如 `/sdcard/xxx`。
  - 多文件上传为串行处理，建议分批调用。
- **返回示例**：

```json
{
  "Code": 0,
  "Message": "",
  "FileStatus": [
    {
      "FileName": "test_demo.py",
      "CloudPath": "/sdcard/123456/test_demo.py"
    }
  ]
}
```

#### tcr_instance_request

批量请求云端任务。

```cpp
TcrErrorCode tcr_instance_request(
    TcrAndroidInstance op,
    const char* input_json,
    char** output,
    size_t* output_size
);
```

- **参数**：
  - `op`：实例操作句柄。
  - `input_json`：输入 JSON 字符串。
  - `output`：输出 JSON 字符串，由 SDK 分配，使用后需 `tcr_instance_free_result` 释放。
  - `output_size`：输出字符串长度。
- **返回值**：
  - `TCR_SUCCESS`：请求已成功发起。
  - `TCR_ERR_INVALID_PARAMS`：输入参数非法。
- **输入格式**：

```json
{
  "TaskType": "任务类型字符串",
  "Params": {
    "cai-xxxx-xxx": { },
    "cai-yyyy-yyy": { }
  }
}
```

- **通用说明**：
  - `TaskType`：任务类型。
  - `Params`：以实例 ID 为 key、任务参数为 value 的对象。
  - 返回结果通常是一个 JSON 对象，key 为实例 ID，value 为执行结果。

- **支持的任务类型**：

| TaskType | 说明 | 参数示例 |
| -------- | ---- | -------- |
| `ModifyResolution` | 修改分辨率 | `{ "Width": 1080, "Height": 1920, "DPI": 320 }` |
| `ModifyGPS` | 修改 GPS 信息 | `{ "Longitude": 121.47, "Latitude": 31.23 }` |
| `Paste` | 粘贴文本 | `{ "Text": "hello" }` |
| `SendClipboard` | 发送剪贴板内容 | `{ "Text": "hello" }` |
| `ModifySensor` | 修改传感器数据 | `{ "Type": "accelerometer", "Values": [0.1, 0.2, 0.3], "Accuracy": 3 }` |
| `Shake` | 摇动设备 | `{}` |
| `Blow` | 吹气 | `{}` |
| `SendTransMessage` | 发送透传消息 | `{ "PackageName": "com.example", "Msg": "hello" }` |
| `ModifyInstanceProperties` | 修改实例属性 | 见下方补充说明 |
| `ModifyKeepFrontAppStatus` | 修改保活应用状态 | `{ "PackageName": "com.example", "Enable": true, "RestartInterValSeconds": 3 }` |
| `UnInstallByPackageName` | 卸载应用 | `{ "PackageName": "com.example" }` |
| `StartApp` | 启动应用 | `{ "PackageName": "com.example", "ActivityName": "MainActivity" }` |
| `StopApp` | 停止应用 | `{ "PackageName": "com.example" }` |
| `ClearAppData` | 清除应用数据 | `{ "PackageName": "com.example" }` |
| `EnableApp` | 启用应用 | `{ "PackageName": "com.example" }` |
| `DisableApp` | 禁用应用 | `{ "PackageName": "com.example" }` |
| `StartCameraMediaPlay` | 开始摄像头媒体播放 | `{ "FilePath": "/sdcard/video.mp4", "Loops": 3 }` |
| `DisplayCameraImage` | 显示摄像头图像 | `{ "FilePath": "/sdcard/image.jpg" }` |
| `AddKeepAliveList` | 添加保活列表 | `{ "AppList": ["com.wechat", "com.alipay"] }` |
| `RemoveKeepAliveList` | 移除保活列表 | `{ "AppList": ["com.wechat"] }` |
| `SetKeepAliveList` | 覆盖保活列表 | `{ "AppList": ["com.wechat", "com.alipay"] }` |
| `DescribeInstanceProperties` | 查询实例属性 | `{}` |
| `DescribeKeepFrontAppStatus` | 查询保活应用状态 | `{}` |
| `StopCameraMediaPlay` | 停止摄像头媒体播放 | `{}` |
| `DescribeCameraMediaPlayStatus` | 查询摄像头播放状态 | `{}` |
| `DescribeKeepAliveList` | 查询保活列表 | `{}` |
| `ClearKeepAliveList` | 清空保活列表 | `{}` |
| `ListUserApps` | 查询用户应用列表 | `{}` |
| `Mute` | 静音开关 | `{ "Mute": true }` |
| `MediaSearch` | 媒体库文件搜索 | `{ "Keyword": "abc" }` |
| `Reboot` | 重启实例 | `{}` |
| `ListAllApps` | 查询全部应用列表 | `{}` |
| `MoveAppBackground` | 将应用移到后台 | `{}` |
| `AddAppInstallBlackList` | 添加安装黑名单 | `{ "AppList": ["com.xxx"] }` |
| `RemoveAppInstallBlackList` | 移除安装黑名单 | `{ "AppList": ["com.xxx"] }` |
| `SetAppInstallBlackList` | 覆盖安装黑名单 | `{ "AppList": ["com.xxx"] }` |
| `DescribeAppInstallBlackList` | 查询安装黑名单 | `{}` |
| `ClearAppInstallBlackList` | 清空安装黑名单 | `{}` |

- **`ModifyInstanceProperties` 参数结构**：

```json
{
  "DeviceInfo": {
    "Brand": "Samsung",
    "Model": "Galaxy S23"
  },
  "ProxyInfo": {
    "Enabled": true,
    "Protocol": "socks5",
    "Host": "proxy.example.com",
    "Port": 1080,
    "User": "username",
    "Password": "password123"
  },
  "GPSInfo": {
    "Longitude": 121.4737,
    "Latitude": 31.2304
  },
  "SIMInfo": {
    "State": 1,
    "PhoneNumber": "13800138000",
    "IMSI": "460001234567890",
    "ICCID": "89860123456789012345"
  },
  "LocaleInfo": {
    "Timezone": "Asia/Shanghai"
  },
  "LanguageInfo": {
    "Language": "zh",
    "Country": "CN"
  },
  "ExtraProperties": [
    { "Key": "k1", "Value": "v1" }
  ]
}
```

- **输出结果示例**：

```json
{
  "cai-xxxx": {
    "Code": 0
  },
  "cai-yyyy": {
    "Code": 10100,
    "Msg": "operation denied"
  },
  "cai-zzzz": {
    "Code": -101,
    "Msg": "HTTP request failed"
  }
}
```

#### tcr_instance_free_result

释放由 `tcr_instance_request`、上传接口等分配的结果内存。

```cpp
void tcr_instance_free_result(TcrAndroidInstance op, char* output);
```

- **参数**：
  - `op`：实例操作句柄。
  - `output`：待释放内存指针。

### 会话与串流接口

#### tcr_session_init

初始化会话；初始化成功后会通过观察者回调 `TCR_SESSION_EVENT_STATE_INITED` 事件。

```cpp
void tcr_session_init(TcrSessionHandle session);
```

- **参数**：
  - `session`：会话句柄。

#### tcr_session_start

使用服务端返回的会话串启动会话。

```cpp
void tcr_session_start(TcrSessionHandle session, const char* serverSession);
```

- **参数**：
  - `session`：会话句柄。
  - `serverSession`：服务端 session 字符串。
- **说明**：该接口对同一个会话通常只应调用一次。

#### tcr_session_get_request_id

获取会话的 RequestId。

```cpp
bool tcr_session_get_request_id(TcrSessionHandle session, char* buffer, int buffer_size);
```

- **参数**：
  - `session`：会话句柄。
  - `buffer`：输出缓冲区。
  - `buffer_size`：缓冲区长度。
- **返回值**：成功返回 `true`。

#### tcr_session_set_video_frame_observer

设置视频帧观察者。

```cpp
void tcr_session_set_video_frame_observer(
    TcrSessionHandle session,
    const TcrVideoFrameObserver* observer
);
```

- **参数**：
  - `session`：会话句柄。
  - `observer`：观察者指针；传 `NULL` 表示取消。
- **说明**：回调中拿到的是 `TcrVideoFrameHandle`，需通过 `tcr_video_frame_get_buffer` 获取实际帧缓冲区。

#### tcr_session_access

启动会话并接入云端实例，支持单实例或群控模式。

```cpp
void tcr_session_access(
    TcrSessionHandle session,
    const char** instanceIds,
    int32_t length,
    bool isGroupControl
);
```

- **参数**：
  - `session`：会话句柄。
  - `instanceIds`：实例 ID 数组。
  - `length`：数组长度。
  - `isGroupControl`：是否群控。

#### tcr_session_access_multi_stream

连接多个云端实例并分别接收独立视频流。

```cpp
void tcr_session_access_multi_stream(
    TcrSessionHandle session,
    const char** instanceIds,
    int32_t length
);
```

- **参数**：
  - `session`：会话句柄。
  - `instanceIds`：实例 ID 数组。
  - `length`：数组长度。
- **说明**：
  - 与 `tcr_session_access` 互斥。
  - 每路视频流可在 `TcrVideoFrameBuffer.instance_id` 中区分来源实例。
  - 连接后可通过 `tcr_session_switch_streaming_instances` 动态切换实际拉流实例列表。

#### tcr_session_switch_streaming_instances

动态切换多流会话当前拉流的实例列表。

```cpp
void tcr_session_switch_streaming_instances(
    TcrSessionHandle session,
    const char** streamingInstanceIds,
    int32_t streamingLength
);
```

- **参数**：
  - `session`：会话句柄。
  - `streamingInstanceIds`：希望继续拉流的实例 ID 数组。
  - `streamingLength`：数组长度。
- **说明**：
  - 仅在 `tcr_session_access_multi_stream` 模式下有效。
  - 传入列表必须是最初 `instanceIds` 的子集。
  - 数量不能超过 `TcrSessionConfig.concurrentStreamingInstances`。
  - 若部分实例切换失败，会触发 `TCR_SESSION_EVENT_STREAMING_SWITCH_FAILED`。

#### tcr_session_pause_streaming

暂停指定媒体流。

```cpp
void tcr_session_pause_streaming(
    TcrSessionHandle session,
    const char* media_type = nullptr,
    const char** instanceIds = nullptr,
    int32_t instance_count = 0
);
```

- **参数**：
  - `session`：会话句柄。
  - `media_type`：`"audio"`、`"video"` 或空字符串；为空时表示同时暂停音视频。
  - `instanceIds`：实例 ID 数组；仅多流模式下有意义，为 `NULL` 时对全部实例生效。
  - `instance_count`：实例数量。

#### tcr_session_resume_streaming

恢复指定媒体流。

```cpp
void tcr_session_resume_streaming(
    TcrSessionHandle session,
    const char* media_type = nullptr,
    const char** instanceIds = nullptr,
    int32_t instance_count = 0
);
```

- **参数**：与 `tcr_session_pause_streaming` 一致。

#### tcr_session_set_remote_video_profile

设置远端视频流参数。

```cpp
void tcr_session_set_remote_video_profile(
    TcrSessionHandle session,
    int32_t fps,
    int32_t minBitrate,
    int32_t maxBitrate,
    int32_t video_width,
    int32_t video_height,
    const char** instanceIds = nullptr,
    int32_t instance_count = 0
);
```

- **参数**：
  - `session`：会话句柄。
  - `fps`：帧率，`<= 0` 表示不设置。
  - `minBitrate`：最小码率（kbps），与 `maxBitrate` 配套使用。
  - `maxBitrate`：最大码率（kbps），与 `minBitrate` 配套使用。
  - `video_width`：目标宽度。
  - `video_height`：目标高度。
  - `instanceIds`：仅多流模式下有效；为空时作用于全部实例。
  - `instance_count`：实例数量。
- **分辨率规则**：
  - `video_width > 0 && video_height > 0`：固定宽高。
  - `video_width > 0 && video_height == 0`：短边固定为 `video_width`，长边自适应。
  - `video_width == 0 && video_height > 0`：长边固定为 `video_height`，短边自适应。

### 视频帧访问接口

#### tcr_video_frame_get_buffer

获取视频帧缓冲区。

```cpp
const TcrVideoFrameBuffer* tcr_video_frame_get_buffer(TcrVideoFrameHandle frame_handle);
```

- **参数**：
  - `frame_handle`：视频帧句柄。
- **返回值**：`TcrVideoFrameBuffer*`；失败返回 `NULL`。
- **说明**：返回指针生命周期受 `frame_handle` 管理。

#### tcr_video_frame_add_ref

增加视频帧引用计数。

```cpp
void tcr_video_frame_add_ref(TcrVideoFrameHandle frame_handle);
```

- **参数**：
  - `frame_handle`：视频帧句柄。
- **说明**：跨线程、跨回调持有时必须先调用本接口。

#### tcr_video_frame_release

释放视频帧引用。

```cpp
void tcr_video_frame_release(TcrVideoFrameHandle frame_handle);
```

- **参数**：
  - `frame_handle`：视频帧句柄。
- **说明**：需与 `tcr_video_frame_add_ref` 成对调用。

#### tcr_video_frame_buffer_is_d3d11

判断缓冲区是否为 D3D11 纹理类型。

```cpp
bool tcr_video_frame_buffer_is_d3d11(const TcrVideoFrameBuffer* buffer);
```

- **参数**：
  - `buffer`：视频帧缓冲区。
- **返回值**：`true` 表示为 D3D11 类型。

#### tcr_video_frame_buffer_get_d3d11_texture

获取 D3D11 纹理指针。

```cpp
void* tcr_video_frame_buffer_get_d3d11_texture(const TcrVideoFrameBuffer* buffer);
```

- **参数**：
  - `buffer`：视频帧缓冲区，且必须为 D3D11 类型。
- **返回值**：底层 `ID3D11Texture2D*` 指针；失败返回 `NULL`。

### 数据通道接口

#### tcr_session_create_data_channel

创建自定义数据通道。

```cpp
TcrDataChannelHandle tcr_session_create_data_channel(
    TcrSessionHandle session,
    int32_t port,
    const TcrDataChannelObserver* observer,
    const char* type = "android"
);
```

- **参数**：
  - `session`：会话句柄。
  - `port`：云端数据通道唯一端口号。
  - `observer`：数据通道观察者；可为 `NULL`。
  - `type`：仅支持 `""`、`"android"`、`"android_broadcast"`，默认 `"android"`。
- **返回值**：数据通道句柄；失败返回空。

#### tcr_data_channel_send

通过数据通道发送数据。

```cpp
TcrErrorCode tcr_data_channel_send(
    TcrDataChannelHandle channel,
    const uint8_t* data,
    size_t size
);
```

- **参数**：
  - `channel`：数据通道句柄。
  - `data`：待发送数据。
  - `size`：数据长度。
- **返回值**：`TcrErrorCode`。

#### tcr_data_channel_close

关闭数据通道。

```cpp
void tcr_data_channel_close(TcrDataChannelHandle channel);
```

- **参数**：
  - `channel`：数据通道句柄。

### 本地音视频设备接口

#### tcr_session_enable_local_camera_with_config

使用指定配置启用本地摄像头。

```cpp
bool tcr_session_enable_local_camera_with_config(
    TcrSessionHandle session,
    const TcrVideoConfig* config
);
```

- **参数**：
  - `session`：会话句柄。
  - `config`：本地摄像头配置，建议通过 `tcr_video_config_default()` 获取默认值后修改。
- **返回值**：成功返回 `true`。
- **说明**：SDK 会根据配置查找最匹配的本地摄像头能力。

#### tcr_session_enable_local_camera

启用或禁用本地摄像头。

```cpp
bool tcr_session_enable_local_camera(TcrSessionHandle session, bool enable);
```

- **参数**：
  - `session`：会话句柄。
  - `enable`：`true` 启用，`false` 禁用。
- **返回值**：成功返回 `true`。

#### tcr_session_disable_local_camera

禁用本地摄像头。

```cpp
void tcr_session_disable_local_camera(TcrSessionHandle session);
```

- **参数**：
  - `session`：会话句柄。

#### tcr_session_is_local_camera_enabled

查询本地摄像头是否已启用。

```cpp
bool tcr_session_is_local_camera_enabled(TcrSessionHandle session);
```

- **参数**：
  - `session`：会话句柄。
- **返回值**：已启用返回 `true`。

#### tcr_session_local_camera_config

配置本地摄像头最大码率与帧率。

```cpp
bool tcr_session_local_camera_config(
    TcrSessionHandle session,
    int max_bitrate_bps,
    int max_fps
);
```

- **参数**：
  - `session`：会话句柄。
  - `max_bitrate_bps`：最大码率，单位 bps。
  - `max_fps`：最大帧率。
- **返回值**：成功返回 `true`。
- **说明**：需在摄像头已启用后调用。

#### tcr_session_get_camera_device_count

获取可用摄像头设备数量。

```cpp
int tcr_session_get_camera_device_count(TcrSessionHandle session);
```

- **参数**：
  - `session`：会话句柄。
- **返回值**：可用设备数量，负值表示错误。

#### tcr_session_get_camera_device

获取指定摄像头设备信息。

```cpp
bool tcr_session_get_camera_device(
    TcrSessionHandle session,
    int device_index,
    TcrCameraDeviceInfo* info
);
```

- **参数**：
  - `session`：会话句柄。
  - `device_index`：设备索引。
  - `info`：输出设备信息结构体。
- **返回值**：成功返回 `true`。

#### tcr_session_enable_local_microphone

启用或禁用本地麦克风。

```cpp
bool tcr_session_enable_local_microphone(TcrSessionHandle session, bool enable);
```

- **参数**：
  - `session`：会话句柄。
  - `enable`：`true` 启用，`false` 禁用。
- **返回值**：成功返回 `true`。

#### tcr_session_is_local_microphone_enabled

查询本地麦克风是否已启用。

```cpp
bool tcr_session_is_local_microphone_enabled(TcrSessionHandle session);
```

- **参数**：
  - `session`：会话句柄。
- **返回值**：已启用返回 `true`。

#### tcr_session_get_microphone_device_count

获取可用麦克风设备数量。

```cpp
int tcr_session_get_microphone_device_count(TcrSessionHandle session);
```

- **参数**：
  - `session`：会话句柄。
- **返回值**：设备数量。

#### tcr_session_get_microphone_device

获取指定麦克风设备信息。

```cpp
bool tcr_session_get_microphone_device(
    TcrSessionHandle session,
    int device_index,
    TcrMicrophoneDeviceInfo* info
);
```

- **参数**：
  - `session`：会话句柄。
  - `device_index`：设备索引。
  - `info`：输出设备信息结构体。
- **返回值**：成功返回 `true`。

#### tcr_session_enable_microphone_with_config

使用指定设备配置启用麦克风。

```cpp
bool tcr_session_enable_microphone_with_config(
    TcrSessionHandle session,
    const TcrMicrophoneConfig* config
);
```

- **参数**：
  - `session`：会话句柄。
  - `config`：麦克风配置；`device_id` 为空时使用系统默认设备。
- **返回值**：成功返回 `true`。

### 输入与控制接口

#### tcr_session_send_trans_message

发送透传消息到云端应用。

```cpp
void tcr_session_send_trans_message(
    TcrSessionHandle session,
    const char* package_name,
    const char* msg
);
```

- **参数**：
  - `session`：会话句柄。
  - `package_name`：目标应用包名。
  - `msg`：消息内容。
- **说明**：目标应用需实现 `Messenger` 服务。

#### tcr_session_paste_text

将文本写入云端剪贴板并执行粘贴。

```cpp
void tcr_session_paste_text(TcrSessionHandle session, const char* msg);
```

- **参数**：
  - `session`：会话句柄。
  - `msg`：文本内容。

#### tcr_session_input_text

直接向当前输入框输入文本。

```cpp
void tcr_session_input_text(
    TcrSessionHandle session,
    const char* content,
    const char* mode
);
```

- **参数**：
  - `session`：会话句柄。
  - `content`：待输入文本。
  - `mode`：输入模式，`"append"` 或 `"override"`。
- **说明**：调用前应已收到 `TCR_SESSION_EVENT_IME_STATUS_CHANGE`，且事件中的 `ime_type` 为 `"local"`。

#### tcr_session_switch_ime

切换输入法。

```cpp
void tcr_session_switch_ime(TcrSessionHandle session, const char* ime);
```

- **参数**：
  - `session`：会话句柄。
  - `ime`：输入法类型，`"cloud"` 或 `"local"`。

#### tcr_session_switch_camera

切换云端摄像头状态。

```cpp
void tcr_session_switch_camera(TcrSessionHandle session, const char* status);
```

- **参数**：
  - `session`：会话句柄。
  - `status`：状态，`"open"` 或 `"close"`。

#### tcr_session_switch_mic

切换云端麦克风状态。

```cpp
void tcr_session_switch_mic(TcrSessionHandle session, const char* status);
```

- **参数**：
  - `session`：会话句柄。
  - `status`：状态，`"open"` 或 `"close"`。

#### tcr_session_send_keyboard_event

发送云端键盘事件。

```cpp
void tcr_session_send_keyboard_event(TcrSessionHandle session, int32_t keycode, bool down);
```

- **参数**：
  - `session`：会话句柄。
  - `keycode`：按键码。
  - `down`：`true` 表示按下，`false` 表示抬起。
- **说明**：
  - 常见按键：返回键 `158`、菜单键 `139`、Home 键 `172`、音量加 `58`、音量减 `59`。
  - 模拟一次完整按键通常需要先发送按下，再发送抬起。

#### tcr_session_send_mouse_key

发送鼠标按键事件。

```cpp
void tcr_session_send_mouse_key(TcrSessionHandle session, TcrMouseKeyType key, bool down);
```

- **参数**：
  - `session`：会话句柄。
  - `key`：鼠标按键类型，见 `TcrMouseKeyType`。
  - `down`：`true` 按下，`false` 释放。

#### tcr_session_send_mouse_scroll

发送垂直滚轮事件。

```cpp
void tcr_session_send_mouse_scroll(TcrSessionHandle session, float delta);
```

- **参数**：
  - `session`：会话句柄。
  - `delta`：滚轮增量，建议范围 `-1.0 ~ 1.0`。

#### tcr_session_send_mouse_horizontal_scroll

发送水平滚轮事件。

```cpp
void tcr_session_send_mouse_horizontal_scroll(TcrSessionHandle session, float delta);
```

- **参数**：同 `tcr_session_send_mouse_scroll`。

#### tcr_session_send_mouse_move_to

发送鼠标绝对坐标移动事件。

```cpp
void tcr_session_send_mouse_move_to(TcrSessionHandle session, int32_t x, int32_t y);
```

- **参数**：
  - `session`：会话句柄。
  - `x`：远端坐标 X。
  - `y`：远端坐标 Y。

#### tcr_session_send_mouse_delta_move

发送鼠标相对位移事件。

```cpp
void tcr_session_send_mouse_delta_move(
    TcrSessionHandle session,
    int32_t delta_x,
    int32_t delta_y
);
```

- **参数**：
  - `session`：会话句柄。
  - `delta_x`：水平位移，正值向右。
  - `delta_y`：垂直位移，正值向下。

#### tcr_session_set_mouse_cursor_style

设置云端鼠标光标样式。

```cpp
void tcr_session_set_mouse_cursor_style(TcrSessionHandle session, TcrCursorStyle style);
```

- **参数**：
  - `session`：会话句柄。
  - `style`：光标样式，见 `TcrCursorStyle`。

#### tcr_session_touchscreen_touch

发送基础触摸事件。

```cpp
void tcr_session_touchscreen_touch(
    TcrSessionHandle session,
    float x,
    float y,
    int eventType,
    int width,
    int height,
    long timestamp
);
```

- **参数**：
  - `session`：会话句柄。
  - `x` / `y`：像素坐标。
  - `eventType`：`0` 按下、`1` 移动、`2` 抬起。
  - `width` / `height`：远端屏幕尺寸。
  - `timestamp`：事件时间戳（毫秒）。

#### tcr_session_touchscreen_touch_ex

发送扩展多点触摸事件。

```cpp
void tcr_session_touchscreen_touch_ex(
    TcrSessionHandle session,
    float x,
    float y,
    int eventType,
    int fingerID,
    int width,
    int height,
    long timestamp,
    float orientation,
    float pressure,
    float size,
    float tool_major,
    float tool_minor,
    float touch_major,
    float touch_minor
);
```

- **参数**：
  - `session`：会话句柄。
  - `x` / `y`：像素坐标。
  - `eventType`：`0` 按下、`1` 移动、`2` 抬起。
  - `fingerID`：指针 ID。
  - `width` / `height`：远端屏幕尺寸。
  - `timestamp`：事件时间戳（毫秒）。
  - `orientation`：触摸方向（弧度）。
  - `pressure`：压力值，通常范围 `0-1`。
  - `size`：标准化触摸尺寸。
  - `tool_major` / `tool_minor`：工具椭圆长短轴。
  - `touch_major` / `touch_minor`：实际接触区域长短轴。
- **说明**：适合多点触控、绘画、手写等需要精细控制的场景。

### 日志与工具接口

#### tcr_set_log_callback

设置日志回调。

```cpp
void tcr_set_log_callback(const TcrLogCallback* callback);
```

- **参数**：
  - `callback`：日志回调结构体指针。

#### tcr_set_log_level

设置日志输出级别。

```cpp
void tcr_set_log_level(TcrLogLevel level);
```

- **参数**：
  - `level`：日志级别，见 `TcrLogLevel`。

#### tcr_get_log_level

获取当前日志级别。

```cpp
TcrLogLevel tcr_get_log_level();
```

- **参数**：无。
- **返回值**：当前日志级别。

#### tcr_session_event_to_string

将 `TcrSessionEvent` 枚举值转换为字符串。

```cpp
const char* tcr_session_event_to_string(TcrSessionEvent event);
```

- **参数**：
  - `event`：事件枚举值。
- **返回值**：静态字符串，不需要释放。

## 结构体、枚举与回调说明

### 句柄类型

```cpp
typedef void* TcrClientHandle;
typedef void* TcrSessionHandle;
typedef void* TcrAndroidInstance;
typedef void* TcrDataChannelHandle;
typedef void* TcrVideoFrameHandle;
```

### 常量

#### TCR_MAX_CONCURRENT_STREAMING_INSTANCES

```cpp
#define TCR_MAX_CONCURRENT_STREAMING_INSTANCES 500
```

- **说明**：多实例场景下允许同时拉流的实例数上限。

### TcrErrorCode

```cpp
typedef enum {
    TCR_SUCCESS = 0,
    TCR_ERR_TIMEOUT = 100010,
    TCR_ERR_INVALID_PARAMS = 100011,
    TCR_ERR_UNKNOWN = 100012,
    TCR_ERR_INTERNAL_ERROR = 100013,
    TCR_ERR_STATE_ILLEGAL = 100014,
    TCR_ERR_DATA_CHANNEL_BASE_CODE = 102000,
    TCR_ERR_CREATE_FAILURE = 102001,
    TCR_ERR_CLOSED = 102002,
} TcrErrorCode;
```

| 枚举值 | 说明 |
| ------ | ---- |
| `TCR_SUCCESS` | 成功 |
| `TCR_ERR_TIMEOUT` | 超时 |
| `TCR_ERR_INVALID_PARAMS` | 参数无效 |
| `TCR_ERR_UNKNOWN` | 未知错误 |
| `TCR_ERR_INTERNAL_ERROR` | 内部错误 |
| `TCR_ERR_STATE_ILLEGAL` | 状态非法 |
| `TCR_ERR_CREATE_FAILURE` | 数据通道创建失败 |
| `TCR_ERR_CLOSED` | 数据通道已关闭 |

### TcrSdkType

```cpp
typedef enum {
    CloudPhonePaas = 0,
    CloudStream = 1
} TcrSdkType;
```

### TcrMouseKeyType

```cpp
typedef enum {
    TCR_MOUSE_KEY_LEFT = 0,
    TCR_MOUSE_KEY_RIGHT = 1,
    TCR_MOUSE_KEY_MIDDLE = 2,
    TCR_MOUSE_KEY_FORWARD = 3,
    TCR_MOUSE_KEY_BACKWARD = 4
} TcrMouseKeyType;
```

### TcrCursorStyle

```cpp
typedef enum {
    TCR_CURSOR_STYLE_NORMAL = 0,
    TCR_CURSOR_STYLE_HUGE = 1
} TcrCursorStyle;
```

### TcrConfig

```cpp
typedef struct {
    const char* token;
    const char* accessInfo;
    TcrSdkType sdkType;
    bool streamOpt;
    bool hardwareDecode;
} TcrConfig;
```

- **字段说明**：
  - `token`：业务后台生成的访问令牌。
  - `accessInfo`：业务后台返回的访问信息。
  - `sdkType`：SDK 类型，默认 `CloudStream`。
  - `streamOpt`：是否开启串流优化，仅测试场景使用。
  - `hardwareDecode`：是否启用硬件解码。

#### tcr_config_default

```cpp
TcrConfig tcr_config_default(void);
```

- **默认值**：
  - `token = NULL`
  - `accessInfo = NULL`
  - `sdkType = CloudStream`
  - `streamOpt = false`
  - `hardwareDecode = false`

### TcrI420Buffer

```cpp
typedef struct {
    const uint8_t* data_y;
    const uint8_t* data_u;
    const uint8_t* data_v;
    int32_t stride_y;
    int32_t stride_u;
    int32_t stride_v;
    int32_t width;
    int32_t height;
} TcrI420Buffer;
```

### TcrVideoBufferType

```cpp
typedef enum {
    TCR_VIDEO_BUFFER_TYPE_I420 = 0,
    TCR_VIDEO_BUFFER_TYPE_D3D11 = 1
} TcrVideoBufferType;
```

### TcrD3D11Buffer

```cpp
typedef struct {
    void* texture;
    void* device;
    int32_t width;
    int32_t height;
    int32_t array_index;
    int32_t format;
} TcrD3D11Buffer;
```

- **说明**：
  - `texture` 可按 `ID3D11Texture2D*` 使用。
  - `device` 可按 `ID3D11Device*` 使用。
  - SDK 当前保证纹理格式固定为 `DXGI_FORMAT_R8G8B8A8_UNORM`（值 `28`）。

### TcrVideoRotation

```cpp
typedef enum {
    TCR_VIDEO_ROTATION_0 = 0,
    TCR_VIDEO_ROTATION_90 = 90,
    TCR_VIDEO_ROTATION_180 = 180,
    TCR_VIDEO_ROTATION_270 = 270
} TcrVideoRotation;
```

### TcrVideoFrameBuffer

```cpp
typedef struct TcrVideoFrameBuffer {
    TcrVideoBufferType type;
    union {
        TcrI420Buffer i420;
        TcrD3D11Buffer d3d11;
    } buffer;
    int64_t timestamp_us;
    TcrVideoRotation rotation;
    const char* instance_id;
    int instance_index;
} TcrVideoFrameBuffer;
```

- **说明**：
  - `type` 用于区分软件解码的 `I420` 与硬件解码的 `D3D11`。
  - 多流模式下可通过 `instance_id` 和 `instance_index` 区分来源。
  - `rotation` 字段当前保留，业务应以实际画面处理策略为准。

### TcrVideoFrame

```cpp
typedef struct {
    TcrVideoFrameBuffer frame_buffer;
    int64_t timestamp_us;
    TcrVideoRotation rotation;
} TcrVideoFrame;
```

### TcrVideoFrameCallback / TcrVideoFrameObserver

```cpp
typedef void (*TcrVideoFrameCallback)(void* user_data, TcrVideoFrameHandle frame_handle);

typedef struct TcrVideoFrameObserver {
    void* user_data;
    TcrVideoFrameCallback on_frame;
} TcrVideoFrameObserver;
```

#### tcr_video_frame_observer_default

```cpp
TcrVideoFrameObserver tcr_video_frame_observer_default(void);
```

- **默认值**：`user_data = NULL`，`on_frame = NULL`。

### TcrStreamProfile

```cpp
typedef struct {
    int32_t video_width;
    int32_t video_height;
    int32_t fps;
    int32_t max_bitrate;
    int32_t min_bitrate;
} TcrStreamProfile;
```

### TcrSessionConfig

```cpp
typedef struct {
    TcrStreamProfile stream_profile;
    int32_t statsInterval;
    const char* user_id;
    bool enable_audio;
    bool enable_hardware_decode;
    int32_t concurrentStreamingInstances;
} TcrSessionConfig;
```

- **字段说明**：
  - `stream_profile`：初始拉流参数。
  - `statsInterval`：`TCR_SESSION_EVENT_CLIENT_STATS` 回调间隔，范围通常为 `1-60` 秒。
  - `user_id`：会话用户标识。
  - `enable_audio`：是否启用音频。
  - `enable_hardware_decode`：是否启用硬件解码。
  - `concurrentStreamingInstances`：多流模式下允许同时拉流的最大实例数。

#### tcr_session_config_default

```cpp
TcrSessionConfig tcr_session_config_default(void);
```

- **默认值**：
  - `stream_profile.video_width = -1`
  - `stream_profile.video_height = -1`
  - `stream_profile.fps = 0`
  - `stream_profile.max_bitrate = 0`
  - `stream_profile.min_bitrate = 0`
  - `user_id = NULL`
  - `enable_audio = true`
  - `enable_hardware_decode = false`
  - `statsInterval = 1`
  - `concurrentStreamingInstances = 1`

### TcrUploadProgressCallback / TcrUploadCallback

```cpp
typedef void (*TcrUploadProgressCallback)(
    const char* instance_id,
    double total_bytes,
    double transferred_bytes,
    void* user_data
);

typedef struct {
    TcrUploadProgressCallback on_progress;
    void* user_data;
} TcrUploadCallback;
```

### TcrVideoConfig

```cpp
typedef struct TcrVideoConfig {
    int width;
    int height;
    int max_fps;
    int max_bitrate_bps;
    const char* device_id;
} TcrVideoConfig;
```

#### tcr_video_config_default

```cpp
TcrVideoConfig tcr_video_config_default(void);
```

- **默认值**：
  - `width = 640`
  - `height = 480`
  - `max_fps = 30`
  - `max_bitrate_bps = 1000000`
  - `device_id = NULL`

### TcrVideoCapability / TcrCameraDeviceInfo

```cpp
typedef struct TcrVideoCapability {
    int width;
    int height;
} TcrVideoCapability;

typedef struct TcrCameraDeviceInfo {
    char device_name[256];
    char device_id[256];
    TcrVideoCapability capabilities[16];
    int capability_count;
} TcrCameraDeviceInfo;
```

### TcrMicrophoneDeviceInfo / TcrMicrophoneConfig

```cpp
typedef struct TcrMicrophoneDeviceInfo {
    char device_name[128];
    char device_id[128];
} TcrMicrophoneDeviceInfo;

typedef struct TcrMicrophoneConfig {
    char device_id[128];
} TcrMicrophoneConfig;
```

### TcrDataChannelObserver

```cpp
typedef struct TcrDataChannelObserver {
    void* user_data;
    void (*on_connected)(void* user_data, const int32_t port);
    void (*on_error)(void* user_data, const int32_t port, const TcrErrorCode& code, const char* msg);
    void (*on_message)(void* user_data, const int32_t port, const uint8_t* data, size_t size);
} TcrDataChannelObserver;
```

#### tcr_data_channel_observer_default

```cpp
TcrDataChannelObserver tcr_data_channel_observer_default(void);
```

- **默认值**：所有回调与 `user_data` 均初始化为 `NULL`。

### TcrSessionObserver

```cpp
typedef struct TcrSessionObserver {
    void* user_data;
    void (*on_event)(void* user_data, TcrSessionEvent event, const char* eventData);
    void (*on_instance_event)(void* user_data, const char* instanceId, TcrSessionEvent event, const char* eventData);
} TcrSessionObserver;
```

#### tcr_session_observer_default

```cpp
TcrSessionObserver tcr_session_observer_default(void);
```

- **默认值**：`user_data = NULL`，所有回调为 `NULL`。

### TcrSessionEvent

```cpp
typedef enum {
    TCR_SESSION_EVENT_UNDEFINED = -1,
    TCR_SESSION_EVENT_STATE_INITED = 0,
    TCR_SESSION_EVENT_STATE_CONNECTED = 1,
    TCR_SESSION_EVENT_STATE_CLOSED = 2,
    TCR_SESSION_EVENT_CLIENT_STATS = 3,
    TCR_SESSION_EVENT_SCREEN_CONFIG_CHANGE = 4,
    TCR_SESSION_EVENT_REQUEST_STREAMING_STATUS = 5,
    TCR_SESSION_EVENT_HIT_INPUT = 6,
    TCR_SESSION_EVENT_CAMERA_STATUS = 7,
    TCR_SESSION_EVENT_TRANSMISSION_MESSAGE = 8,
    TCR_SESSION_EVENT_SYSTEM_USAGE = 9,
    TCR_SESSION_EVENT_CLIPBOARD_EVENT = 10,
    TCR_SESSION_EVENT_NOTIFICATION_EVENT = 11,
    TCR_SESSION_EVENT_IME_STATUS_CHANGE = 12,
    TCR_SESSION_EVENT_REMOTE_DESKTOP_INFO = 13,
    TCR_SESSION_EVENT_RECONNECTING = 14,
    TCR_SESSION_EVENT_STREAMING_DISCONNECT = 15,
    TCR_SESSION_EVENT_STREAMING_SWITCH_FAILED = 16,
    TCR_SESSION_EVENT_SERVER_STREAMING_STARTED = 17,
} TcrSessionEvent;
```

| 事件 | 说明 | `eventData` 结构 |
| ---- | ---- | ---------------- |
| `TCR_SESSION_EVENT_UNDEFINED` | 未定义事件 | - |
| `TCR_SESSION_EVENT_STATE_INITED` | 会话初始化完成（已废弃状态） | - |
| `TCR_SESSION_EVENT_STATE_CONNECTED` | 会话已连接 | - |
| `TCR_SESSION_EVENT_STATE_CLOSED` | 会话关闭 | 关闭原因字符串 |
| `TCR_SESSION_EVENT_CLIENT_STATS` | 客户端性能统计 | JSON，包含 `bitrate`、`fps`、`rtt`、丢包、收发包等字段 |
| `TCR_SESSION_EVENT_SCREEN_CONFIG_CHANGE` | 云端屏幕配置变化 | JSON，包含 `degree`、`width`、`height` |
| `TCR_SESSION_EVENT_REQUEST_STREAMING_STATUS` | 请求视频流状态结果 | JSON，包含 `user`、`code`、`status` |
| `TCR_SESSION_EVENT_HIT_INPUT` | 输入框状态变化 | JSON，包含 `field_type`、`text` |
| `TCR_SESSION_EVENT_CAMERA_STATUS` | 云端摄像头状态变化 | JSON，包含 `status`、`width`、`height` |
| `TCR_SESSION_EVENT_TRANSMISSION_MESSAGE` | 收到云端透传消息 | JSON，包含 `package_name`、`msg` |
| `TCR_SESSION_EVENT_SYSTEM_USAGE` | 云端系统资源使用率 | JSON，包含 `cpu_usage`、`mem_usage`、`gpu_usage` |
| `TCR_SESSION_EVENT_CLIPBOARD_EVENT` | 云端剪贴板变化 | JSON，包含 `text` |
| `TCR_SESSION_EVENT_NOTIFICATION_EVENT` | 新通知事件 | JSON，包含 `package_name`、`title`、`text` |
| `TCR_SESSION_EVENT_IME_STATUS_CHANGE` | 输入法状态变化 | JSON，包含 `ime_type`（`cloud` / `local`） |
| `TCR_SESSION_EVENT_REMOTE_DESKTOP_INFO` | 云桌面屏幕信息 | JSON，包含 `screen_width`、`screen_height`、`screen_left`、`screen_top` |
| `TCR_SESSION_EVENT_RECONNECTING` | 正在重连 | 无数据 |
| `TCR_SESSION_EVENT_STREAMING_DISCONNECT` | 多流场景实例断开 | JSON，包含 `user_list` |
| `TCR_SESSION_EVENT_STREAMING_SWITCH_FAILED` | 多流切流失败 | JSON，包含 `failed_list` |
| `TCR_SESSION_EVENT_SERVER_STREAMING_STARTED` | 服务端开始推流 | 无数据 |

### TcrLogLevel / TcrLogCallback

```cpp
typedef enum TcrLogLevel {
    TCR_LOG_LEVEL_TRACE = 0,
    TCR_LOG_LEVEL_DEBUG = 1,
    TCR_LOG_LEVEL_INFO  = 2,
    TCR_LOG_LEVEL_WARN  = 3,
    TCR_LOG_LEVEL_ERROR = 4
} TcrLogLevel;

typedef struct TcrLogCallback {
    void* user_data;
    void (*on_log)(void* user_data, TcrLogLevel level, const char* tag, const char* log);
} TcrLogCallback;
```

## 备注

- 上传类接口返回中的错误信息字段，文档示例中可能出现 `Msg` 或 `Message`，以实际返回数据为准。
- 如需查看更多任务返回样例、事件 JSON 细节或渲染实现说明，请结合头文件注释一并参考。
