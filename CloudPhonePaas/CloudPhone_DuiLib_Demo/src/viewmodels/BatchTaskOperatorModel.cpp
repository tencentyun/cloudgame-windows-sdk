#include "BatchTaskOperatorModel.h"

#include <sstream>

#include <json/json.h>

#include "utils/Logger.h"

BatchTaskOperatorModel::BatchTaskOperatorModel(BatchTaskOperator* batchTaskOperator)
    : m_batchTaskOperator(batchTaskOperator) {
    m_batchTaskOperator->onBatchTaskCompleted = [this](const BatchTaskOperator::BatchResult& result) {
        onBatchTaskCompleted(result);
    };
    m_batchTaskOperator->onBatchTaskFailed = [this](int errorCode, const std::string& errorMessage) {
        onBatchTaskFailed(errorCode, errorMessage);
    };
}

static double toDouble(const std::map<std::string, std::string>& params, const std::string& key, double def = 0.0) {
    auto it = params.find(key);
    if (it == params.end()) return def;
    try { return std::stod(it->second); } catch (...) { return def; }
}

static int toInt(const std::map<std::string, std::string>& params, const std::string& key, int def = 0) {
    auto it = params.find(key);
    if (it == params.end()) return def;
    try { return std::stoi(it->second); } catch (...) { return def; }
}

static std::string toStr(const std::map<std::string, std::string>& params, const std::string& key) {
    auto it = params.find(key);
    return it != params.end() ? it->second : "";
}

static bool toBool(const std::map<std::string, std::string>& params, const std::string& key) {
    auto it = params.find(key);
    if (it == params.end()) return false;
    return it->second == "1" || it->second == "true";
}

static std::vector<std::string> splitComma(const std::string& s) {
    std::vector<std::string> result;
    std::istringstream stream(s);
    std::string item;
    while (std::getline(stream, item, ',')) {
        // Trim whitespace
        size_t start = item.find_first_not_of(" \t");
        size_t end = item.find_last_not_of(" \t");
        if (start != std::string::npos)
            result.push_back(item.substr(start, end - start + 1));
    }
    return result;
}

