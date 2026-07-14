# CloudPhone_DuiLib_Demo

基于 DuiLib 的云手机 Demo 客户端。

## 环境要求
- Windows 10+
- Visual Studio 2022
- CMake 3.21+

## 构建前置准备

执行构建脚本前，需要先准备好以下依赖：

### 1. 克隆 vcpkg

在项目根目录执行：
```powershell
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat -disableMetrics
cd ..
```

### 2. 下载 TcrSdk

将 TcrSdk 二进制文件（`.dll`、`.lib`、`.pdb`）按以下目录结构放置到 `third_party/TcrSdk/` 中：

```
third_party/TcrSdk/
├── include/
│   ├── tcr_c_api.h
│   ├── tcr_export.h
│   └── tcr_types.h
└── win/
    ├── Debug/
    │   ├── Win32/
    │   │   ├── TcrSdk.dll
    │   │   ├── TcrSdk.lib
    │   │   └── TcrSdk.pdb
    │   └── x64/
    │       ├── TcrSdk.dll
    │       ├── TcrSdk.lib
    │       └── TcrSdk.pdb
    └── Release/
        ├── Win32/
        │   ├── TcrSdk.dll
        │   ├── TcrSdk.lib
        │   └── TcrSdk.pdb
        └── x64/
            ├── TcrSdk.dll
            ├── TcrSdk.lib
            └── TcrSdk.pdb
```

> **注意：** 仓库不包含 vcpkg 和 TcrSdk 二进制文件，请自行获取。

## 构建
```powershell
.\build.ps1                    # Debug 构建
.\build.ps1 -Config release    # Release 构建
.\build.ps1 -Clean             # 清理后重新构建
.\build.ps1 -Open              # 构建后打开 VS 解决方案
```

## 运行依赖
- TcrSdk DLL (自动拷贝到输出目录)
- skin/ 目录 (自动拷贝到输出目录)

## 功能
- 登录窗口
- 云手机实例列表 (带截图)
- 视频串流 (I420 YUV + libyuv + GDI)
- 触摸/键盘/鼠标输入
- 38种批量操作
- 群控模式
- 摄像头/麦克风控制
- 视频码率/帧率动态调整
- 实时统计信息显示
