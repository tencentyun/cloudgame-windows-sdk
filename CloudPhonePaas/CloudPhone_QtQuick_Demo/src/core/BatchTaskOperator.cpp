#include "BatchTaskOperator.h"
#include "utils/Logger.h"
#include <QJsonParseError>
#include <vector>
#include "tcr_c_api.h"

BatchTaskOperator::BatchTaskOperator(QObject *parent) : QObject(parent)
{
}

void BatchTaskOperator::modifyResolution(const QMap<QString, Resolution> &params)
{
    QJsonObject jsonParams;
    for (auto it = params.begin(); it != params.end(); ++it) {
        QJsonObject resObj;
        resObj["Width"] = it.value().width;
        resObj["Height"] = it.value().height;
        resObj["DPI"] = it.value().dpi;
        jsonParams[it.key()] = resObj;
    }
    executeBatchTask("ModifyResolution", jsonParams);
}

void BatchTaskOperator::modifyGPS(const QMap<QString, GPSInfo> &params)
{
    QJsonObject jsonParams;
    for (auto it = params.begin(); it != params.end(); ++it) {
        QJsonObject gpsObj;
        gpsObj["Longitude"] = it.value().longitude;
        gpsObj["Latitude"] = it.value().latitude;
        jsonParams[it.key()] = gpsObj;
    }
    executeBatchTask("ModifyGPS", jsonParams);
}

void BatchTaskOperator::paste(const QMap<QString, TextParam> &params)
{
    QJsonObject jsonParams;
    for (auto it = params.begin(); it != params.end(); ++it) {
        QJsonObject textObj;
        textObj["Text"] = it.value().text;
        jsonParams[it.key()] = textObj;
    }
    executeBatchTask("Paste", jsonParams);
}

void BatchTaskOperator::sendClipboard(const QMap<QString, TextParam> &params)
{
    QJsonObject jsonParams;
    for (auto it = params.begin(); it != params.end(); ++it) {
        QJsonObject textObj;
        textObj["Text"] = it.value().text;
        jsonParams[it.key()] = textObj;
    }
    executeBatchTask("SendClipboard", jsonParams);
}

void BatchTaskOperator::modifySensor(const QMap<QString, SensorData> &params)
{
    QJsonObject jsonParams;
    for (auto it = params.begin(); it != params.end(); ++it) {
        QJsonObject sensorObj;
        sensorObj["Type"] = it.value().type;
        
        QJsonArray valuesArray;
        for (double val : it.value().values) {
            valuesArray.append(val);
        }
        sensorObj["Values"] = valuesArray;
        sensorObj["Accuracy"] = it.value().accuracy;
        
        jsonParams[it.key()] = sensorObj;
    }
    executeBatchTask("ModifySensor", jsonParams);
}

void BatchTaskOperator::shake(const QVector<QString> &instanceIds)
{
    QJsonObject jsonParams;
    for (const QString &id : instanceIds) {
        jsonParams[id] = QJsonObject(); // 空对象表示无参数
    }
    executeBatchTask("Shake", jsonParams);
}

void BatchTaskOperator::blow(const QVector<QString> &instanceIds)
{
    QJsonObject jsonParams;
    for (const QString &id : instanceIds) {
        jsonParams[id] = QJsonObject(); // 空对象表示无参数
    }
    executeBatchTask("Blow", jsonParams);
}

void BatchTaskOperator::sendTransMessage(const QMap<QString, TransMessage> &params)
{
    QJsonObject jsonParams;
    for (auto it = params.begin(); it != params.end(); ++it) {
        QJsonObject msgObj;
        msgObj["PackageName"] = it.value().packageName;
        msgObj["Msg"] = it.value().msg;
        jsonParams[it.key()] = msgObj;
    }
    executeBatchTask("SendTransMessage", jsonParams);
}

