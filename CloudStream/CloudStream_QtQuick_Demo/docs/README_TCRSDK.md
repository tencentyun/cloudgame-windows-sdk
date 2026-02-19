# QtQuick Demo - TcrSdk 集成说明

本项目是一个使用 TcrSdk 的 Qt Quick 示例应用，**仅支持动态库链接**。

## 目录结构

```
CloudStream_QtQuick_Demo/
├── CMakeLists.txt           # 构建配置（已简化）
├── src/                     # 源代码
├── qml/                     # QML 文件
├── shaders/                 # 着色器文件
└── third_party/
    └── TcrSdk/             # TcrSdk 动态库
        ├── win/            # Windows 平台
        │   ├── Debug/
        │   │   ├── x64/
        │   │   │   ├── TcrSdk.dll
        │   │   │   ├── TcrSdk.lib
        │   │   │   └── TcrSdk.pdb
        │   │   └── Win32/
        │   │       ├── TcrSdk.dll
        │   │       ├── TcrSdk.lib
        │   │       └── TcrSdk.pdb
        │   └── Release/
        │       ├── include/    # 头文件
        │       ├── x64/
        │       │   ├── TcrSdk.dll
        │       │   ├── TcrSdk.lib
        │       │   └── TcrSdk.pdb
        │       └── Win32/
        │           ├── TcrSdk.dll
        │           ├── TcrSdk.lib
        │           └── TcrSdk.pdb
        └── macos/          # macOS 平台
            ├── include/    # 头文件
            └── lib/
                ├── libTcrSdk.dylib
                ├── libTcrSdk.1.dylib
                └── libTcrSdk.1.2.3.dylib
```

## 构建配置特点

### 简化的设计

CMakeLists.txt 已经简化，**仅支持动态库**：
- ✅ 支持 Windows 动态库（.dll + .lib）
- ✅ 支持 macOS 动态库（.dylib）
- ❌ 不支持静态库（.a）
- ❌ 不支持 macOS framework

### Windows 平台

**自动处理：**
- 自动检测架构（x64 或 Win32）
- 自动选择 Debug 或 Release 版本
- 自动复制 DLL 和 PDB 到构建目录

**目录约定：**
- 头文件：`third_party/TcrSdk/win/Release/include/`
- 库文件：`third_party/TcrSdk/win/[Debug|Release]/[x64|Win32]/`

### macOS 平台

**自动处理：**
- 自动查找 dylib 文件（支持版本化文件名）
- 自动复制所有 dylib 文件（包括符号链接）到应用包
- 自动设置 rpath（`@executable_path/../Frameworks`）
- 自动运行 macdeployqt 部署 Qt 框架

**目录约定：**
- 头文件：`third_party/TcrSdk/macos/include/`
- 库文件：`third_party/TcrSdk/macos/lib/`

**版本化支持：**
脚本会自动识别版本化的 dylib 文件：
- `libTcrSdk.1.2.3.dylib` （实际文件）
- `libTcrSdk.1.dylib` （符号链接）
- `libTcrSdk.dylib` （符号链接）

所有文件都会被复制到应用包中，保持符号链接结构。

## 构建步骤

### Windows

```cmd
# 配置
cmake -S . -B build -G "Visual Studio 17 2022" -A x64

# 构建 Release
cmake --build build --config Release

# 构建 Debug
cmake --build build --config Debug
```

构建完成后，`TcrSdk.dll` 和 `TcrSdk.pdb` 会自动复制到可执行文件目录。

### macOS

```bash
# 配置
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# 构建
cmake --build build

# 运行
open build/QtQuick_Demo.app
```

构建完成后：
1. 所有 `libTcrSdk*.dylib` 文件会被复制到 `QtQuick_Demo.app/Contents/Frameworks/`
2. macdeployqt 会自动打包 Qt 框架
3. 应用包可以独立运行，无需额外配置

## 部署 TcrSdk

### 从 TcrSdk 项目构建

如果你有 TcrSdk 源码：

**Windows:**
```cmd
cd TcrSdkProject
scripts\build-win-sdk.bat Release

# 复制输出到 Demo 项目
xcopy TcrSdk\out\TcrSdk\* ..\cloudgame-windows-sdk\CloudStream\CloudStream_QtQuick_Demo\third_party\TcrSdk\win\Release\ /E /Y
```

**macOS:**
```bash
cd TcrSdkProject
VERSION=1.2.3 ./scripts/build-macos-sdk.sh Release arm64

# 解压并复制到 Demo 项目
unzip TcrSdk/out/TcrSdk-Release-arm64-1.2.3-*.zip
cp -R TcrSdk/* ../cloudgame-windows-sdk/CloudStream/CloudStream_QtQuick_Demo/third_party/TcrSdk/macos/
```

### 手动部署

1. 将 TcrSdk 构建产物放到 `third_party/TcrSdk/` 目录
2. 确保目录结构符合上述约定
3. 重新运行 CMake 配置

## 故障排查

### Windows

**问题：找不到 TcrSdk.dll**
- 检查 `third_party/TcrSdk/win/Release/[x64|Win32]/` 目录是否存在
- 确认 DLL 文件在正确的架构目录中

**问题：链接错误**
- 确认 `TcrSdk.lib` 存在
- 检查构建类型（Debug/Release）是否匹配

### macOS

**问题：找不到 libTcrSdk.dylib**
- 检查 `third_party/TcrSdk/macos/lib/` 目录
- 确认至少有一个 `libTcrSdk*.dylib` 文件

**问题：运行时找不到 dylib**
- 检查应用包：`QtQuick_Demo.app/Contents/Frameworks/`
- 应该包含所有 TcrSdk dylib 文件
- 使用 `otool -L` 检查依赖：
  ```bash
  otool -L build/QtQuick_Demo.app/Contents/MacOS/QtQuick_Demo
  ```

**问题：符号链接没有被复制**
- CMake 配置会自动复制所有符号链接
- 如果手动复制，使用 `cp -P` 保留符号链接

## 与旧版本的区别

### 移除的功能

1. **静态库支持**
   - 移除了 `libTcrSdk.a` 的处理逻辑
   - 移除了静态库所需的系统框架链接
   - 移除了 OpenSSL 依赖查找

2. **Framework 支持**
   - 移除了 `TcrSdk.framework` 的处理逻辑
   - 简化为只支持 dylib

3. **复杂的条件判断**
   - 移除了多种库类型的优先级判断
   - 移除了重复的 dylib 处理逻辑

### 保留的功能

- ✅ 自动架构检测（Windows: x64/Win32）
- ✅ 自动构建类型检测（Debug/Release）
- ✅ 版本化 dylib 支持
- ✅ 符号链接复制
- ✅ rpath 配置
- ✅ macdeployqt 自动部署

## 配置原理

### Windows 链接原理

1. **编译时**：链接 `TcrSdk.lib`（导入库）
2. **运行时**：需要 `TcrSdk.dll`（动态库）
3. **调试时**：使用 `TcrSdk.pdb`（符号文件）

### macOS 链接原理

1. **编译时**：直接链接 `libTcrSdk.dylib` 或版本化文件
2. **运行时**：通过 rpath 查找 dylib
   - `@executable_path/../Frameworks` = 应用包内的 Frameworks 目录
3. **符号链接**：保持版本化结构，支持多版本共存

## 更多信息

- TcrSdk 构建文档：`TcrSdkProject/scripts/README.md`
- TcrSdk 项目指导：`TcrSdkProject/CLAUDE.md`
