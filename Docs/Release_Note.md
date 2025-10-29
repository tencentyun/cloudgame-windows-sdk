# Release Notes

## Version 1.13.2
### Bugfix
- 修复个别工程加载程序时崩溃的问题

### Download
- [TcrSdk-Debug-1.13.2-20251029-a7e7a38b1](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Debug-1.13.2-20251029-a7e7a38b1.zip)
- [TcrSdk-Release-1.13.2-20251029-a7e7a38b1](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Release-1.13.2-20251029-a7e7a38b1.zip)

## Version 1.13.1
### Features
1. **视频帧回调与数据结构调整**
   - 视频帧回调由原来的**直接buffer回调**（`on_frame_buffer`）调整为基于 **handle/引用计数的 frame handle 回调**（`on_frame` & `TcrVideoFrameHandle`）。
   - 新增了与引用计数相关的接口（`add_ref`/`release`），以及新的访问接口（`get_buffer`）。
   - `TcrVideoFrameBuffer` 结构体得到了扩展，新增帧时间戳、旋转角度、实例ID/索引等字段。
2. **接口变动情况**
   - `tcr_session_create_data_channel`：新增 `type` 参数。可选为 `android` 或 `android_broadcast`，后者支持广播消息至群控会话内所有被控实例。
   - `tcr_session_enable_local_microphone`：原本的启用/禁用操作现已合并为一个 enable 布尔参数。
   - 新增 `tcr_session_get_request_id`：用于获取请求 ID。
   - 新增 `tcr_session_config_default`：返回默认会话配置，确保默认参数安全有效。
   - `TcrSessionConfig` 新增 `enable_audio` 字段，可在小流应用场景屏蔽音频模块以降低 CPU 开销。
3. **Observer 生命周期管理加强**
   - 明确要求observer结构体由业务方持有, 在`TcrSession`的整个生命周期全程保持，销毁`TcrSession`前需先置空处理。

### Download
- [TcrSdk-Debug-1.13.1-20251022-ab382a72f](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Debug-1.13.1-20251022-ab382a72f.zip)
- [TcrSdk-Release-1.13.1-20251022-ab382a72f](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Release-1.13.1-20251022-ab382a72f.zip)

## Version 1.12.0
### Features
- 支持新的前后端串流架构
- `tcr_client_create_session`新增一个参数`TcrSessionConfig`支持传入`user_id`

### Download
- [TcrSdk-Debug-1.12.0-20250917-a558cd6ac](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Debug-1.12.0-20250917-a558cd6ac.zip)
- [TcrSdk-Release-1.12.0-20250917-a558cd6ac](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Release-1.12.0-20250917-a558cd6ac.zip)

## Version 1.10.8
### Features
- 支持单独控制音频视频暂停和恢复接口
- 支持变更云端视频流分辨率
- 单连走新鉴权逻辑

### Download
- [TcrSdk-Debug-1.10.8-20250904-a54add26f](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Debug-1.10.8-20250904-a54add26f.zip)
- [TcrSdk-Release-1.10.8-20250904-a54add26f](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Release-1.10.8-20250904-a54add26f.zip)

## Version 1.10.5
### Features
- 支持云手机摄像头上行能力
- 支持云手机麦克风上行能力

### Download
- [TcrSdk-Release-1.10.5-20250829-2741552e0](https://cg-sdk-1258344699.cos-internal.ap-nanjing.tencentcos.cn/CloudDeviceWinSDK/sdk/TcrSdk-Release-1.10.5-20250829-2741552e0.zip)
- [TcrSdk-Debug-1.10.5-20250829-2741552e0](https://cg-sdk-1258344699.cos-internal.ap-nanjing.tencentcos.cn/CloudDeviceWinSDK/sdk/TcrSdk-Debug-1.10.5-20250829-2741552e0.zip)


## Version 1.8.0
### Features
- 支持云手机串流和设备控制
- 支持批量获取云手机截图以及批量云手机控制
- 支持上传文件到云机和获取云机文件下载路径

### Download
- [TcrSdk-Debug-1.8.0-20250727-65e8d51dd.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Debug-1.8.0-20250727-65e8d51dd.zip)
- [TcrSdk-Release-1.8.0-20250727-65e8d51dd.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Release-1.8.0-20250727-65e8d51dd.zip)