void BatchTaskOperator::modifyInstanceProperties(const QMap<QString, InstanceProperties> &params)
{
    QJsonObject jsonParams;
    for (auto it = params.begin(); it != params.end(); ++it) {
        QJsonObject propObj;
        propObj["DeviceInfo"] = it.value().deviceInfo.data;
        propObj["ProxyInfo"] = it.value().proxyInfo.data;
        propObj["GPSInfo"] = QJsonObject::fromVariantMap({
            {"Longitude", it.value().gpsInfo.longitude},
            {"Latitude", it.value().gpsInfo.latitude}
        });
        propObj["SIMInfo"] = it.value().simInfo.data;
        propObj["LocaleInfo"] = it.value().localeInfo.data;
        propObj["LanguageInfo"] = it.value().languageInfo.data;
        propObj["ExtraProperties"] = it.value().extraProperties;
        
        jsonParams[it.key()] = propObj;
    }
    executeBatchTask("ModifyInstanceProperties", jsonParams);
}

void BatchTaskOperator::modifyKeepFrontAppStatus(const QMap<QString, KeepFrontAppStatus> &params)
{
    QJsonObject jsonParams;
    for (auto it = params.begin(); it != params.end(); ++it) {
        QJsonObject statusObj;
        statusObj["PackageName"] = it.value().packageName;
        statusObj["Enable"] = it.value().enable;
        statusObj["RestartInterValSeconds"] = it.value().restartIntervalSeconds;
        jsonParams[it.key()] = statusObj;
    }
    executeBatchTask("ModifyKeepFrontAppStatus", jsonParams);
}

void BatchTaskOperator::unInstallByPackageName(const QMap<QString, PackageNameParam> &params)
{
    QJsonObject jsonParams;
    for (auto it = params.begin(); it != params.end(); ++it) {
        QJsonObject pkgObj;
        pkgObj["PackageName"] = it.value().packageName;
        jsonParams[it.key()] = pkgObj;
    }
    executeBatchTask("UnInstallByPackageName", jsonParams);
}

void BatchTaskOperator::startApp(const QMap<QString, StartAppParam> &params)
{
    QJsonObject jsonParams;
    for (auto it = params.begin(); it != params.end(); ++it) {
        QJsonObject appObj;
        appObj["PackageName"] = it.value().packageName;
        appObj["ActivityName"] = it.value().activityName;
        jsonParams[it.key()] = appObj;
    }
    executeBatchTask("StartApp", jsonParams);
}

void BatchTaskOperator::stopApp(const QMap<QString, PackageNameParam> &params)
{
    QJsonObject jsonParams;
    for (auto it = params.begin(); it != params.end(); ++it) {
        QJsonObject pkgObj;
        pkgObj["PackageName"] = it.value().packageName;
        jsonParams[it.key()] = pkgObj;
    }
    executeBatchTask("StopApp", jsonParams);
}

void BatchTaskOperator::clearAppData(const QMap<QString, PackageNameParam> &params)
{
    QJsonObject jsonParams;
    for (auto it = params.begin(); it != params.end(); ++it) {
        QJsonObject pkgObj;
        pkgObj["PackageName"] = it.value().packageName;
        jsonParams[it.key()] = pkgObj;
    }
    executeBatchTask("ClearAppData", jsonParams);
}

void BatchTaskOperator::enableApp(const QMap<QString, PackageNameParam> &params)
{
    QJsonObject jsonParams;
    for (auto it = params.begin(); it != params.end(); ++it) {
        QJsonObject pkgObj;
        pkgObj["PackageName"] = it.value().packageName;
        jsonParams[it.key()] = pkgObj;
    }
    executeBatchTask("EnableApp", jsonParams);
}

void BatchTaskOperator::disableApp(const QMap<QString, PackageNameParam> &params)
{
    QJsonObject jsonParams;
    for (auto it = params.begin(); it != params.end(); ++it) {
        QJsonObject pkgObj;
        pkgObj["PackageName"] = it.value().packageName;
        jsonParams[it.key()] = pkgObj;
    }
    executeBatchTask("DisableApp", jsonParams);
}

