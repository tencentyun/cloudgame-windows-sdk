# Qt Quick Demo 快速开始

## 准备 TcrSdk 动态库

### 方式一：从 TcrSdk 项目构建

```bash
# 1. 构建 TcrSdk
cd TcrSdkProject
VERSION=1.2.3 ./scripts/build-macos-sdk.sh Release arm64

# 2. 复制到 Demo 项目
cd ../cloudgame-windows-sdk/CloudStream/CloudStream_QtQuick_Demo
cp -P ../../../../TcrSdkProject/TcrSdk/out/TcrSdk/lib/*.dylib \
    third_party/TcrSdk/macos/lib/
```

### 方式二：使用现有的 SDK 包

```bash
# 解压 SDK 包
unzip TcrSdk-Release-arm64-1.2.3-*.zip

# 复制到 Demo 项目
cp -P TcrSdk/lib/*.dylib \
    cloudgame-windows-sdk/CloudStream/CloudStream_QtQuick_Demo/third_party/TcrSdk/macos/lib/
```

## 构建项目

### 使用 Qt Creator（推荐）

1. 打开 Qt Creator
2. 打开项目：`CloudStream_QtQuick_Demo/CMakeLists.txt`
3. 选择 Kit（确保使用 Qt 6.8+）
4. 点击构建按钮

**重要提示：** 首次构建后，macdeployqt 会自动部署 Qt 插件和框架。

### 使用命令行

```bash
cd cloudgame-windows-sdk/CloudStream/CloudStream_QtQuick_Demo

# 配置
cmake -B build -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_PREFIX_PATH=/Users/cyy/Qt/6.9.0/macos

# 构建（macdeployqt 会自动运行）
cmake --build build

# 运行
open build/QtQuick_Demo.app
```

## macdeployqt 说明

构建过程会自动运行 `macdeployqt` 来部署 Qt 依赖：

### macdeployqt 会做什么？

1. **复制 Qt 框架**：将所有需要的 Qt 框架复制到 `Contents/Frameworks/`
2. **复制插件**：将 Qt 插件复制到 `Contents/PlugIns/`
   - `platforms/libqcocoa.dylib`（必需，用于 macOS 窗口系统）
   - `styles/libqmacstyle.dylib`（macOS 样式）
   - `imageformats/`（图像格式支持）
   - 等等
3. **修复依赖路径**：更新所有 dylib 的 rpath 和依赖路径
4. **创建 qt.conf**：配置 Qt 插件和资源路径

### 验证 macdeployqt 是否成功

```bash
# 检查插件是否存在（最重要）
ls build/QtQuick_Demo.app/Contents/PlugIns/platforms/libqcocoa.dylib

# 检查 Qt 框架
ls build/QtQuick_Demo.app/Contents/Frameworks/QtCore.framework

# 检查 qt.conf
cat build/QtQuick_Demo.app/Contents/Resources/qt.conf
```

应该看到类似输出：
```
[Paths]
Plugins = PlugIns
Imports = Resources/qml
QmlImports = Resources/qml
```

## 验证部署

### 检查 dylib 是否在应用包中

```bash
ls -lh build/QtQuick_Demo.app/Contents/Frameworks/libTcrSdk*
```

应该看到：
```
libTcrSdk.1.2.3.dylib  （实际文件）
libTcrSdk.1.dylib      → libTcrSdk.1.2.3.dylib
libTcrSdk.dylib        → libTcrSdk.1.dylib
```

### 检查链接

```bash
otool -L build/QtQuick_Demo.app/Contents/MacOS/QtQuick_Demo | grep -i tcr
```

应该看到：
```
@rpath/libTcrSdk.1.dylib (compatibility version 1.0.0, current version 1.2.3)
```

### 检查 rpath

```bash
otool -l build/QtQuick_Demo.app/Contents/MacOS/QtQuick_Demo | grep -A2 LC_RPATH
```

应该看到：
```
cmd LC_RPATH
path @executable_path/../Frameworks
```

## 故障排查

### 问题：TcrSdk dynamic library not found

