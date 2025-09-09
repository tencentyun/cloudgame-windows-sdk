#include "BatchTaskOperatorModel.h"
#include "utils/Logger.h"

BatchTaskOperatorModel::BatchTaskOperatorModel(BatchTaskOperator* batchTaskOperator, QObject *parent)
    : QObject(parent), m_batchTaskOperator(batchTaskOperator)
{
    // 连接批量任务完成和失败信号
    connect(m_batchTaskOperator, &BatchTaskOperator::batchTaskCompleted, this, &BatchTaskOperatorModel::onBatchTaskCompleted);
    connect(m_batchTaskOperator, &BatchTaskOperator::batchTaskFailed, this, &BatchTaskOperatorModel::onBatchTaskFailed);
}

// 实现处理对话框信号的方法
void BatchTaskOperatorModel::handleDialogSignal(const QString& dialogType, const QStringList& instanceIds, const QVariantMap& params) {

    Logger::debug(QString("[BatchTaskOperatorModel::handleDialogSignal] dialogType: %1, instanceIds: %2, params: %3")
        .arg(dialogType)
        .arg(instanceIds.join(","))
        .arg(QJsonDocument::fromVariant(params).toJson(QJsonDocument::Compact)));
    
    // 检查实例ID是否为空
    if (instanceIds.isEmpty()) {
        emit showDialog(tr("提示"), tr("请先选择实例"));
        return;
    }
    
    // 根据对话框类型调用不同的批量任务方法
    if (dialogType == "gps") {
        BatchTaskOperator::GPSInfo gpsInfo;
        gpsInfo.longitude = params["longitude"].toDouble();
        gpsInfo.latitude = params["latitude"].toDouble();
        
        QMap<QString, BatchTaskOperator::GPSInfo> gpsParams;
        for (const QString& id : instanceIds) {
            gpsParams[id] = gpsInfo;
        }
        m_batchTaskOperator->modifyGPS(gpsParams);
    }
    else if (dialogType == "resolution") {
        BatchTaskOperator::Resolution resolution;
        resolution.width = params["width"].toInt();
        resolution.height = params["height"].toInt();
        resolution.dpi = params["dpi"].toInt();
        
        QMap<QString, BatchTaskOperator::Resolution> resolutionParams;
        for (const QString& id : instanceIds) {
            resolutionParams[id] = resolution;
        }
        m_batchTaskOperator->modifyResolution(resolutionParams);
    }
    else if (dialogType == "paste") {
        BatchTaskOperator::TextParam textParam;
        textParam.text = params["text"].toString();
        
        QMap<QString, BatchTaskOperator::TextParam> pasteParams;
        for (const QString& id : instanceIds) {
            pasteParams[id] = textParam;
        }
        m_batchTaskOperator->paste(pasteParams);
    }
    else if (dialogType == "sendClipboard") {
        BatchTaskOperator::TextParam textParam;
        textParam.text = params["text"].toString();
        
        QMap<QString, BatchTaskOperator::TextParam> clipboardParams;
        for (const QString& id : instanceIds) {
            clipboardParams[id] = textParam;
        }
        m_batchTaskOperator->sendClipboard(clipboardParams);
    }
    else if (dialogType == "modifySensor") {
        BatchTaskOperator::SensorData sensorData;
        sensorData.type = params["sensorType"].toString();
        QVariantList valuesList = params["values"].toList();
        QVector<double> values;
        for (const QVariant& v : valuesList) {
            values.append(v.toDouble());
        }
        sensorData.values = values;
        sensorData.accuracy = params["accuracy"].toInt();
        
        QMap<QString, BatchTaskOperator::SensorData> sensorParams;
        for (const QString& id : instanceIds) {
            sensorParams[id] = sensorData;
        }
        m_batchTaskOperator->modifySensor(sensorParams);
    }
    else if (dialogType == "shake") {
        m_batchTaskOperator->shake(instanceIds.toVector());
    }
    else if (dialogType == "blow") {
        m_batchTaskOperator->blow(instanceIds.toVector());
    }
    else if (dialogType == "message") {
        BatchTaskOperator::TransMessage transMessage;
        transMessage.packageName = params["packageName"].toString();
        transMessage.msg = params["msg"].toString();
        
        QMap<QString, BatchTaskOperator::TransMessage> messageParams;
        for (const QString& id : instanceIds) {
            messageParams[id] = transMessage;
        }
        m_batchTaskOperator->sendTransMessage(messageParams);
    }
    else if (dialogType == "modifyInstanceProperties") {
        // 解析JSON字符串
        QString jsonStr = params["json"].toString();
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
        if (doc.isNull()) {
            emit showDialog(tr("错误"), tr("修改实例属性参数解析失败"));
            return;
        }
        QJsonObject jsonObj = doc.object();
        
        // 构造实例属性参数
        BatchTaskOperator::InstanceProperties properties;
        // 解析DeviceInfo
        if (jsonObj.contains("DeviceInfo")) {
            QJsonObject deviceInfoObj = jsonObj["DeviceInfo"].toObject();
            properties.deviceInfo.data = deviceInfoObj;
        }
        // 解析ProxyInfo
        if (jsonObj.contains("ProxyInfo")) {
            QJsonObject proxyInfoObj = jsonObj["ProxyInfo"].toObject();
            properties.proxyInfo.data = proxyInfoObj;
        }
        // 解析GPSInfo
        if (jsonObj.contains("GPSInfo")) {
            QJsonObject gpsInfoObj = jsonObj["GPSInfo"].toObject();
            properties.gpsInfo.longitude = gpsInfoObj["Longitude"].toDouble();
            properties.gpsInfo.latitude = gpsInfoObj["Latitude"].toDouble();
        }
        // 解析SIMInfo
        if (jsonObj.contains("SIMInfo")) {
            QJsonObject simInfoObj = jsonObj["SIMInfo"].toObject();
            properties.simInfo.data = simInfoObj;
        }
        // 解析LocaleInfo
        if (jsonObj.contains("LocaleInfo")) {
            QJsonObject localeInfoObj = jsonObj["LocaleInfo"].toObject();
            properties.localeInfo.data = localeInfoObj;
        }
        // 解析LanguageInfo
        if (jsonObj.contains("LanguageInfo")) {
            QJsonObject languageInfoObj = jsonObj["LanguageInfo"].toObject();
            properties.languageInfo.data = languageInfoObj;
        }
        // 解析ExtraProperties
        if (jsonObj.contains("ExtraProperties")) {
            properties.extraProperties = jsonObj["ExtraProperties"].toArray();
        }
        
        QMap<QString, BatchTaskOperator::InstanceProperties> propertiesParams;
        for (const QString& id : instanceIds) {
            propertiesParams[id] = properties;
        }
        m_batchTaskOperator->modifyInstanceProperties(propertiesParams);
    }
    else if (dialogType == "modifyKeepFrontAppStatus") {
        BatchTaskOperator::KeepFrontAppStatus status;
        status.packageName = params["packageName"].toString();
        status.enable = params["enable"].toBool();
        status.restartIntervalSeconds = params["restartInterval"].toInt();
        
        QMap<QString, BatchTaskOperator::KeepFrontAppStatus> statusParams;
        for (const QString& id : instanceIds) {
            statusParams[id] = status;
        }
        m_batchTaskOperator->modifyKeepFrontAppStatus(statusParams);
    }
    else if (dialogType == "unInstallByPackageName") {
        BatchTaskOperator::PackageNameParam packageParam;
        packageParam.packageName = params["packageName"].toString();
        
        QMap<QString, BatchTaskOperator::PackageNameParam> packageParams;
        for (const QString& id : instanceIds) {
            packageParams[id] = packageParam;
        }
        m_batchTaskOperator->unInstallByPackageName(packageParams);
    }
    else if (dialogType == "startApp") {
        BatchTaskOperator::StartAppParam startAppParam;
        startAppParam.packageName = params["packageName"].toString();
        startAppParam.activityName = params["activityName"].toString();
        
        QMap<QString, BatchTaskOperator::StartAppParam> startAppParams;
        for (const QString& id : instanceIds) {
            startAppParams[id] = startAppParam;
        }
        m_batchTaskOperator->startApp(startAppParams);
    }
    else if (dialogType == "stopApp") {
        BatchTaskOperator::PackageNameParam packageParam;
        packageParam.packageName = params["packageName"].toString();
        
        QMap<QString, BatchTaskOperator::PackageNameParam> packageParams;
        for (const QString& id : instanceIds) {
            packageParams[id] = packageParam;
        }
        m_batchTaskOperator->stopApp(packageParams);
    }
    else if (dialogType == "clearAppData") {
        BatchTaskOperator::PackageNameParam packageParam;
        packageParam.packageName = params["packageName"].toString();
        
        QMap<QString, BatchTaskOperator::PackageNameParam> packageParams;
        for (const QString& id : instanceIds) {
            packageParams[id] = packageParam;
        }
        m_batchTaskOperator->clearAppData(packageParams);
    }
    else if (dialogType == "enableApp") {
        BatchTaskOperator::PackageNameParam packageParam;
        packageParam.packageName = params["packageName"].toString();
        
        QMap<QString, BatchTaskOperator::PackageNameParam> packageParams;
        for (const QString& id : instanceIds) {
            packageParams[id] = packageParam;
        }
        m_batchTaskOperator->enableApp(packageParams);
    }
    else if (dialogType == "disableApp") {
        BatchTaskOperator::PackageNameParam packageParam;
        packageParam.packageName = params["packageName"].toString();
        
        QMap<QString, BatchTaskOperator::PackageNameParam> packageParams;
        for (const QString& id : instanceIds) {
            packageParams[id] = packageParam;
        }
        m_batchTaskOperator->disableApp(packageParams);
    }
    else if (dialogType == "startCameraMediaPlay") {
        BatchTaskOperator::CameraMediaPlayParam mediaParam;
        mediaParam.filePath = params["filePath"].toString();
        mediaParam.loops = params["loops"].toInt();
        
        QMap<QString, BatchTaskOperator::CameraMediaPlayParam> mediaParams;
        for (const QString& id : instanceIds) {
            mediaParams[id] = mediaParam;
        }
        m_batchTaskOperator->startCameraMediaPlay(mediaParams);
    }
    else if (dialogType == "displayCameraImage") {
        BatchTaskOperator::CameraImageParam imageParam;
        imageParam.filePath = params["filePath"].toString();
        
        QMap<QString, BatchTaskOperator::CameraImageParam> imageParams;
        for (const QString& id : instanceIds) {
            imageParams[id] = imageParam;
        }
        m_batchTaskOperator->displayCameraImage(imageParams);
    }
    else if (dialogType == "addKeepAliveList") {
        BatchTaskOperator::AppListParam appListParam;
        appListParam.appList = params["appList"].toString().split(',');
        
        QMap<QString, BatchTaskOperator::AppListParam> appListParams;
        for (const QString& id : instanceIds) {
            appListParams[id] = appListParam;
        }
        m_batchTaskOperator->addKeepAliveList(appListParams);
    }
    else if (dialogType == "removeKeepAliveList") {
        BatchTaskOperator::AppListParam appListParam;
        appListParam.appList = params["appList"].toString().split(',');
        
        QMap<QString, BatchTaskOperator::AppListParam> appListParams;
        for (const QString& id : instanceIds) {
            appListParams[id] = appListParam;
        }
        m_batchTaskOperator->removeKeepAliveList(appListParams);
    } else if (dialogType == "setKeepAliveList") {
        BatchTaskOperator::AppListParam appListParam;
        appListParam.appList = params["appList"].toString().split(',');
        
        QMap<QString, BatchTaskOperator::AppListParam> appListParams;
        for (const QString& id : instanceIds) {
            appListParams[id] = appListParam;
        }
        m_batchTaskOperator->setKeepAliveList(appListParams);
    } else if (dialogType == "describeInstanceProperties") {
        m_batchTaskOperator->describeInstanceProperties(instanceIds.toVector());
    }
    else if (dialogType == "describeKeepFrontAppStatus") {
        m_batchTaskOperator->describeKeepFrontAppStatus(instanceIds.toVector());
    }
    else if (dialogType == "stopCameraMediaPlay") {
        m_batchTaskOperator->stopCameraMediaPlay(instanceIds.toVector());
    }
    else if (dialogType == "describeCameraMediaPlayStatus") {
        m_batchTaskOperator->describeCameraMediaPlayStatus(instanceIds.toVector());
    }
    else if (dialogType == "describeKeepAliveList") {
        m_batchTaskOperator->describeKeepAliveList(instanceIds.toVector());
    }
    else if (dialogType == "clearKeepAliveList") {
        m_batchTaskOperator->clearKeepAliveList(instanceIds.toVector());
    }
    else if (dialogType == "listUserApps") {
        m_batchTaskOperator->listUserApps(instanceIds.toVector());
    }
    else if (dialogType == "mute") {
        bool mute = params["mute"].toBool();
        QMap<QString, BatchTaskOperator::MuteParam> muteParams;
        for (const QString& id : instanceIds) {
            BatchTaskOperator::MuteParam param;
            param.mute = mute;
            muteParams[id] = param;
        }
        m_batchTaskOperator->mute(muteParams);
    }
    else if (dialogType == "mediaSearch") {
        QString keyword = params["keyword"].toString();
        QMap<QString, BatchTaskOperator::MediaSearchParam> mediaSearchParams;
        for (const QString& id : instanceIds) {
            BatchTaskOperator::MediaSearchParam param;
            param.keyword = keyword;
            mediaSearchParams[id] = param;
        }
        m_batchTaskOperator->mediaSearch(mediaSearchParams);
    }
    else if (dialogType == "reboot") {
        m_batchTaskOperator->reboot(instanceIds.toVector());
    }
    else if (dialogType == "listAllApps") {
        m_batchTaskOperator->listAllApps(instanceIds.toVector());
    }
    else if (dialogType == "moveAppBackground") {
        m_batchTaskOperator->moveAppBackground(instanceIds.toVector());
    }
    else if (dialogType == "addAppInstallBlackList") {
        QStringList appList = params["appList"].toString().split(',');
        BatchTaskOperator::AppListParam appListParam;
        appListParam.appList = appList;
        QMap<QString, BatchTaskOperator::AppListParam> appListParams;
        for (const QString& id : instanceIds) {
            appListParams[id] = appListParam;
        }
        m_batchTaskOperator->addAppInstallBlackList(appListParams);
    }
    else if (dialogType == "removeAppInstallBlackList") {
        QStringList appList = params["appList"].toString().split(',');
        BatchTaskOperator::AppListParam appListParam;
        appListParam.appList = appList;
        QMap<QString, BatchTaskOperator::AppListParam> appListParams;
        for (const QString& id : instanceIds) {
            appListParams[id] = appListParam;
        }
        m_batchTaskOperator->removeAppInstallBlackList(appListParams);
    }
    else if (dialogType == "setAppInstallBlackList") {
        QStringList appList = params["appList"].toString().split(',');
        BatchTaskOperator::AppListParam appListParam;
        appListParam.appList = appList;
        QMap<QString, BatchTaskOperator::AppListParam> appListParams;
        for (const QString& id : instanceIds) {
            appListParams[id] = appListParam;
        }
        m_batchTaskOperator->setAppInstallBlackList(appListParams);
    }
    else if (dialogType == "describeAppInstallBlackList") {
        m_batchTaskOperator->describeAppInstallBlackList(instanceIds.toVector());
    }
    else if (dialogType == "clearAppInstallBlackList") {
        m_batchTaskOperator->clearAppInstallBlackList(instanceIds.toVector());
    }
    else {
        // 如果dialogType没有被处理，弹出提示
        emit showDialog(tr("提示"), tr("当前没有处理该操作类型: %1").arg(dialogType));
    }
}

// 处理批量任务完成信号
void BatchTaskOperatorModel::onBatchTaskCompleted(const BatchTaskOperator::BatchResult &result) {
    QString message;
    
    if (!result.successItems.isEmpty()) {
        message += tr("以下实例任务执行成功:\n");
        for (const auto &item : result.successItems) {
            message += QString("%1: %2\n").arg(item.instanceId).arg(item.message);
        }
    }
    
    if (!result.failureItems.isEmpty()) {
        message += tr("\n以下实例任务执行失败:\n");
        for (const auto &item : result.failureItems) {
            message += QString("%1: %2\n").arg(item.instanceId).arg(item.message);
        }
    }
    
    if (message.isEmpty()) {
        message = tr("批量任务完成，但没有返回任何结果。");
    }
    
    emit showDialog(tr("批量任务结果"), message);
}

// 处理批量任务失败信号
void BatchTaskOperatorModel::onBatchTaskFailed(int errorCode, const QString &errorMessage) {
    QString message = tr("批量任务失败，错误码：%1，错误信息：%2").arg(errorCode).arg(errorMessage);
    emit showDialog(tr("批量任务错误"), message);
}
