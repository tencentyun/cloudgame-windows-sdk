# Release Notes

## Version 1.18.5
### Features
- 优化ICE配置并使用更宽松的网络切换策略

### Download
- [TcrSdk-Debug-1.18.5-20251224-166f4103c.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Debug-1.18.5-20251224-166f4103c.zip)
- [TcrSdk-Release-1.18.5-20251224-166f4103c.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Release-1.18.5-20251224-166f4103c.zip)


## Version 1.18.4
### Features
- 优化缓冲策略减少内存占用

### Download
- [TcrSdk-Debug-1.18.4-20251221-a0858abe6.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Debug-1.18.4-20251221-a0858abe6.zip)
- [TcrSdk-Release-1.18.4-20251221-a0858abe6.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Release-1.18.4-20251221-a0858abe6.zip)

## Version 1.18.1
### Features
- 支持`tcr_session_access_multi_stream`调用后默认不拉流在`tcr_session_resume_streaming`调用后开始拉流
- `tcr_session_config_default`返回的`TcrStreamProfile`支持单独设置长短边大小另一边等比缩放

### Download
- [TcrSdk-Debug-1.18.1-20251219-790d9f7fa.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Debug-1.18.1-20251219-790d9f7fa.zip)
- [TcrSdk-Release-1.18.1-20251219-790d9f7fa.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Release-1.18.1-20251219-790d9f7fa.zip)

## Version 1.17.5
### Features
- 回调的`TCR_SESSION_EVENT_CLIENT_STATS`数据中支持`edge_rtt`
- `TcrSessionConfig`中新增`statsInterval`参数控制TCR_SESSION_EVENT_CLIENT_STATS 触发间隔（单位秒, 范围1-60，默认1秒）

### Download
- [TcrSdk-Debug-1.17.5-20251211-afa0cbab5.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Debug-1.17.5-20251211-afa0cbab5.zip)
- [TcrSdk-Release-1.17.5-20251211-afa0cbab5.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Release-1.17.5-20251211-afa0cbab5.zip)

## Version 1.17.4
### Features
- 新增释放全局资源的`tcr_client_release()`接口
- 回调的`TCR_SESSION_EVENT_CLIENT_STATS`数据中支持`rtt`和`raw_rtt`

### Download
- [TcrSdk-Debug-1.17.4-20251210-5ca177064.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Debug-1.17.4-20251210-5ca177064.zip)
- [TcrSdk-Release-1.17.4-20251210-5ca177064.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Release-1.17.4-20251210-5ca177064.zip)

## Version 1.16.7
### Bugfix
- 修复个别机型进程退出时的崩溃问题
### Download
- [TcrSdk-Debug-1.16.7-20251204-7471714df.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Debug-1.16.7-20251204-7471714df.zip)
- [TcrSdk-Release-1.16.7-20251204-7471714df.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Release-1.16.7-20251204-7471714df.zip)

## Version 1.17.3
### Bugfix
- 修复底层库关闭会话时的崩溃问题

### Download
- [TcrSdk-Debug-1.17.3-20251202-fa7421f20.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Debug-1.17.3-20251202-fa7421f20.zip)
- [TcrSdk-Release-1.17.3-20251202-fa7421f20.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Release-1.17.3-20251202-fa7421f20.zip)

## Version 1.17.2
### Features
- 接口`tcr_session_set_remote_video_profile`支持指定实例ID
- 减少日志打印

### Download
- [TcrSdk-Debug-1.17.2-20251128-84106dcfd.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Debug-1.17.2-20251128-84106dcfd.zip)
- [TcrSdk-Release-1.17.2-20251128-84106dcfd.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Release-1.17.2-20251128-84106dcfd.zip)

## Version 1.16.1
### Bugfix
- 群控和单连没有触发`TCR_SESSION_EVENT_SCREEN_CONFIG_CHANGE`消息的问题

### Download
- [TcrSdk-Debug-1.16.1-20251126-520c20b23.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Debug-1.16.1-20251126-520c20b23.zip)
- [TcrSdk-Release-1.16.1-20251126-520c20b23.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Release-1.16.1-20251126-520c20b23.zip)

## Version 1.16.0
### Features
- 接口`tcr_session_resume_streaming`和`tcr_session_pause_streaming`支持传递实例ID

### Download
- [TcrSdk-Debug-1.16.0-20251125-3d00776ab.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Debug-1.16.0-20251125-3d00776ab.zip)
- [TcrSdk-Release-1.16.0-20251125-3d00776ab.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Release-1.16.0-20251125-3d00776ab.zip)


## Version 1.15.2
### Features
- 新增接口`tcr_session_touchscreen_touch_ex`接口

### Download
- [TcrSdk-Debug-1.15.2-20251125-0df05f8ee.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Debug-1.15.2-20251125-0df05f8ee.zip)
- [TcrSdk-Release-1.15.2-20251125-0df05f8ee.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Release-1.15.2-20251125-0df05f8ee.zip)

## Version 1.15.0
### Features
- 支持单会话拉最多路30路实例画面
- 新增获取观察者配置方法`tcr_video_frame_observer_default()`、`tcr_session_observer_default()`、`tcr_video_config_default()`、`tcr_data_channel_observer_default()`

### Bugfix
- 修复`tcr_instance_set_master`不生效问题

### Download
- [TcrSdk-Debug-1.15.0-20251118-1830e0fb8.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Debug-1.15.0-20251118-1830e0fb8.zip)
- [TcrSdk-Release-1.15.0-20251118-1830e0fb8.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Release-1.15.0-20251118-1830e0fb8.zip)

## Version 1.14.1
### Bugfix
- 删除`TcrSessionConfig`中流配置的unit规避指针传递时值无效问题

### Download
- [TcrSdk-Debug-1.14.1-20251114-281f04ccd.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Debug-1.14.1-20251114-281f04ccd.zip)
- [TcrSdk-Release-1.14.1-20251114-281f04ccd.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Release-1.14.1-20251114-281f04ccd.zip)

## Version 1.14.0
### Bugfix
- 修复`tcr_session_pause_streaming`无法停止和恢复音频问题

### Features
- 新增函数`tcr_config_default()`用于获取客户端默认配置
- `TcrSessionConfig`新增参数`enable_hardware_decode`支持输出硬解RGBA纹理

### Download
- [TcrSdk-Debug-1.14.0-20251113-177ec1791.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Debug-1.14.0-20251113-177ec1791.zip)
- [TcrSdk-Release-1.14.0-20251113-177ec1791.zip](https://tcrsdk.tencent-cloud.com/CloudDeviceWinSDK/sdk/TcrSdk-Release-1.14.0-20251113-177ec1791.zip)

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