void BatchTaskOperatorModel::handleDialogSignal(const std::string& dialogType,
                                                const std::vector<std::string>& instanceIds,
                                                const std::map<std::string, std::string>& params) {
    Logger::debug("[BatchTaskOperatorModel::handleDialogSignal] dialogType: " + dialogType);

    if (instanceIds.empty()) {
        if (onShowDialog) onShowDialog("提示", "请先选择实例");
        return;
    }

    if (dialogType == "gps") {
        BatchTaskOperator::GPSInfo gpsInfo;
        gpsInfo.longitude = toDouble(params, "longitude");
        gpsInfo.latitude = toDouble(params, "latitude");
        std::map<std::string, BatchTaskOperator::GPSInfo> gpsParams;
        for (const auto& id : instanceIds) gpsParams[id] = gpsInfo;
        m_batchTaskOperator->modifyGPS(gpsParams);

    } else if (dialogType == "resolution") {
        BatchTaskOperator::Resolution resolution;
        resolution.width = toInt(params, "width");
        resolution.height = toInt(params, "height");
        resolution.dpi = toInt(params, "dpi");
        std::map<std::string, BatchTaskOperator::Resolution> resParams;
        for (const auto& id : instanceIds) resParams[id] = resolution;
        m_batchTaskOperator->modifyResolution(resParams);

    } else if (dialogType == "paste") {
        BatchTaskOperator::TextParam textParam;
        textParam.text = toStr(params, "text");
        std::map<std::string, BatchTaskOperator::TextParam> textParams;
        for (const auto& id : instanceIds) textParams[id] = textParam;
        m_batchTaskOperator->paste(textParams);

    } else if (dialogType == "sendClipboard") {
        BatchTaskOperator::TextParam textParam;
        textParam.text = toStr(params, "text");
        std::map<std::string, BatchTaskOperator::TextParam> textParams;
        for (const auto& id : instanceIds) textParams[id] = textParam;
        m_batchTaskOperator->sendClipboard(textParams);

    } else if (dialogType == "modifySensor") {
        BatchTaskOperator::SensorData sensorData;
        sensorData.type = toStr(params, "sensorType");
        std::string valuesStr = toStr(params, "values");
        // Parse comma-separated values "x,y,z"
        auto parts = splitComma(valuesStr);
        for (const auto& p : parts) {
            try { sensorData.values.push_back(std::stod(p)); } catch (...) {}
        }
        sensorData.accuracy = toInt(params, "accuracy", 3);
        std::map<std::string, BatchTaskOperator::SensorData> sensorParams;
        for (const auto& id : instanceIds) sensorParams[id] = sensorData;
        m_batchTaskOperator->modifySensor(sensorParams);

    } else if (dialogType == "shake") {
        m_batchTaskOperator->shake(instanceIds);

    } else if (dialogType == "blow") {
        m_batchTaskOperator->blow(instanceIds);

    } else if (dialogType == "message") {
        BatchTaskOperator::TransMessage transMessage;
        transMessage.packageName = toStr(params, "packageName");
        transMessage.msg = toStr(params, "msg");
        std::map<std::string, BatchTaskOperator::TransMessage> msgParams;
        for (const auto& id : instanceIds) msgParams[id] = transMessage;
        m_batchTaskOperator->sendTransMessage(msgParams);

    } else if (dialogType == "modifyInstanceProperties") {
        std::string jsonStr = toStr(params, "json");
        Json::Value jsonObj;
        Json::CharReaderBuilder builder;
        std::string errors;
        std::istringstream stream(jsonStr);
        if (!Json::parseFromStream(builder, stream, &jsonObj, &errors)) {
            if (onShowDialog) onShowDialog("错误", "修改实例属性参数解析失败");
            return;
        }
        BatchTaskOperator::InstanceProperties properties;
        if (jsonObj.isMember("DeviceInfo")) properties.deviceInfo = jsonObj["DeviceInfo"];
        if (jsonObj.isMember("ProxyInfo")) properties.proxyInfo = jsonObj["ProxyInfo"];
        if (jsonObj.isMember("GPSInfo")) {
            properties.gpsInfo.longitude = jsonObj["GPSInfo"].get("Longitude", 0.0).asDouble();
            properties.gpsInfo.latitude = jsonObj["GPSInfo"].get("Latitude", 0.0).asDouble();
        }
        if (jsonObj.isMember("SIMInfo")) properties.simInfo = jsonObj["SIMInfo"];
        if (jsonObj.isMember("LocaleInfo")) properties.localeInfo = jsonObj["LocaleInfo"];
        if (jsonObj.isMember("LanguageInfo")) properties.languageInfo = jsonObj["LanguageInfo"];
        if (jsonObj.isMember("ExtraProperties")) properties.extraProperties = jsonObj["ExtraProperties"];

        std::map<std::string, BatchTaskOperator::InstanceProperties> propParams;
        for (const auto& id : instanceIds) propParams[id] = properties;
        m_batchTaskOperator->modifyInstanceProperties(propParams);

    } else if (dialogType == "modifyKeepFrontAppStatus") {
        BatchTaskOperator::KeepFrontAppStatus status;
        status.packageName = toStr(params, "packageName");
        status.enable = toBool(params, "enable");
        status.restartIntervalSeconds = toInt(params, "restartInterval");
        std::map<std::string, BatchTaskOperator::KeepFrontAppStatus> statusParams;
        for (const auto& id : instanceIds) statusParams[id] = status;
        m_batchTaskOperator->modifyKeepFrontAppStatus(statusParams);

    } else if (dialogType == "unInstallByPackageName") {
        BatchTaskOperator::PackageNameParam pkg;
        pkg.packageName = toStr(params, "packageName");
        std::map<std::string, BatchTaskOperator::PackageNameParam> pkgParams;
        for (const auto& id : instanceIds) pkgParams[id] = pkg;
        m_batchTaskOperator->unInstallByPackageName(pkgParams);

    } else if (dialogType == "startApp") {
        BatchTaskOperator::StartAppParam startApp;
        startApp.packageName = toStr(params, "packageName");
        startApp.activityName = toStr(params, "activityName");
        std::map<std::string, BatchTaskOperator::StartAppParam> startParams;
        for (const auto& id : instanceIds) startParams[id] = startApp;
        m_batchTaskOperator->startApp(startParams);

    } else if (dialogType == "stopApp") {
        BatchTaskOperator::PackageNameParam pkg;
        pkg.packageName = toStr(params, "packageName");
        std::map<std::string, BatchTaskOperator::PackageNameParam> pkgParams;
        for (const auto& id : instanceIds) pkgParams[id] = pkg;
        m_batchTaskOperator->stopApp(pkgParams);

    } else if (dialogType == "clearAppData") {
        BatchTaskOperator::PackageNameParam pkg;
        pkg.packageName = toStr(params, "packageName");
        std::map<std::string, BatchTaskOperator::PackageNameParam> pkgParams;
        for (const auto& id : instanceIds) pkgParams[id] = pkg;
        m_batchTaskOperator->clearAppData(pkgParams);

    } else if (dialogType == "enableApp") {
        BatchTaskOperator::PackageNameParam pkg;
        pkg.packageName = toStr(params, "packageName");
        std::map<std::string, BatchTaskOperator::PackageNameParam> pkgParams;
        for (const auto& id : instanceIds) pkgParams[id] = pkg;
        m_batchTaskOperator->enableApp(pkgParams);

    } else if (dialogType == "disableApp") {
        BatchTaskOperator::PackageNameParam pkg;
        pkg.packageName = toStr(params, "packageName");
        std::map<std::string, BatchTaskOperator::PackageNameParam> pkgParams;
        for (const auto& id : instanceIds) pkgParams[id] = pkg;
        m_batchTaskOperator->disableApp(pkgParams);

    } else if (dialogType == "startCameraMediaPlay") {
        BatchTaskOperator::CameraMediaPlayParam mediaParam;
        mediaParam.filePath = toStr(params, "filePath");
        mediaParam.loops = toInt(params, "loops", 1);
        std::map<std::string, BatchTaskOperator::CameraMediaPlayParam> mediaParams;
        for (const auto& id : instanceIds) mediaParams[id] = mediaParam;
        m_batchTaskOperator->startCameraMediaPlay(mediaParams);

    } else if (dialogType == "displayCameraImage") {
        BatchTaskOperator::CameraImageParam imageParam;
        imageParam.filePath = toStr(params, "filePath");
        std::map<std::string, BatchTaskOperator::CameraImageParam> imageParams;
        for (const auto& id : instanceIds) imageParams[id] = imageParam;
        m_batchTaskOperator->displayCameraImage(imageParams);

    } else if (dialogType == "addKeepAliveList") {
        BatchTaskOperator::AppListParam appListParam;
        appListParam.appList = splitComma(toStr(params, "appList"));
        std::map<std::string, BatchTaskOperator::AppListParam> appParams;
        for (const auto& id : instanceIds) appParams[id] = appListParam;
        m_batchTaskOperator->addKeepAliveList(appParams);

    } else if (dialogType == "removeKeepAliveList") {
        BatchTaskOperator::AppListParam appListParam;
        appListParam.appList = splitComma(toStr(params, "appList"));
        std::map<std::string, BatchTaskOperator::AppListParam> appParams;
        for (const auto& id : instanceIds) appParams[id] = appListParam;
        m_batchTaskOperator->removeKeepAliveList(appParams);

    } else if (dialogType == "setKeepAliveList") {
        BatchTaskOperator::AppListParam appListParam;
        appListParam.appList = splitComma(toStr(params, "appList"));
        std::map<std::string, BatchTaskOperator::AppListParam> appParams;
        for (const auto& id : instanceIds) appParams[id] = appListParam;
        m_batchTaskOperator->setKeepAliveList(appParams);

    } else if (dialogType == "describeInstanceProperties") {
        m_batchTaskOperator->describeInstanceProperties(instanceIds);

    } else if (dialogType == "describeKeepFrontAppStatus") {
        m_batchTaskOperator->describeKeepFrontAppStatus(instanceIds);

    } else if (dialogType == "stopCameraMediaPlay") {
        m_batchTaskOperator->stopCameraMediaPlay(instanceIds);

    } else if (dialogType == "describeCameraMediaPlayStatus") {
        m_batchTaskOperator->describeCameraMediaPlayStatus(instanceIds);

    } else if (dialogType == "describeKeepAliveList") {
        m_batchTaskOperator->describeKeepAliveList(instanceIds);

    } else if (dialogType == "clearKeepAliveList") {
        m_batchTaskOperator->clearKeepAliveList(instanceIds);

    } else if (dialogType == "listUserApps") {
        m_batchTaskOperator->listUserApps(instanceIds);

    } else if (dialogType == "mute") {
        BatchTaskOperator::MuteParam muteParam;
        muteParam.mute = toBool(params, "mute");
        std::map<std::string, BatchTaskOperator::MuteParam> muteParams;
        for (const auto& id : instanceIds) muteParams[id] = muteParam;
        m_batchTaskOperator->mute(muteParams);

    } else if (dialogType == "mediaSearch") {
        BatchTaskOperator::MediaSearchParam searchParam;
        searchParam.keyword = toStr(params, "keyword");
        std::map<std::string, BatchTaskOperator::MediaSearchParam> searchParams;
        for (const auto& id : instanceIds) searchParams[id] = searchParam;
        m_batchTaskOperator->mediaSearch(searchParams);

    } else if (dialogType == "reboot") {
        m_batchTaskOperator->reboot(instanceIds);

    } else if (dialogType == "listAllApps") {
        m_batchTaskOperator->listAllApps(instanceIds);

    } else if (dialogType == "moveAppBackground") {
        m_batchTaskOperator->moveAppBackground(instanceIds);

    } else if (dialogType == "addAppInstallBlackList") {
        BatchTaskOperator::AppListParam appListParam;
        appListParam.appList = splitComma(toStr(params, "appList"));
        std::map<std::string, BatchTaskOperator::AppListParam> appParams;
        for (const auto& id : instanceIds) appParams[id] = appListParam;
        m_batchTaskOperator->addAppInstallBlackList(appParams);

    } else if (dialogType == "removeAppInstallBlackList") {
        BatchTaskOperator::AppListParam appListParam;
        appListParam.appList = splitComma(toStr(params, "appList"));
        std::map<std::string, BatchTaskOperator::AppListParam> appParams;
        for (const auto& id : instanceIds) appParams[id] = appListParam;
        m_batchTaskOperator->removeAppInstallBlackList(appParams);

    } else if (dialogType == "setAppInstallBlackList") {
        BatchTaskOperator::AppListParam appListParam;
        appListParam.appList = splitComma(toStr(params, "appList"));
        std::map<std::string, BatchTaskOperator::AppListParam> appParams;
        for (const auto& id : instanceIds) appParams[id] = appListParam;
        m_batchTaskOperator->setAppInstallBlackList(appParams);

    } else if (dialogType == "describeAppInstallBlackList") {
        m_batchTaskOperator->describeAppInstallBlackList(instanceIds);

    } else if (dialogType == "clearAppInstallBlackList") {
        m_batchTaskOperator->clearAppInstallBlackList(instanceIds);

    } else {
        if (onShowDialog) onShowDialog("提示", "当前没有处理该操作类型: " + dialogType);
    }
}

void BatchTaskOperatorModel::onBatchTaskCompleted(const BatchTaskOperator::BatchResult& result) {
    std::string message;

    if (!result.successItems.empty()) {
        message += "以下实例任务执行成功:\n";
        for (const auto& item : result.successItems) {
            message += item.instanceId + ": " + item.message + "\n";
        }
    }

    if (!result.failureItems.empty()) {
        if (!message.empty()) message += "\n";
        message += "以下实例任务执行失败:\n";
        for (const auto& item : result.failureItems) {
            message += item.instanceId + ": " + item.message + "\n";
        }
    }

    if (message.empty()) {
        message = "批量任务完成，但没有返回任何结果。";
    }

    if (onShowDialog) onShowDialog("批量任务结果", message);
}

void BatchTaskOperatorModel::onBatchTaskFailed(int errorCode, const std::string& errorMessage) {
    std::string message = "批量任务失败，错误码：" + std::to_string(errorCode) +
                          "，错误信息：" + errorMessage;
    if (onShowDialog) onShowDialog("批量任务错误", message);
}
