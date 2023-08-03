# 集成 SDK（Windows）

本文介绍如何快速地将腾讯云的 TcrSdk 集成到项目中，只要按照如下步骤进行操作，可以轻松完成 TcrSdk 的集成工作。
## 开发环境要求

- 操作系统：推荐 Windows 10。
- 开发环境：推荐使用 Visual Studio 2019。

## 集成 TcrSdk

下面通过创建一个简单的 MFC 项目，介绍如何在 Visual Studio 2019 工程中集成 SDK。
### 步骤1. 下载 TcrSdk


|目录名 |说明 |
|--|--|
|includes |接口头文件 |
|libs\x86| **32位**，采用 /MT 选项链接库文件 |
|libs\x64| **64位**，采用 /MT 选项链接库文件 |
### 步骤2. 新建工程

打开 Visual Studio，新建一个名为 TcrDemo 的 Windows桌面应用程序
### 步骤3. 拷贝文件

将解压后的 tcrsdk 文件夹拷贝到 TcrDemo.vcxproj 所在目录下，如下图所示：
![enter image description here](/tencent/api/attachments/s3/url?attachmentid=3497143)
### 步骤4. 修改工程配置

TcrSdk 提供了 **x64** 和 **x86** 两种编译生成的静态库，针对这两种有些地方要专门配置。打开 TcrDemo 属性页，在**解决方案资源管理器**>**TcrDemo工程的右键菜单**>**属性**。
以**x64**为例，请按照以下步骤进行配置：
1. 添加包含目录
    在 **C/C++**>**常规**>**附件包含目录**，添加 tcrsdk 头文件目录 $(ProjectDir)tcrsdk\include，如下图所示：
<br><br>
<img src="docs/images/接入SDK添加包含目录.png" width="700px">
<br><br>
2. 添加库目录
    在 **链接器**>**常规**>**附加库目录**，添加 tcrsdk 库目录 $(ProjectDir)tcrsdk\libs\x64
<br><br>
<img src="docs/images/接入SDK添加库目录.png" width="700px">
<br><br>
3. 添加库文件
    在 **链接器**>**输入**>**附加依赖项**，添加 tcrsdk 库文件 TcrSdk-Win.lib ，如下图所示：
<br><br>
<img src="docs/images/接入SDK添加库文件.png" width="700px">
<br><br>
4. 拷贝 DLL 到执行目录
    在**生成事件**>**生成前事件**>**命令行**，输入  `xcopy /E /Y "$(ProjectDir)tcrsdk\libs\x64" "$(OutDir)"` ，拷贝 TcrSdk-Win.dll 动态库文件到程序生成目录，如下图所示：
<br><br>
<img src="docs/images/接入SDK生成前事件.png" width="700px">
<br><br>
5. 更改代码生成方式
在**代码生成**>**运行库**，改为“多线程/MT”
<br><br>
<img src="docs/images/接入SDK更改代码生成.png" width="700px">
<br><br>
6. 运行，创建TcrSession

* 添加头文件
```
#include "tcr_session.h"
```
* 实现TcrSession::Observer
```
class TcrObserver : public tcrsdk::TcrSession::Observer {
    void onEvent(tcrsdk::TcrSession::Event event, const char* eventData)
    {
        switch (event)
        {
        case tcrsdk::TcrSession::STATE_INITED:
            //do something
            break;
        case tcrsdk::TcrSession::STATE_CONNECTED:
            //do something
            break;
        case tcrsdk::TcrSession::STATE_CLOSED:
            //do something
            break;
        case tcrsdk::TcrSession::CLIENT_STATS:
            //do something
            break;
        case tcrsdk::TcrSession::GAME_START_COMPLETE:
            //do something
            break;
        case tcrsdk::TcrSession::ARCHIVE_LOAD_STATUS:
            //do something
            break;
        case tcrsdk::TcrSession::ARCHIVE_SAVE_STATUS:
            //do something
            break;
        case tcrsdk::TcrSession::INPUT_STATUS_CHANGED:
            //do something
            break;
        case tcrsdk::TcrSession::REMOTE_DESKTOP_INFO:
            //do something
            break;
        case tcrsdk::TcrSession::SCREEN_CONFIG_CHANGE:
            //do something
            break;
        case tcrsdk::TcrSession::CURSOR_IMAGE_INFO: 
            //do something
            break;
        case tcrsdk::TcrSession::CURSOR_STATE_CHANGE:
            //do something
            break;
        default:
            break;
        }
    }
};
```
* 创建TcrSession
```
    tcrsdk::TcrSession* tcrSession = new tcrsdk::TcrSession(new TcrObserver());
```