**原因：** `third_party/TcrSdk/macos/lib/` 目录中没有 dylib 文件

**解决：**
```bash
# 检查是否只有静态库
ls -lh third_party/TcrSdk/macos/lib/

# 如果只有 libTcrSdk.a，需要复制 dylib 文件
cp -P /path/to/TcrSdk/lib/*.dylib third_party/TcrSdk/macos/lib/
```

### 问题：应用启动时崩溃，提示 "createPlatformIntegration"

**原因：** Qt 平台插件没有被部署（macdeployqt 失败）

**症状：**
```
QMessageLogger::fatal(char const*, ...) const
QGuiApplicationPrivate::createPlatformIntegration()
```

**解决方案 1：重新构建**
```bash
# 完全清理并重新构建
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_PREFIX_PATH=/Users/cyy/Qt/6.9.0/macos
cmake --build build
```

**解决方案 2：手动运行 macdeployqt**
```bash
cd build
/Users/cyy/Qt/6.9.0/macos/bin/macdeployqt QtQuick_Demo.app -verbose=1
```

**解决方案 3：检查 Qt 插件**
```bash
# 如果 PlugIns 目录不存在或为空，说明 macdeployqt 失败
ls build/QtQuick_Demo.app/Contents/PlugIns/platforms/

# 手动运行 macdeployqt 修复
/Users/cyy/Qt/6.9.0/macos/bin/macdeployqt build/QtQuick_Demo.app
```

### 问题：运行时找不到 libTcrSdk.dylib

**原因：** dylib 没有被复制到应用包，或 rpath 设置不正确

**解决：**
```bash
# 检查应用包
ls -lh build/QtQuick_Demo.app/Contents/Frameworks/

# 如果没有 libTcrSdk*.dylib，重新构建
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_PREFIX_PATH=/Users/cyy/Qt/6.9.0/macos
cmake --build build
```

### 问题：macdeployqt 找不到

**原因：** Qt 安装路径不正确或 macdeployqt 不在 PATH 中

**解决：**
```bash
# 检查 macdeployqt 是否存在
which macdeployqt
# 或
ls /Users/cyy/Qt/6.9.0/macos/bin/macdeployqt

# 如果不存在，确保 Qt 安装完整
# 重新安装 Qt 6.9.0 或更高版本
```

## 目录结构

构建成功后的应用包结构：

```
QtQuick_Demo.app/
├── Contents/
    ├── MacOS/
    │   └── QtQuick_Demo          （可执行文件）
    ├── Frameworks/
    │   ├── libTcrSdk.1.2.3.dylib （TcrSdk 实际文件）
    │   ├── libTcrSdk.1.dylib     （符号链接）
    │   ├── libTcrSdk.dylib       （符号链接）
    │   ├── QtCore.framework      （Qt 框架）
    │   ├── QtGui.framework
    │   ├── QtQuick.framework
    │   └── ...
    ├── PlugIns/                  （Qt 插件，必需！）
    │   ├── platforms/
    │   │   └── libqcocoa.dylib   （macOS 平台插件）
    │   ├── styles/
    │   ├── imageformats/
    │   └── ...
    └── Resources/
        ├── qt.conf               （Qt 配置文件）
        └── ...
```

## 运行时行为

1. **启动应用**：`QtQuick_Demo` 可执行文件开始运行
2. **加载 Qt 插件**：Qt 读取 `qt.conf`，从 `PlugIns/` 加载插件
3. **初始化平台**：加载 `libqcocoa.dylib` 创建 macOS 窗口系统
4. **解析 rpath**：系统解析 `@rpath` 为 `@executable_path/../Frameworks`
5. **加载 TcrSdk**：从 `Frameworks/libTcrSdk.1.dylib` 加载
6. **加载 Qt 框架**：从相同目录加载 Qt 框架

应用包是**自包含的**，可以独立分发和运行，无需用户安装 Qt 或 TcrSdk。

## 更多信息

- CMakeLists.txt 配置说明：`README_TCRSDK.md`
- TcrSdk 构建文档：`../../../../TcrSdkProject/scripts/README.md`

