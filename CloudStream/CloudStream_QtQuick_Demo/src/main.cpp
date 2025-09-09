#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QResource>
#include "services/ApiService.h"
#include "services/NetworkService.h"
#include "utils/Logger.h" 
#include "core/video/VideoRenderItem.h"
#include "core/TcrInstanceManager.h"
#include "viewmodels/StreamingViewModel.h"
#include "viewmodels/InstanceTokenViewModel.h"


/**
 * @brief 应用程序入口函数
 * 
 * 主要职责：
 * - 初始化日志系统
 * - 创建服务层和视图模型对象
 * - 注册QML上下文属性和自定义类型
 * - 加载QML主界面
 * - 处理QML对象创建失败的情况
 */
int main(int argc, char *argv[]) {
    // 创建Qt GUI应用程序对象
    QGuiApplication app(argc, argv);

    // -------------------- 日志系统初始化 --------------------
    /// 初始化全局日志系统，确保日志功能在应用生命周期内可用
    Logger::globalInit();

    // -------------------- 服务层与模型层对象创建 --------------------
    /// 创建网络服务对象，负责HTTP请求
    NetworkService* networkService = new NetworkService(&app);

    /// 创建API服务对象，封装所有与云手机API的交互
    ApiService* apiService = new ApiService(networkService, &app);

    /// 创建云手机实例模型，管理实例列表及图片下载
    TcrInstanceManager* instanceManager = new TcrInstanceManager(apiService, &app);

    InstanceTokenViewModel* instanceAccessViewModel = new InstanceTokenViewModel(apiService, &app);
    // -------------------- QML引擎与上下文注册 --------------------
    QQmlApplicationEngine engine;

    // -------------------- QML自定义类型与图像提供者注册 --------------------

    /// 注册自定义图像提供者，用于实例的图片显示
    engine.addImageProvider(QLatin1String("instance"), TcrInstanceManager::imageProvider());

    /// 注册自定义视频渲染组件，供QML使用
    qmlRegisterType<VideoRenderItem>("CustomComponents", 1, 0, "VideoRenderItem");

    /// 注册流媒体视图模型，供QML使用
    qmlRegisterType<StreamingViewModel>("CustomComponents", 1, 0, "StreamingViewModel");

    engine.rootContext()->setContextProperty("instanceAccessViewModel", instanceAccessViewModel);
    engine.rootContext()->setContextProperty("instanceManager", instanceManager);
    engine.rootContext()->setContextProperty("networkService", networkService);
    engine.rootContext()->setContextProperty("apiService", apiService);

    // -------------------- QML对象创建失败处理 --------------------
    /**
     * 连接QML引擎的objectCreationFailed信号，
     * 若QML主界面加载失败，则退出应用并返回-1
     */
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    // -------------------- 加载QML主界面 --------------------
    engine.loadFromModule("CloudStream_QtQuick_Demo", "InstanceTokenWindow");

    // -------------------- 进入主事件循环 --------------------
    return app.exec();
}