void BatchTaskOperator::startCameraMediaPlay(const QMap<QString, CameraMediaPlayParam> &params)
{
    QJsonObject jsonParams;
    for (auto it = params.begin(); it != params.end(); ++it) {
        QJsonObject mediaObj;
        mediaObj["FilePath"] = it.value().filePath;
        mediaObj["Loops"] = it.value().loops;
        jsonParams[it.key()] = mediaObj;
    }
    executeBatchTask("StartCameraMediaPlay", jsonParams);
}

void BatchTaskOperator::displayCameraImage(const QMap<QString, CameraImageParam> &params)
{
    QJsonObject jsonParams;
    for (auto it = params.begin(); it != params.end(); ++it) {
        QJsonObject imageObj;
        imageObj["FilePath"] = it.value().filePath;
        jsonParams[it.key()] = imageObj;
    }
    executeBatchTask("DisplayCameraImage", jsonParams);
}

void BatchTaskOperator::addKeepAliveList(const QMap<QString, AppListParam> &params)
{
    QJsonObject jsonParams;
    for (auto it = params.begin(); it != params.end(); ++it) {
        QJsonObject listObj;
        QJsonArray appArray;
        for (const QString &app : it.value().appList) {
            appArray.append(app);
        }
        listObj["AppList"] = appArray;
        jsonParams[it.key()] = listObj;
    }
    executeBatchTask("AddKeepAliveList", jsonParams);
}

void BatchTaskOperator::removeKeepAliveList(const QMap<QString, AppListParam> &params)
{
    QJsonObject jsonParams;
    for (auto it = params.begin(); it != params.end(); ++it) {
        QJsonObject listObj;
        QJsonArray appArray;
        for (const QString &app : it.value().appList) {
            appArray.append(app);
        }
        listObj["AppList"] = appArray;
        jsonParams[it.key()] = listObj;
    }
    executeBatchTask("RemoveKeepAliveList", jsonParams);
}

void BatchTaskOperator::setKeepAliveList(const QMap<QString, AppListParam> &params)
{
    QJsonObject jsonParams;
    for (auto it = params.begin(); it != params.end(); ++it) {
        QJsonObject listObj;
        QJsonArray appArray;
        for (const QString &app : it.value().appList) {
            appArray.append(app);
        }
        listObj["AppList"] = appArray;
        jsonParams[it.key()] = listObj;
    }
    executeBatchTask("SetKeepAliveList", jsonParams);
}

void BatchTaskOperator::describeInstanceProperties(const QVector<QString> &instanceIds)
{
    QJsonObject jsonParams;
    for (const QString &id : instanceIds) {
        jsonParams[id] = QJsonObject(); // 空对象表示无参数
    }
    executeBatchTask("DescribeInstanceProperties", jsonParams);
}

void BatchTaskOperator::describeKeepFrontAppStatus(const QVector<QString> &instanceIds)
{
    QJsonObject jsonParams;
    for (const QString &id : instanceIds) {
        jsonParams[id] = QJsonObject(); // 空对象表示无参数
    }
    executeBatchTask("DescribeKeepFrontAppStatus", jsonParams);
}

void BatchTaskOperator::stopCameraMediaPlay(const QVector<QString> &instanceIds)
{
    QJsonObject jsonParams;
    for (const QString &id : instanceIds) {
        jsonParams[id] = QJsonObject(); // 空对象表示无参数
    }
    executeBatchTask("StopCameraMediaPlay", jsonParams);
}

void BatchTaskOperator::describeCameraMediaPlayStatus(const QVector<QString> &instanceIds)
{
    QJsonObject jsonParams;
    for (const QString &id : instanceIds) {
        jsonParams[id] = QJsonObject(); // 空对象表示无参数
    }
    executeBatchTask("DescribeCameraMediaPlayStatus", jsonParams);
}

