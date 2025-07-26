#pragma once

#include <QObject>
#include <QMap>
#include <QVector>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <memory>
#include "tcr_c_api.h"

/**
 * @brief 云手机批量任务的操作封装类，提供批量任务操作的简化接口
 * 
 * 该类封装了云手机批量操作相关的功能，提供更简洁易用的 API 接口，
 * 负责演示如何使用 TcrAndroidInstance 。支持批量修改分辨率、GPS、传感器、应用管理等多种云手机实例操作。
 * 
 * 典型用法：
 *   - 实例化 BatchTaskOperator
 *   - 调用对应的批量操作方法（如 modifyResolution、paste 等）
 *   - 通过 batchTaskCompleted 信号获取结果
 * 
 * 注意：所有批量操作均为异步，结果通过信号返回。
 */
class BatchTaskOperator : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief 单个实例操作结果项
     */
    struct ResultItem {
        QString instanceId; ///< 实例ID
        QString message;    ///< 结果消息（如错误信息或成功提示）
    };

    /**
     * @brief 批量任务结果结构体
     * 
     * successItems: 业务成功的实例
     * failureItems: 业务失败的实例
     */
    struct BatchResult {
        QVector<ResultItem> successItems; ///< 成功项
        QVector<ResultItem> failureItems; ///< 失败项
    };

    /**
     * @brief 传感器类型枚举
     */
    enum class SensorType {
        Accelerometer, ///< 加速度计
        Gyroscope,     ///< 陀螺仪
        Unknown        ///< 未知类型
    };
    Q_ENUM(SensorType)

    /**
     * @brief 批量任务类型枚举
     * 
     * 用于类型安全地指定批量任务类型
     */
    enum class TaskType {
        ModifyResolution,           ///< 修改分辨率
        ModifyGPS,                  ///< 修改GPS
        Paste,                      ///< 粘贴文本
        SendClipboard,              ///< 发送剪贴板
        ModifySensor,               ///< 修改传感器
        Shake,                      ///< 摇动设备
        Blow,                       ///< 吹气
        SendTransMessage,           ///< 发送透传消息
        ModifyInstanceProperties,   ///< 修改实例属性
        ModifyKeepFrontAppStatus,   ///< 修改保活应用状态
        UnInstallByPackageName,     ///< 卸载应用
        StartApp,                   ///< 启动应用
        StopApp,                    ///< 停止应用
        ClearAppData,               ///< 清除应用数据
        EnableApp,                  ///< 启用应用
        DisableApp,                 ///< 禁用应用
        StartCameraMediaPlay,       ///< 开始摄像头媒体播放
        DisplayCameraImage,         ///< 显示摄像头图片
        AddKeepAliveList,           ///< 添加保活列表
        RemoveKeepAliveList,        ///< 移除保活列表
        SetKeepAliveList,           ///< 设置保活列表
        DescribeInstanceProperties, ///< 获取实例属性
        DescribeKeepFrontAppStatus, ///< 获取保活应用状态
        StopCameraMediaPlay,        ///< 停止摄像头播放
        DescribeCameraMediaPlayStatus, ///< 获取摄像头播放状态
        DescribeKeepAliveList,      ///< 获取保活列表
        ClearKeepAliveList,         ///< 清除保活列表
        ListUserApps,               ///< 列出用户应用
        Mute,                       ///< 静音
        MediaSearch,                ///< 媒体库搜索
        Reboot,                     ///< 重启实例
        ListAllApps,                ///< 查询所有应用
        MoveAppBackground,          ///< 应用退到后台
        AddAppInstallBlackList,     ///< 添加应用安装黑名单
        RemoveAppInstallBlackList,  ///< 移除应用安装黑名单
        SetAppInstallBlackList,     ///< 设置应用安装黑名单
        DescribeAppInstallBlackList,///< 查询应用安装黑名单
        ClearAppInstallBlackList,   ///< 清空应用安装黑名单
        Unknown                     ///< 未知类型
    };
    Q_ENUM(TaskType)

    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit BatchTaskOperator(QObject *parent = nullptr);

    /**
     * @brief 分辨率参数结构体
     */
    struct Resolution {
        int width = 0;  ///< 宽度
        int height = 0; ///< 高度
        int dpi = 0;    ///< DPI（可选）
    };
    /**
     * @brief 批量修改分辨率
     * @param params 实例ID到分辨率参数的映射
     */
    void modifyResolution(const QMap<QString, Resolution> &params);

    /**
     * @brief GPS参数结构体
     */
    struct GPSInfo {
        double longitude = 0.0; ///< 经度
        double latitude = 0.0;  ///< 纬度
    };
    /**
     * @brief 批量修改GPS
     * @param params 实例ID到GPS参数的映射
     */
    void modifyGPS(const QMap<QString, GPSInfo> &params);

    /**
     * @brief 文本参数结构体
     */
    struct TextParam {
        QString text; ///< 文本内容
    };
    /**
     * @brief 批量粘贴文本
     * @param params 实例ID到文本参数的映射
     */
    void paste(const QMap<QString, TextParam> &params);
    /**
     * @brief 批量发送剪贴板内容
     * @param params 实例ID到文本参数的映射
     */
    void sendClipboard(const QMap<QString, TextParam> &params);

    /**
     * @brief 传感器数据结构体
     */
    struct SensorData {
        QString type;           ///< 传感器类型字符串
        QVector<double> values; ///< 数值数组
        int accuracy = 3;       ///< 精度级别
    };
    /**
     * @brief 批量修改传感器数据
     * @param params 实例ID到传感器数据的映射
     */
    void modifySensor(const QMap<QString, SensorData> &params);

    /**
     * @brief 批量摇动设备
     * @param instanceIds 实例ID列表
     */
    void shake(const QVector<QString> &instanceIds);
    /**
     * @brief 批量吹气
     * @param instanceIds 实例ID列表
     */
    void blow(const QVector<QString> &instanceIds);

    /**
     * @brief 透传消息参数结构体
     */
    struct TransMessage {
        QString packageName; ///< 目标包名
        QString msg;         ///< 消息内容
    };
    /**
     * @brief 批量发送透传消息
     * @param params 实例ID到透传消息参数的映射
     */
    void sendTransMessage(const QMap<QString, TransMessage> &params);

    // 实例属性相关结构
    struct DeviceInfo { QJsonObject data; };   ///< 设备信息
    struct ProxyInfo { QJsonObject data; };    ///< 代理信息
    struct SIMInfo { QJsonObject data; };      ///< SIM卡信息
    struct LocaleInfo { QJsonObject data; };   ///< 时区信息
    struct LanguageInfo { QJsonObject data; }; ///< 语言信息
    /**
     * @brief 实例属性参数结构体
     */
    struct InstanceProperties {
        DeviceInfo deviceInfo;         ///< 设备信息
        ProxyInfo proxyInfo;           ///< 代理信息
        GPSInfo gpsInfo;               ///< GPS信息
        SIMInfo simInfo;               ///< SIM卡信息
        LocaleInfo localeInfo;         ///< 时区信息
        LanguageInfo languageInfo;     ///< 语言信息
        QJsonArray extraProperties;    ///< 额外自定义属性
    };
    /**
     * @brief 批量修改实例属性
     * @param params 实例ID到属性参数的映射
     */
    void modifyInstanceProperties(const QMap<QString, InstanceProperties> &params);

    /**
     * @brief 保活应用状态参数结构体
     */
    struct KeepFrontAppStatus {
        QString packageName;           ///< 应用包名
        bool enable;                   ///< 是否启用保活
        int restartIntervalSeconds = 0;///< 重启间隔（秒）
    };
    /**
     * @brief 批量修改保活应用状态
     * @param params 实例ID到保活状态参数的映射
     */
    void modifyKeepFrontAppStatus(const QMap<QString, KeepFrontAppStatus> &params);

    /**
     * @brief 应用包名参数结构体
     */
    struct PackageNameParam {
        QString packageName; ///< 应用包名
    };
    /**
     * @brief 批量卸载应用
     * @param params 实例ID到包名参数的映射
     */
    void unInstallByPackageName(const QMap<QString, PackageNameParam> &params);

    /**
     * @brief 启动应用参数结构体
     */
    struct StartAppParam {
        QString packageName; ///< 应用包名
        QString activityName;///< Activity 名称
    };
    /**
     * @brief 批量启动应用
     * @param params 实例ID到启动参数的映射
     */
    void startApp(const QMap<QString, StartAppParam> &params);

    /**
     * @brief 批量停止应用
     * @param params 实例ID到包名参数的映射
     */
    void stopApp(const QMap<QString, PackageNameParam> &params);
    /**
     * @brief 批量清除应用数据
     * @param params 实例ID到包名参数的映射
     */
    void clearAppData(const QMap<QString, PackageNameParam> &params);
    /**
     * @brief 批量启用应用
     * @param params 实例ID到包名参数的映射
     */
    void enableApp(const QMap<QString, PackageNameParam> &params);
    /**
     * @brief 批量禁用应用
     * @param params 实例ID到包名参数的映射
     */
    void disableApp(const QMap<QString, PackageNameParam> &params);

    /**
     * @brief 摄像头媒体播放参数结构体
     */
    struct CameraMediaPlayParam {
        QString filePath; ///< 媒体文件路径
        int loops = 1;    ///< 循环次数
    };
    /**
     * @brief 批量开始摄像头媒体播放
     * @param params 实例ID到播放参数的映射
     */
    void startCameraMediaPlay(const QMap<QString, CameraMediaPlayParam> &params);

    /**
     * @brief 显示摄像头图片参数结构体
     */
    struct CameraImageParam {
        QString filePath; ///< 图片文件路径
    };
    /**
     * @brief 批量显示摄像头图片
     * @param params 实例ID到图片参数的映射
     */
    void displayCameraImage(const QMap<QString, CameraImageParam> &params);

    /**
     * @brief AppList参数结构体
     */
    struct AppListParam {
        QStringList appList; ///< 应用包名列表
    };
    /**
     * @brief 批量添加保活列表
     * @param params 实例ID到AppList参数的映射
     */
    void addKeepAliveList(const QMap<QString, AppListParam> &params);
    /**
     * @brief 批量移除保活列表
     * @param params 实例ID到AppList参数的映射
     */
    void removeKeepAliveList(const QMap<QString, AppListParam> &params);
    /**
     * @brief 批量设置保活列表
     * @param params 实例ID到AppList参数的映射
     */
    void setKeepAliveList(const QMap<QString, AppListParam> &params);

    /**
     * @brief 批量获取实例属性
     * @param instanceIds 实例ID列表
     */
    void describeInstanceProperties(const QVector<QString> &instanceIds);
    /**
     * @brief 批量获取保活应用状态
     * @param instanceIds 实例ID列表
     */
    void describeKeepFrontAppStatus(const QVector<QString> &instanceIds);
    /**
     * @brief 批量停止摄像头媒体播放
     * @param instanceIds 实例ID列表
     */
    void stopCameraMediaPlay(const QVector<QString> &instanceIds);
    /**
     * @brief 批量获取摄像头播放状态
     * @param instanceIds 实例ID列表
     */
    void describeCameraMediaPlayStatus(const QVector<QString> &instanceIds);
    /**
     * @brief 批量获取保活列表
     * @param instanceIds 实例ID列表
     */
    void describeKeepAliveList(const QVector<QString> &instanceIds);
    /**
     * @brief 批量清除保活列表
     * @param instanceIds 实例ID列表
     */
    void clearKeepAliveList(const QVector<QString> &instanceIds);
    /**
     * @brief 批量列出用户应用
     * @param instanceIds 实例ID列表
     */
    void listUserApps(const QVector<QString> &instanceIds);

    /**
     * @brief 静音参数结构体
     */
    struct MuteParam {
        bool mute; ///< 是否静音
    };
    /**
     * @brief 批量设置静音
     * @param params 实例ID到静音参数的映射
     */
    void mute(const QMap<QString, MuteParam> &params);

    /**
     * @brief 媒体库搜索参数结构体
     */
    struct MediaSearchParam {
        QString keyword; ///< 搜索关键字
    };
    /**
     * @brief 批量媒体库搜索
     * @param params 实例ID到搜索参数的映射
     */
    void mediaSearch(const QMap<QString, MediaSearchParam> &params);

    /**
     * @brief 批量重启实例
     * @param instanceIds 实例ID列表
     */
    void reboot(const QVector<QString> &instanceIds);
    /**
     * @brief 批量查询所有应用
     * @param instanceIds 实例ID列表
     */
    void listAllApps(const QVector<QString> &instanceIds);
    /**
     * @brief 批量将应用退到后台
     * @param instanceIds 实例ID列表
     */
    void moveAppBackground(const QVector<QString> &instanceIds);

    /**
     * @brief 批量添加应用安装黑名单
     * @param params 实例ID到AppList参数的映射
     */
    void addAppInstallBlackList(const QMap<QString, AppListParam> &params);
    /**
     * @brief 批量移除应用安装黑名单
     * @param params 实例ID到AppList参数的映射
     */
    void removeAppInstallBlackList(const QMap<QString, AppListParam> &params);
    /**
     * @brief 批量设置应用安装黑名单
     * @param params 实例ID到AppList参数的映射
     */
    void setAppInstallBlackList(const QMap<QString, AppListParam> &params);
    /**
     * @brief 批量查询应用安装黑名单
     * @param instanceIds 实例ID列表
     */
    void describeAppInstallBlackList(const QVector<QString> &instanceIds);
    /**
     * @brief 批量清空应用安装黑名单
     * @param instanceIds 实例ID列表
     */
    void clearAppInstallBlackList(const QVector<QString> &instanceIds);

    /**
     * @brief 获取指定实例的小流截图URL
     * @param instanceId 实例ID
     * @param quality 图片质量（1-100，默认20）
     * @return 截图URL字符串，失败返回空字符串
     */
    QString getInstanceImage(const QString &instanceId, int quality = 20);

signals:
    /**
     * @brief 批量任务完成信号
     * @param result 任务结果
     */
    void batchTaskCompleted(const BatchResult &result);
    
    /**
     * @brief 批量任务失败信号
     * @param errorCode 错误码
     * @param errorMessage 错误信息
     */
    void batchTaskFailed(int errorCode, const QString &errorMessage);

private:
    /**
     * @brief 解析批量任务结果
     * @param jsonData JSON数据
     * @return 解析后的BatchResult
     */
    BatchResult parseBatchResult(const QByteArray &jsonData);

    /**
     * @brief 执行批量任务
     * @param taskType 任务类型字符串
     * @param params 参数对象
     */
    void executeBatchTask(const QString &taskType, const QJsonObject &params);
};