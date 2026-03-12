# CloudStream_QtQuick_Demo

基于 Qt Quick 的云手机 TcrSdk 客户端演示工程

## 项目简介

CloudStream_QtQuick_Demo 是一个基于 Qt 6/QML 的云手机串流客户端演示工程，集成了 [TcrSdk](https://cg-sdk-1258344699.cos.ap-nanjing.myqcloud.com/CloudDeviceWinSDK/docs/release_note.html) 云手机 SDK，支持多视频流渲染、触摸/按键控制等功能。支持 **Windows** 和 **macOS** 平台，便于企业或开发者参考如何使用 TcrSdk 以及与业务后台进行交互。

## 主要功能

- **实例展示**：卡片展示云手机实例画面
- **视频串流**：支持单实例和多实例同步视频流渲染与交互
- **自定义渲染**：基于 OpenGL Shader 的 YUV 视频渲染
- **跨平台支持**：Windows（MSVC）与 macOS（Apple Silicon / Intel）

## 目录结构

```
.
├── CMakeLists.txt                # CMake 构建脚本（支持 Windows/macOS）
├── Info.plist.in                 # macOS 隐私权限声明模板（摄像头/麦克风）
├── config.json                   # 业务配置文件
├── docs/                         # 文档图片
│   └── images/
├── qml/                          # QML 界面文件
│   ├── InstanceTokenWindow.qml   # 启动页，演示如何通过实例ID获取AccessToken
│   ├── MainWindow.qml            # 主界面（云手机实例画面展示）
│   ├── StreamingWindow.qml       # 串流窗口（演示如何连接到云机实例并进行交互）
│   └── components/               # QML 复用组件
│       └── Dialogs.qml           # 对话框
├── README.md                     # 本文档
├── shaders/                      # OpenGL 着色器
│   ├── yuv.frag                  # YUV 片段着色器
│   └── yuv.vert                  # YUV 顶点着色器
├── src/                          # C++ 源码
│   ├── main.cpp                  # 程序入口
│   ├── core/                     # 业务核心与渲染
│   │   └── video/                # 视频渲染相关
│   │       ├── Frame.h           # 视频帧数据结构
│   │       ├── VideoRenderItem.h # QQuickItem 视频渲染组件
│   │       ├── VideoRenderItem.cpp
│   │       ├── YuvMaterial.h     # OpenGL YUV 着色材质
│   │       ├── YuvMaterial.cpp
│   │       ├── YuvNode.h         # 场景图 YUV 渲染节点
│   │       ├── YuvNode.cpp
│   │       ├── YuvTestPattern.h  # YUV 测试模式工具
│   │       └── YuvTestPattern.cpp
│   ├── services/                 # 网络与 API 服务
│   │   ├── ApiService.h          # 云手机 API 服务接口
│   │   ├── ApiService.cpp
│   │   └── NetworkService.h      # 网络请求服务接口
│   │   ├── NetworkService.cpp
│   ├── utils/                    # 工具类
│   │   ├── Logger.h              # 日志工具
│   │   ├── Logger.cpp
│   │   ├── VariantListConverter.h
│   │   └── VariantListConverter.cpp
│   └── viewmodels/               # QML 绑定的 ViewModel
│       ├── InstanceTokenViewModel.h   # 演示如何请求业务后台接口拿到 AccessToken
│       ├── InstanceTokenViewModel.cpp
│       ├── StreamingViewModel.h   # 串流交互控制管理
│       └── StreamingViewModel.cpp
└── third_party/
    └── TcrSdk/                   # 云手机 SDK（含头文件及各平台库）
        ├── include/              # 公共头文件（跨平台）
        │   ├── tcr_c_api.h
        │   ├── tcr_types.h
        │   └── tcr_export.h
        ├── win/                  # Windows 平台
        │   ├── Release/
        │   │   ├── x64/          # TcrSdk.dll / TcrSdk.lib / TcrSdk.pdb
        │   │   └── Win32/
        │   └── Debug/
        │       ├── x64/
        │       └── Win32/
        └── macos/                # macOS 平台（Universal Binary）
            └── lib/
                └── libTcrSdk.dylib
```

## 依赖环境

### Windows

- **编译器**：`MSVC 2022 64-bit`
- **Qt 6.8+**（需包含 Quick、Network、Concurrent、ShaderTools 模块）
- **CMake 3.16+**
- **TcrSdk**：请从[TcrSdk发布记录](https://github.com/tencentyun/cloudgame-windows-sdk/blob/main/Docs/Release_Note.md)下载 Windows SDK，解压后将 `win/` 目录和 `include/` 目录拷贝到 `third_party/TcrSdk/` 下

### macOS

- **Xcode 14.0+**（需安装 Command Line Tools）
- **Qt 6.8+**（需包含 macOS Kit，支持 Apple Silicon 或 Intel）
- **CMake 3.16+**
- **TcrSdk**：请从[TcrSdk发布记录](https://github.com/tencentyun/cloudgame-windows-sdk/blob/main/Docs/Release_Note.md)下载 macOS Universal SDK，解压后将 `macos/` 目录和 `include/` 目录拷贝到 `third_party/TcrSdk/` 下

## 构建方法

### Windows

#### 1. 安装 Visual Studio（MSVC 编译器）

- 如果尚未安装，请前往 [Visual Studio 2022 官方下载页面](https://visualstudio.microsoft.com/zh-hans/downloads/)下载安装包。
- 安装过程中**务必勾选"使用C++的桌面开发"**，以确保 MSVC 编译器和相关工具链被正确安装。

---

#### 2. 安装 Qt 6 的 MSVC 版本

- 启动 **Qt Maintenance Tool** 或 **Qt Online Installer**。
- 选择"添加或删除组件"。
- 在"Qt 6.x.x"下，找到与你的 Visual Studio 版本对应的 MSVC 组件，例如：
  - `MSVC 2022 64-bit`
- 勾选所需的 MSVC 版本，点击"下一步"完成安装。
- ![MSVC 组件选择界面](docs/images/Qt_Download.png)

---

#### 3. 在 Qt Creator 里选择 MSVC Kit

- 安装完成后，重启 **Qt Creator**。
- 打开"工具"→"选项"→"Kits"（或"构建套件"）。
- 在"自动检测"区域应能看到类似 `Desktop Qt 6.x.x MSVC 2022 64-bit` 的 Kit。
- 新建项目或打开本项目时，选择对应的 MSVC Kit 进行构建。
- ![MSVC Kit 选择界面](docs/images/Qt_config.png)

---

#### 4. 使用 Qt Creator 打开并编译项目

1. 启动 **Qt Creator**，点击"打开项目"，选择本项目根目录下的 `CMakeLists.txt` 文件。
2. 在弹出的"配置项目"界面，选择已安装的 MSVC Kit（如 `Desktop Qt 6.x.x MSVC 2022 64-bit`）。
3. 点击"配置项目"并等待 Qt Creator 完成 CMake 配置。
4. 在左侧"项目"面板中，点击"构建"按钮（或按 `Ctrl+B`）进行编译。
5. 编译完成后，可直接点击"运行"按钮（或按 `Ctrl+R`）启动程序。

> **注意：** 运行前请确保构建的套件是 `MSVC 2022 64-bit`

---

### macOS

#### 1. 安装 Xcode 及 Command Line Tools

```bash
xcode-select --install
```

或从 Mac App Store 下载安装完整的 Xcode（14.0+）。

---

#### 2. 安装 Qt 6 的 macOS Kit

- 启动 **Qt Maintenance Tool** 或 **Qt Online Installer**。
- 在"Qt 6.x.x"下，找到 macOS 组件：
  - `macOS` (支持 Apple Silicon / Intel Universal Binary)
- 勾选并完成安装。

---

#### 3. 在 Qt Creator 里选择 macOS Kit

- 重启 **Qt Creator**。
- 打开"工具"→"选项"→"Kits"。
- 在"自动检测"区域应能看到类似 `Desktop Qt 6.x.x for macOS` 的 Kit。
- 打开本项目时选择该 Kit。

---

#### 4. 使用 Qt Creator 打开并编译项目

1. 启动 **Qt Creator**，点击"打开项目"，选择 `CMakeLists.txt`。
2. 在"配置项目"界面选择 macOS Kit。
3. 点击"配置项目"等待 CMake 配置完成。
4. 点击"构建"（`Cmd+B`）进行编译。
5. 编译完成后点击"运行"（`Cmd+R`）启动程序。

> **注意：**
> - 首次运行时，macOS 可能弹出摄像头/麦克风权限授权对话框，请点击"允许"。
> - 如遇到 `libTcrSdk.dylib` 无法加载，请确认 `third_party/TcrSdk/macos/lib/` 目录下存在该文件。

---

## 常见问题

- **Q: 运行时提示找不到 `TcrSdk.dll` / `libTcrSdk.dylib`？**
  A: 请确认 SDK 文件已正确拷贝到 `third_party/TcrSdk/` 对应平台子目录下，并重新编译。

- **Q: macOS 访问摄像头/麦克风时提示权限被拒绝？**
  A: 请在"系统设置"→"隐私与安全性"中，找到对应权限（摄像头/麦克风），确认已允许该应用访问。或检查 `Info.plist.in` 中是否已包含 `NSCameraUsageDescription` / `NSMicrophoneUsageDescription` 字段并重新编译。

- **Q: macOS 构建报错 `TcrSdk dynamic library not found`？**
  A: 请确认 `third_party/TcrSdk/macos/lib/` 目录下存在 `libTcrSdk.dylib` 文件。

- **Q: 运行过程中出现其他错误，如连接云手机失败？**
  A: 请在 logs 目录下找到 `app.log` 文件，并将文件反馈给云渲染开发团队。