void BatchTaskOperator::describeKeepAliveList(const QVector<QString> &instanceIds)
{
    QJsonObject jsonParams;
    for (const QString &id : instanceIds) {
        jsonParams[id] = QJsonObject(); // 空对象表示无参数
    }
    executeBatchTask("DescribeKeepAliveList", jsonParams);
}

void BatchTaskOperator::clearKeepAliveList(const QVector<QString> &instanceIds)
{
    QJsonObject jsonParams;
    for (const QString &id : instanceIds) {
        jsonParams[id] = QJsonObject(); // 空对象表示无参数
    }
    executeBatchTask("ClearKeepAliveList", jsonParams);
}

void BatchTaskOperator::listUserApps(const QVector<QString> &instanceIds)
{
    QJsonObject jsonParams;
    for (const QString &id : instanceIds) {
        jsonParams[id] = QJsonObject(); // 空对象表示无参数
    }
    executeBatchTask("ListUserApps", jsonParams);
}

void BatchTaskOperator::mute(const QMap<QString, MuteParam> &params)
{
    QJsonObject jsonParams;
    for (auto it = params.begin(); it != params.end(); ++it) {
        QJsonObject muteObj;
        muteObj["Mute"] = it.value().mute;
        jsonParams[it.key()] = muteObj;
    }
    executeBatchTask("Mute", jsonParams);
}

void BatchTaskOperator::mediaSearch(const QMap<QString, MediaSearchParam> &params)
{
    QJsonObject jsonParams;
    for (auto it = params.begin(); it != params.end(); ++it) {
        QJsonObject searchObj;
        searchObj["Keyword"] = it.value().keyword;
        jsonParams[it.key()] = searchObj;
    }
    executeBatchTask("MediaSearch", jsonParams);
}

void BatchTaskOperator::reboot(const QVector<QString> &instanceIds)
{
    QJsonObject jsonParams;
    for (const QString &id : instanceIds) {
        jsonParams[id] = QJsonObject(); // 空对象表示无参数
    }
    executeBatchTask("Reboot", jsonParams);
}

void BatchTaskOperator::listAllApps(const QVector<QString> &instanceIds)
{
    QJsonObject jsonParams;
    for (const QString &id : instanceIds) {
        jsonParams[id] = QJsonObject(); // 空对象表示无参数
    }
    executeBatchTask("ListAllApps", jsonParams);
}

void BatchTaskOperator::moveAppBackground(const QVector<QString> &instanceIds)
{
    QJsonObject jsonParams;
    for (const QString &id : instanceIds) {
        jsonParams[id] = QJsonObject(); // 空对象表示无参数
    }
    executeBatchTask("MoveAppBackground", jsonParams);
}

void BatchTaskOperator::addAppInstallBlackList(const QMap<QString, AppListParam> &params)
{
    QJsonObject jsonParams;
    for (auto it = params.begin(); it != params.end(); ++it) {
        QJsonObject listObj;
        QJsonArray appArray;
        for (const QString &app : it.value().appList) {
            appArray.append(app);
        }
        listObj["AppList"] = appArray;
        jsonParams[it.key()] = listObj;
    }
    executeBatchTask("AddAppInstallBlackList", jsonParams);
}

void BatchTaskOperator::removeAppInstallBlackList(const QMap<QString, AppListParam> &params)
{
    QJsonObject jsonParams;
    for (auto it = params.begin(); it != params.end(); ++it) {
        QJsonObject listObj;
        QJsonArray appArray;
        for (const QString &app : it.value().appList) {
            appArray.append(app);
        }
        listObj["AppList"] = appArray;
        jsonParams[it.key()] = listObj;
    }
    executeBatchTask("RemoveAppInstallBlackList", jsonParams);
}

void BatchTaskOperator::setAppInstallBlackList(const QMap<QString, AppListParam> &params)
{
    QJsonObject jsonParams;
    for (auto it = params.begin(); it != params.end(); ++it) {
        QJsonObject listObj;
        QJsonArray appArray;
        for (const QString &app : it.value().appList) {
            appArray.append(app);
        }
        listObj["AppList"] = appArray;
        jsonParams[it.key()] = listObj;
    }
    executeBatchTask("SetAppInstallBlackList", jsonParams);
}

