## 接口概览

### 生命周期接口

| 函数名 | 说明 |
| ------ | ---- |
| [tcr_client_get_instance](#获取tcrclienthandle实例) | 获取 TcrClientHandle 实例 |
| [tcr_client_init](#初始化tcrclienthandle) | 初始化 TcrClientHandle |
| [tcr_client_create_session](#创建会话对象) | 创建会话对象 |
| [tcr_client_destroy_session](#销毁会话对象) | 销毁会话对象 |
| [tcr_session_set_observer](#设置会话事件观察者) | 设置会话事件观察者 |
| [tcr_set_log_callback](#设置日志回调函数) | 设置日志回调函数 |
| [tcr_set_log_level](#设置日志输出级别) | 设置日志输出级别 |
| [tcr_get_log_level](#获取当前日志输出级别) | 获取当前日志输出级别 |

### 安卓实例操作接口

| 函数名 | 说明 |
| ------ | ---- |
| [tcr_client_get_android_instance](#获取tcrandroidinstance操作对象) | 获取 TcrAndroidInstance 操作对象 |
| [tcr_instance_join_group](#将多个实例加入当前群组会话) | 多实例同步控制 |
| [tcr_instance_request_stream](#请求打开或关闭指定实例的视频流) | 控制实例视频流开关及质量 |
| [tcr_instance_set_master](#设置或取消主控实例) | 设置/取消主控实例 |
| [tcr_instance_set_sync_list](#设置群组会话同步实例列表) | 设置群控同步列表 |
| [tcr_instance_get_image](#获取指定实例的截图图片url) | 获取实例截图图片 URL |
| [tcr_instance_get_download_address](#获取文件下载url) | 获取文件下载 URL |
| [tcr_instance_get_download_logcat_address](#获取logcat日志下载地址) | 获取 logcat 日志下载地址 |
| [tcr_instance_upload_media](#上传文件到云实例相册) | 上传文件到云实例相册 |
| [tcr_instance_upload_files](#上传本地文件到云手机实例) | 上传本地文件到云手机 |
| [tcr_instance_request](#批量请求云端任务) | 批量请求云端任务 |
| [tcr_instance_free_result](#释放由tcr_instance_request分配的结果内存) | 释放结果内存 |

### 音视频控制接口

| 函数名 | 说明 |
| ------ | ---- |
| [tcr_session_set_video_frame_observer](#设置远端视频帧观察者) | 设置视频帧回调 |
| [tcr_session_connect](#连接到指定云端实例) | 连接单实例 |
| [tcr_session_access](#启动会话并连接云手机) | 启动/群控会话 |
| [tcr_session_pause_streaming](#暂停媒体流) | 暂停媒体流 |
| [tcr_session_resume_streaming](#恢复媒体流) | 恢复媒体流 |
| [tcr_session_set_remote_video_profile](#设置远端视频流参数) | 设置远端视频流参数 |
| [tcr_session_send_trans_message](#向云端应用发送透传消息) | 发送透传消息 |
| [tcr_session_paste_text](#将文本写入剪贴板并粘贴到输入框) | 粘贴文本到输入框 |
| [tcr_session_input_text](#将文本直接输入到输入框) | 直接输入文本 |
| [tcr_session_switch_ime](#切换输入法) | 切换输入法 |

### 外设控制接口

| 函数名 | 说明 |
| ------ | ---- |
| [tcr_session_switch_camera](#开关摄像头) | 开关摄像头 |
| [tcr_session_switch_mic](#开关麦克风) | 开关麦克风 |
| [tcr_session_send_keyboard_event](#发送键盘事件) | 发送云端键盘事件 |
| [tcr_session_touchscreen_touch](#发送触摸屏触摸事件) | 发送触摸事件 |

## 详细说明

#### 获取TcrClientHandle实例

获取 TcrClientHandle 实例，用于后续所有客户端相关操作。

```c
TcrClientHandle tcr_client_get_instance();
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| 无 | - | - |

#### 初始化TcrClientHandle

初始化 TcrClientHandle。

```c
TcrErrorCode tcr_client_init(TcrClientHandle client, const TcrConfig* config);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| client | TcrClientHandle | 客户端句柄 |
| config | const TcrConfig* | 配置信息指针 |

#### 创建会话对象

创建会话对象，用于串流和控制云手机实例。

```c
TcrSessionHandle tcr_client_create_session(TcrClientHandle client);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| client | TcrClientHandle | 客户端句柄 |

#### 销毁会话对象

销毁会话对象，释放相关资源。

```c
void tcr_client_destroy_session(TcrClientHandle client, TcrSessionHandle session);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| client | TcrClientHandle | 客户端句柄 |
| session | TcrSessionHandle | 会话句柄 |

#### 设置会话事件观察者

设置会话事件观察者，用于接收会话生命周期及业务事件。

```c
void tcr_session_set_observer(TcrSessionHandle session, const TcrSessionObserver* observer);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| session | TcrSessionHandle | 会话句柄 |
| observer | const TcrSessionObserver* | 观察者结构体指针 |

#### 设置日志回调函数

设置日志回调函数，用于接收 SDK 日志。

```c
void tcr_set_log_callback(const TcrLogCallback* callback);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| callback | const TcrLogCallback* | 日志回调结构体指针 |

#### 设置日志输出级别

设置日志输出级别。

```c
void tcr_set_log_level(TcrLogLevel level);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| level | TcrLogLevel | 日志级别 |

#### 获取当前日志输出级别

获取当前日志输出级别。

```c
TcrLogLevel tcr_get_log_level();
```

| 参数  | 类型  | 说明  |
| --- | --- | --- |
| 无   | -   | -   |

#### 获取TcrAndroidInstance操作对象

获取 TcrAndroidInstance 操作对象。

```c
TcrAndroidInstance tcr_client_get_android_instance(TcrClientHandle client);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| client | TcrClientHandle | 客户端句柄 |

#### 将多个实例加入当前群组会话

将多个实例加入当前群组会话，实现多实例同步控制。

```c
void tcr_instance_join_group(TcrAndroidInstance op, const char** instanceIds, int32_t length);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| op | TcrAndroidInstance | 实例操作句柄 |
| instanceIds | const char** | 实例 ID 字符串数组 |
| length | int32_t | 实例 ID 数组长度 |

#### 请求打开或关闭指定实例的视频流

请求打开或关闭指定实例的视频流，并设置流质量。

```c
void tcr_instance_request_stream(TcrAndroidInstance op, const char* instanceID, bool status, const char* level);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| op | TcrAndroidInstance | 实例操作句柄 |
| instanceID | const char* | 实例 ID |
| status | bool | true 打开流，false 关闭流 |
| level | const char* | 流质量级别（"low"、"normal"、"high"） |

#### 设置或取消主控实例

设置或取消主控实例，实现主从同步。

```c
void tcr_instance_set_master(TcrAndroidInstance op, const char* instanceId, bool status);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| op | TcrAndroidInstance | 实例操作句柄 |
| instanceId | const char* | 主控实例 ID |
| status | bool | true 设置为主控，false 取消主控 |

#### 设置群组会话同步实例列表

设置群组会话中需要被群控的实例列表。

```c
void tcr_instance_set_sync_list(TcrAndroidInstance op, const char** instanceIds, int32_t length);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| op | TcrAndroidInstance | 实例操作句柄 |
| instanceIds | const char** | 需要同步控制的实例 ID 数组 |
| length | int32_t | 实例 ID 数组长度 |

#### 获取指定实例的截图图片URL

获取指定实例的截图图片 URL。

```c
bool tcr_instance_get_image(TcrAndroidInstance op, char* buffer, int bufferLen, const char* instanceId, int width, int height, int quality);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| op | TcrAndroidInstance | 实例操作句柄 |
| buffer | char* | 输出缓冲区 |
| bufferLen | int | 输出缓冲区长度 |
| instanceId | const char* | 实例 ID |
| width | int | 图片宽度（<=0 默认） |
| height | int | 图片高度（<=0 默认） |
| quality | int | 图片质量（1-100） |

#### 获取文件下载URL

获取文件下载 URL。

```c
bool tcr_instance_get_download_address(TcrAndroidInstance op, char* buffer, int bufferLen, const char* instanceId, const char* path);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| op | TcrAndroidInstance | 实例操作句柄 |
| buffer | char* | 输出缓冲区 |
| bufferLen | int | 缓冲区长度 |
| instanceId | const char* | 实例 ID |
| path | const char* | 文件路径 |

#### 获取logcat日志下载地址

获取云手机实例 logcat 日志压缩包的下载地址。

```c
bool tcr_instance_get_download_logcat_address(TcrAndroidInstance op, char* buffer, int bufferLen, const char* instanceId, int recentDays);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| op | TcrAndroidInstance | 实例操作句柄 |
| buffer | char* | 输出缓冲区 |
| bufferLen | int | 缓冲区长度 |
| instanceId | const char* | 实例 ID |
| recentDays | int | 下载几天内的日志 |

#### 上传文件到云实例相册

上传文件到云实例，固定上传至 /data/media/0/DCIM，并自动添加到相册。

```c
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

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| op | TcrAndroidInstance | 实例操作句柄 |
| instanceId | const char* | 实例 ID |
| local_paths | const char** | 本地文件路径数组 |
| file_count | int | 文件数量 |
| output | char** | 输出 JSON 字符串（需释放） |
| output_size | size_t* | 输出字符串长度 |
| callback | const TcrUploadCallback* | 上传进度回调（可为 NULL） |

#### 上传本地文件到云手机实例

上传本地文件到云手机实例指定目录。

```c
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

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| op | TcrAndroidInstance | 实例操作句柄 |
| instanceId | const char* | 实例 ID |
| local_paths | const char** | 本地文件路径数组 |
| cloud_paths | const char** | 云端目标路径数组（可为 NULL） |
| file_count | int | 文件数量 |
| output | char** | 输出 JSON 字符串（需释放） |
| output_size | size_t* | 输出字符串长度 |
| callback | const TcrUploadCallback* | 上传进度回调（可为 NULL） |

#### 批量请求云端任务

`tcr_instance_request` 用于批量请求云端安卓实例的各类任务操作，包括但不限于分辨率修改、文本粘贴、GPS修改、应用管理、设备属性变更、保活策略、媒体操作等。该接口支持一次性对多个实例下发同类型任务。

```c
TcrErrorCode tcr_instance_request(TcrAndroidInstance op, const char* input_json, char** output, size_t* output_size);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| op | TcrAndroidInstance | 实例操作句柄 |
| input_json | const char* | 输入 JSON 字符串，指定任务类型、目标实例及参数，详见下方格式说明 |
| output | char** | 输出 JSON 字符串（需调用 tcr_instance_free_result 释放） |
| output_size | size_t* | 输出字符串长度 |

##### 输入 JSON 格式

```json
{
  "TaskType": "任务类型字符串",
  "Params": {
    "cai-xxxx-xxx": { ... }, // 实例ID: 任务参数
    "cai-yyyy-yyy": { ... }
  }
}
```

- **TaskType**：必填，字符串，指定要执行的任务类型。支持的任务类型详见下表。
- **Params**：必填，字典。key为实例ID，value为该实例的任务参数对象。参数结构依赖于TaskType。

##### 支持的任务类型及参数说明

| 任务类型 | 说明 | 参数结构示例 |
| -------- | ---- | ------------ |
| ModifyResolution | 修改分辨率 | `{ "Width": 1080, "Height": 1920, "DPI": 320 }` |
| ModifyGPS | 修改GPS位置 | `{ "Longitude": 121.47, "Latitude": 31.23 }` |
| Paste | 粘贴文本 | `{ "Text": "hello" }` |
| SendClipboard | 发送剪贴板内容 | `{ "Text": "hello" }` |
| ModifySensor | 修改传感器数据 | `{ "Type": "accelerometer", "Values": [0.1, 0.2, 0.3], "Accuracy": 3 }` |
| Shake | 摇动设备 | `{}` |
| Blow | 吹气 | `{}` |
| SendTransMessage | 发送透传消息 | `{ "PackageName": "com.example", "Msg": "hello" }` |
| ModifyInstanceProperties | 修改实例属性 | 详见下方说明 |
| ModifyKeepFrontAppStatus | 修改保活应用状态 | `{ "PackageName": "com.example", "Enable": true, "RestartInterValSeconds": 3 }` |
| UnInstallByPackageName | 卸载应用 | `{ "PackageName": "com.example" }` |
| StartApp | 启动应用 | `{ "PackageName": "com.example", "ActivityName": "xxx" }` |
| StopApp | 停止应用 | `{ "PackageName": "com.example" }` |
| ClearAppData | 清除应用数据 | `{ "PackageName": "com.example" }` |
| EnableApp | 启用应用 | `{ "PackageName": "com.example" }` |
| DisableApp | 禁用应用 | `{ "PackageName": "com.example" }` |
| StartCameraMediaPlay | 开始摄像头媒体播放 | `{ "FilePath": "/sdcard/video.mp4", "Loops": 3 }` |
| DisplayCameraImage | 显示摄像头图像 | `{ "FilePath": "/sdcard/image.jpg" }` |
| AddKeepAliveList | 添加保活列表 | `{ "AppList": ["com.wechat", "com.alipay"] }` |
| RemoveKeepAliveList | 移除保活列表 | `{ "AppList": ["com.wechat"] }` |
| SetKeepAliveList | 设置保活列表 | `{ "AppList": ["com.wechat", "com.alipay"] }` |
| DescribeKeepFrontAppStatus | 获取保活应用状态 | `{}` |
| StopCameraMediaPlay | 停止摄像头播放 | `{}` |
| DescribeCameraMediaPlayStatus | 获取摄像头播放状态 | `{}` |
| DescribeKeepAliveList | 获取保活列表 | `{}` |
| ClearKeepAliveList | 清除保活列表 | `{}` |
| ListUserApps | 列出用户应用 | `{}` |
| Mute | 静音开关 | `{ "Mute": true }` |
| MediaSearch | 媒体库文件搜索 | `{ "Keyword": "abc" }` |
| Reboot | 重启实例 | `{}` |
| ListAllApps | 查询所有应用列表 | `{}` |
| MoveAppBackground | 关闭应用至后台 | `{}` |
| AddAppInstallBlackList | 新增应用安装黑名单 | `{ "AppList": ["com.xxx"] }` |
| RemoveAppInstallBlackList | 移除应用安装黑名单 | `{ "AppList": ["com.xxx"] }` |
| SetAppInstallBlackList | 覆盖应用安装黑名单 | `{ "AppList": ["com.xxx"] }` |
| DescribeAppInstallBlackList | 查询应用安装黑名单 | `{}` |
| ClearAppInstallBlackList | 清空应用安装黑名单 | `{}` |

> 详细参数结构和返回示例可参考头文件中的说明。

##### 典型输入示例

```json
{
  "TaskType": "ModifyResolution",
  "Params": {
    "cai-xxxx-xxx": { "Width": 1080, "Height": 1920 },
    "cai-yyyy-yyy": { "Width": 720, "Height": 1280 }
  }
}
```

##### 输出结果格式

返回值为一个JSON对象，key为实例ID，value为该实例的任务执行结果对象。例如：

```json
{
  "cai-xxxx-xxx": { "Code": 0 },
  "cai-yyyy-yyy": { "Code": 10100, "Msg": "operation denied" },
  "cai-zzzz-zzz": { "Code": -101, "Msg": "HTTP request failed" }
}
```

- `Code`为0表示成功，非0表示失败，`Msg`为失败原因。
- 某些查询类任务（如DescribeInstanceProperties、ListUserApps等）返回结构会包含详细数据，详见各任务类型在头文件中的说明。

##### 其他说明

- `output` 由SDK内部分配内存，使用完毕后需调用 `tcr_instance_free_result` 释放。
- 支持批量对多个实例下发同类型任务，便于大规模自动化管理。
- 返回的JSON字符串长度通过`output_size`返回。
- 任务类型及参数如有疑问，请参考接口注释或联系技术支持。

##### 返回值

- `TCR_SUCCESS`：任务请求成功
- `TCR_ERR_INVALID_PARAMS`：输入参数无效
#### 释放由tcr_instance_request分配的结果内存

释放由 tcr_instance_request 分配的结果内存。

```c
void tcr_instance_free_result(TcrAndroidInstance op, char* output);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| op | TcrAndroidInstance | 实例操作句柄 |
| output | char* | 需要释放的内存指针 |

#### 设置远端视频帧观察者

设置远端视频帧观察者，用于接收解码后的视频帧数据。

```c
void tcr_session_set_video_frame_observer(TcrSessionHandle session, const TcrVideoFrameObserver* observer);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| session | TcrSessionHandle | 会话句柄 |
| observer | const TcrVideoFrameObserver* | 视频帧观察者结构体指针 |

#### 连接到指定云端实例

连接到指定云端实例，建立单实例会话。

```c
void tcr_session_connect(TcrSessionHandle session, const char* instanceId);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| session | TcrSessionHandle | 会话句柄 |
| instanceId | const char* | 云端实例 ID |

#### 启动会话并连接云手机

启动会话并连接云手机，支持群控。

```c
void tcr_session_access(TcrSessionHandle session, const char** instanceIds, int32_t length, bool isGroupControl);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| session | TcrSessionHandle | 会话句柄 |
| instanceIds | const char** | 云端实例 ID 数组 |
| length | int32_t | 实例 ID 数组长度 |
| isGroupControl | bool | 是否群控 |

#### 暂停媒体流

暂停媒体流（如视频流），通常用于临时挂起。

```c
void tcr_session_pause_streaming(TcrSessionHandle session);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| session | TcrSessionHandle | 会话句柄 |

#### 恢复媒体流

恢复媒体流（如视频流），与暂停配合使用。

```c
void tcr_session_resume_streaming(TcrSessionHandle session);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| session | TcrSessionHandle | 会话句柄 |

#### 设置远端视频流参数

设置远端视频流参数（帧率、码率等）。

```c
void tcr_session_set_remote_video_profile(TcrSessionHandle session, int32_t fps, int32_t minBitrate, int32_t maxBitrate);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| session | TcrSessionHandle | 会话句柄 |
| fps | int32_t | 视频帧率 |
| minBitrate | int32_t | 最小码率（kbps） |
| maxBitrate | int32_t | 最大码率（kbps） |

#### 向云端应用发送透传消息

向云端应用发送透传消息。

```c
void tcr_session_send_trans_message(TcrSessionHandle session, const char* package_name, const char* msg);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| session | TcrSessionHandle | 会话句柄 |
| package_name | const char* | 应用包名 |
| msg | const char* | 消息内容 |

#### 将文本写入剪贴板并粘贴到输入框

将文本写入剪贴板并粘贴到输入框。

```c
void tcr_session_paste_text(TcrSessionHandle session, const char* msg);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| session | TcrSessionHandle | 会话句柄 |
| msg | const char* | 文本内容 |

#### 将文本直接输入到输入框

将文本直接输入到输入框。

```c
void tcr_session_input_text(TcrSessionHandle session, const char* content, const char* mode);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| session | TcrSessionHandle | 会话句柄 |
| content | const char* | 输入文本内容 |
| mode | const char* | 输入模式："append" 或 "override" |

#### 切换输入法

切换输入法。

```c
void tcr_session_switch_ime(TcrSessionHandle session, const char* ime);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| session | TcrSessionHandle | 会话句柄 |
| ime | const char* | 输入法类型："cloud" 或 "local" |

#### 开关摄像头

开关摄像头。

```c
void tcr_session_switch_camera(TcrSessionHandle session, const char* status);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| session | TcrSessionHandle | 会话句柄 |
| status | const char* | 状态："open" 或 "close" |

#### 开关麦克风

开关麦克风。

```c
void tcr_session_switch_mic(TcrSessionHandle session, const char* status);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| session | TcrSessionHandle | 会话句柄 |
| status | const char* | 状态："open" 或 "close" |

#### 发送键盘事件
向云端实例发送单个键盘事件（按键按下/抬起），用于模拟物理键盘操作。

```c
void tcr_session_send_keyboard_event(TcrSessionHandle session, int32_t keycode, bool down);
```

| 参数    | 类型             | 说明                                                                                      |
| ------- | ---------------- | ----------------------------------------------------------------------------------------- |
| session | TcrSessionHandle | 会话句柄                                                                                  |
| keycode | int32_t          | 按键码（建议使用十进制，可参考 [keycode 列表](https://www.toptal.com/developers/keycode) 或 Android KeyEvent 定义）|
| down    | bool             | true 表示按下（KeyDown），false 表示抬起（KeyUp）                                         |

**功能说明：**
该接口用于向云端实例发送单个按键事件，通常需要成对调用（即先发送按下事件，再发送抬起事件），以模拟真实物理键盘的操作。例如，模拟输入字母“A”时，应先调用一次 `down=true`，再调用一次 `down=false`。

**常用云手机按键 keycode 对照表（十进制）：**

| 按键名称         | keycode |
| ---------------- | ------- |
| 返回键           | 158     |
| 菜单键           | 139     |
| Home 键          | 172     |
| 音量加           | 58      |
| 音量减           | 59      |

**使用示例：**

模拟点击返回键：
```c
tcr_session_send_keyboard_event(session, 158, true);   // 按下
tcr_session_send_keyboard_event(session, 158, false);  // 抬起
```

**注意事项：**
- keycode 建议使用十进制，部分特殊按键可参考上表或 Android KeyEvent 定义。
- 若需模拟连续输入，请依次发送对应的按下/抬起事件。
- 该接口仅发送单个按键事件，复杂输入需自行组合多次调用。

#### 发送触摸屏触摸事件

发送触摸屏触摸事件。

```c
void tcr_session_touchscreen_touch(TcrSessionHandle session, float x, float y, int eventType, int width, int height, long timestamp);
```

| 参数 | 类型 | 说明 |
| ---- | ---- | ---- |
| session | TcrSessionHandle | 会话句柄 |
| x | float | 触摸点横坐标（像素） |
| y | float | 触摸点纵坐标（像素） |
| eventType | int | 事件类型（0: 按下, 1: 移动, 2: 抬起） |
| width | int | 屏幕宽度（像素） |
| height | int | 屏幕高度（像素） |
| timestamp | long | 事件时间戳（毫秒） |

## 结构体与回调说明

### TcrConfig

会话配置信息结构体，通常由业务后台调用云API `CreateAndroidInstancesAccessToken` 获取。

```c
typedef struct {
    const char* token;
    const char* accessInfo;
} TcrConfig;
```

- `token`：访问令牌字符串
- `accessInfo`：访问信息字符串

### TcrI420Buffer

I420 (YUV420P) 格式的视频帧缓冲区，包含亮度和色度分量。

```c
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
```
### TcrVideoRotation

视频旋转角度枚举。

```c
typedef enum {
    TCR_VIDEO_ROTATION_0 = 0,    ///< 不旋转(0度)
    TCR_VIDEO_ROTATION_90 = 90,  ///< 顺时针旋转90度
    TCR_VIDEO_ROTATION_180 = 180,///< 旋转180度
    TCR_VIDEO_ROTATION_270 = 270 ///< 顺时针旋转270度
} TcrVideoRotation;
```

### TcrVideoFrame

视频帧结构体，包含视频帧数据和相关时间戳信息。

```c
typedef struct {
    TcrI420Buffer i420_buffer;  ///< I420格式的视频数据缓冲区
    int64_t timestamp_us;       ///< 视频帧时间戳(微秒)
    TcrVideoRotation rotation;  ///< 视频帧需要旋转的角度
    uint32_t timestamp_rtp;     ///< RTP时间戳(90kHz时钟)
    int64_t ntp_time_ms;        ///< NTP网络时间戳(毫秒)
} TcrVideoFrame;
```
### TcrUploadProgressCallback

所有文件上传进度回调函数指针定义。

```c
typedef void (*TcrUploadProgressCallback)(
    const char* instance_id,
    double total_bytes,
    double transferred_bytes,
    void* user_data);
```

- `instance_id`：当前操作的实例ID
- `total_bytes`：文件总大小（字节）
- `transferred_bytes`：已传输的大小（字节）
- `user_data`：用户自定义数据指针

### TcrUploadCallback

上传回调结构体，包含进度回调函数和用户数据。

```c
typedef struct {
    TcrUploadProgressCallback on_progress;
    void* user_data;
} TcrUploadCallback;
```

- `on_progress`：进度回调函数指针
- `user_data`：用户自定义数据指针
**示例代码**
```c
void my_upload_progress(const char* instance_id, double total, double transferred, void* user_data) {
    // 处理上传进度
}
TcrUploadCallback upload_cb = { .on_progress = my_upload_progress, .user_data = NULL };
```

### TcrSessionEvent

会话事件类型枚举，定义了客户端与云端会话过程中可能产生的所有事件类型。

```c
typedef enum {
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
    TCR_SESSION_EVENT_IME_STATUS_CHANGE = 12
} TcrSessionEvent;
```

> 具体每个事件的 JSON 数据结构详见头文件注释。
### TcrLogLevel

日志级别枚举。

```c
typedef enum TcrLogLevel {
    TCR_LOG_LEVEL_TRACE = 0,
    TCR_LOG_LEVEL_DEBUG = 1,
    TCR_LOG_LEVEL_INFO  = 2,
    TCR_LOG_LEVEL_WARN  = 3,
    TCR_LOG_LEVEL_ERROR = 4
} TcrLogLevel;
```

### TcrLogCallback

日志回调结构体。

```c
typedef struct TcrLogCallback {
    void* user_data;
    void (*on_log)(void* user_data, TcrLogLevel level, const char* tag, const char* log);
} TcrLogCallback;
```

- `user_data`：用户自定义数据指针
- `on_log`：日志回调函数指针，参数为用户数据、日志级别、日志标签、日志内容
**示例代码**
```c
void my_log_callback(void* user_data, TcrLogLevel level, const char* tag, const char* log) {
    // 处理日志
}
TcrLogCallback log_cb = { .user_data = NULL, .on_log = my_log_callback };
tcr_set_log_callback(&log_cb);
```