void BatchTaskOperator::describeAppInstallBlackList(const QVector<QString> &instanceIds)
{
    QJsonObject jsonParams;
    for (const QString &id : instanceIds) {
        jsonParams[id] = QJsonObject(); // 空对象表示无参数
    }
    executeBatchTask("DescribeAppInstallBlackList", jsonParams);
}

void BatchTaskOperator::clearAppInstallBlackList(const QVector<QString> &instanceIds)
{
    QJsonObject jsonParams;
    for (const QString &id : instanceIds) {
        jsonParams[id] = QJsonObject(); // 空对象表示无参数
    }
    executeBatchTask("ClearAppInstallBlackList", jsonParams);
}

QString BatchTaskOperator::getInstanceImage(const QString &instanceId, int quality)
{
    constexpr int BUFFER_SIZE = 4096;
    std::vector<char> buffer(BUFFER_SIZE, 0);

    TcrClientHandle client = tcr_client_get_instance();
    TcrAndroidInstance tcrOperator = tcr_client_get_android_instance(client);
    if (tcrOperator == nullptr) {
        Logger::error("tcr_client_init() should be called before calling this function");
        return QString();
    }

    if (tcr_instance_get_image(tcrOperator, buffer.data(), BUFFER_SIZE, instanceId.toUtf8().constData(), 0, 0, quality)) {
        return QString::fromUtf8(buffer.data());
    }
    return QString();
}

BatchTaskOperator::BatchResult BatchTaskOperator::parseBatchResult(const QByteArray &jsonData)
{
    BatchResult result;
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        Logger::error("Failed to parse batch result JSON: " + parseError.errorString());
        return result;
    }

    if (!doc.isObject()) {
        Logger::error("Batch result is not a JSON object");
        return result;
    }

    QJsonObject root = doc.object();

    for (auto it = root.begin(); it != root.end(); ++it) {
        QString instanceId = it.key();
        QJsonValue value = it.value();
        if (!value.isObject()) continue;
        
        QJsonObject obj = value.toObject();
        int code = obj.value("Code").toInt();

        ResultItem resItem;
        resItem.instanceId = instanceId;
        resItem.message = QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));

        if (code == 0) {
            result.successItems.append(resItem);
        } else {
            result.failureItems.append(resItem);
        }
    }

    return result;
}

void BatchTaskOperator::executeBatchTask(const QString &taskType, const QJsonObject &params)
{
    // 构建输入JSON
    QJsonObject inputJson;
    inputJson["TaskType"] = taskType;
    inputJson["Params"] = params;
    
    QJsonDocument doc(inputJson);
    QString jsonString = doc.toJson(QJsonDocument::Compact);
    
    char* output = nullptr;
    size_t output_size = 0;

    TcrClientHandle client = tcr_client_get_instance();
    TcrAndroidInstance tcrOperator = tcr_client_get_android_instance(client);
    if (tcrOperator == nullptr) {
        Logger::error("tcr_client_init() should be called before calling this function");
        return;
    }

    TcrErrorCode result = tcr_instance_request(
        tcrOperator,
        jsonString.toUtf8().constData(),
        &output,
        &output_size
    );

    if (result != TCR_SUCCESS) {
        Logger::error("Batch task failed with error code: " + QString::number(static_cast<int>(result)));
        emit batchTaskFailed(static_cast<int>(result), "Batch task execution failed");
        return;
    }

    // 解析结果
    QByteArray jsonData(output, (output_size > 0 && output[output_size-1] == '\0') ? output_size-1 : output_size);
    BatchResult batchResult = parseBatchResult(jsonData);

    emit batchTaskCompleted(batchResult);

    // 释放结果内存
    if (output) {
        tcr_instance_free_result(tcrOperator, output);
    }
}
