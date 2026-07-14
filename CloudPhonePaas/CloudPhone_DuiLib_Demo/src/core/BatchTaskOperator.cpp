#include "BatchTaskOperator.h"

#include <vector>

#include "tcr_c_api.h"
#include "utils/Logger.h"

// Helper: execute a no-param batch task (instanceIds map to empty objects)
void BatchTaskOperator::executeNoParamTask(const std::string& taskType,
                                           const std::vector<std::string>& instanceIds) {
    Json::Value jsonParams(Json::objectValue);
    for (const auto& id : instanceIds) {
        jsonParams[id] = Json::Value(Json::objectValue);
    }
    executeBatchTask(taskType, jsonParams);
}

// Helper: execute an AppList batch task
void BatchTaskOperator::executeAppListTask(const std::string& taskType,
                                           const std::map<std::string, AppListParam>& params) {
    Json::Value jsonParams(Json::objectValue);
    for (const auto& [id, param] : params) {
        Json::Value listObj(Json::objectValue);
        Json::Value appArray(Json::arrayValue);
        for (const auto& app : param.appList) {
            appArray.append(app);
        }
        listObj["AppList"] = appArray;
        jsonParams[id] = listObj;
    }
    executeBatchTask(taskType, jsonParams);
}

// Helper: execute a PackageName batch task
void BatchTaskOperator::executePackageNameTask(const std::string& taskType,
                                               const std::map<std::string, PackageNameParam>& params) {
    Json::Value jsonParams(Json::objectValue);
    for (const auto& [id, param] : params) {
        Json::Value pkgObj(Json::objectValue);
        pkgObj["PackageName"] = param.packageName;
        jsonParams[id] = pkgObj;
    }
    executeBatchTask(taskType, jsonParams);
}

void BatchTaskOperator::modifyResolution(const std::map<std::string, Resolution>& params) {
    Json::Value jsonParams(Json::objectValue);
    for (const auto& [id, res] : params) {
        Json::Value resObj(Json::objectValue);
        resObj["Width"] = res.width;
        resObj["Height"] = res.height;
        resObj["DPI"] = res.dpi;
        jsonParams[id] = resObj;
    }
    executeBatchTask("ModifyResolution", jsonParams);
}

void BatchTaskOperator::modifyGPS(const std::map<std::string, GPSInfo>& params) {
    Json::Value jsonParams(Json::objectValue);
    for (const auto& [id, gps] : params) {
        Json::Value gpsObj(Json::objectValue);
        gpsObj["Longitude"] = gps.longitude;
        gpsObj["Latitude"] = gps.latitude;
        jsonParams[id] = gpsObj;
    }
    executeBatchTask("ModifyGPS", jsonParams);
}

void BatchTaskOperator::paste(const std::map<std::string, TextParam>& params) {
    Json::Value jsonParams(Json::objectValue);
    for (const auto& [id, param] : params) {
        Json::Value textObj(Json::objectValue);
        textObj["Text"] = param.text;
        jsonParams[id] = textObj;
    }
    executeBatchTask("Paste", jsonParams);
}

void BatchTaskOperator::sendClipboard(const std::map<std::string, TextParam>& params) {
    Json::Value jsonParams(Json::objectValue);
    for (const auto& [id, param] : params) {
        Json::Value textObj(Json::objectValue);
        textObj["Text"] = param.text;
        jsonParams[id] = textObj;
    }
    executeBatchTask("SendClipboard", jsonParams);
}

void BatchTaskOperator::modifySensor(const std::map<std::string, SensorData>& params) {
    Json::Value jsonParams(Json::objectValue);
    for (const auto& [id, sensor] : params) {
        Json::Value sensorObj(Json::objectValue);
        sensorObj["Type"] = sensor.type;
        Json::Value valuesArray(Json::arrayValue);
        for (double val : sensor.values) {
            valuesArray.append(val);
        }
        sensorObj["Values"] = valuesArray;
        sensorObj["Accuracy"] = sensor.accuracy;
        jsonParams[id] = sensorObj;
    }
    executeBatchTask("ModifySensor", jsonParams);
}

void BatchTaskOperator::shake(const std::vector<std::string>& instanceIds) {
    executeNoParamTask("Shake", instanceIds);
}

void BatchTaskOperator::blow(const std::vector<std::string>& instanceIds) {
    executeNoParamTask("Blow", instanceIds);
}

void BatchTaskOperator::sendTransMessage(const std::map<std::string, TransMessage>& params) {
    Json::Value jsonParams(Json::objectValue);
    for (const auto& [id, msg] : params) {
        Json::Value msgObj(Json::objectValue);
        msgObj["PackageName"] = msg.packageName;
        msgObj["Msg"] = msg.msg;
        jsonParams[id] = msgObj;
    }
    executeBatchTask("SendTransMessage", jsonParams);
}

void BatchTaskOperator::modifyInstanceProperties(const std::map<std::string, InstanceProperties>& params) {
    Json::Value jsonParams(Json::objectValue);
    for (const auto& [id, prop] : params) {
        Json::Value propObj(Json::objectValue);
        propObj["DeviceInfo"] = prop.deviceInfo;
        propObj["ProxyInfo"] = prop.proxyInfo;

        Json::Value gpsObj(Json::objectValue);
        gpsObj["Longitude"] = prop.gpsInfo.longitude;
        gpsObj["Latitude"] = prop.gpsInfo.latitude;
        propObj["GPSInfo"] = gpsObj;

        propObj["SIMInfo"] = prop.simInfo;
        propObj["LocaleInfo"] = prop.localeInfo;
        propObj["LanguageInfo"] = prop.languageInfo;
        propObj["ExtraProperties"] = prop.extraProperties;
        jsonParams[id] = propObj;
    }
    executeBatchTask("ModifyInstanceProperties", jsonParams);
}

void BatchTaskOperator::modifyKeepFrontAppStatus(const std::map<std::string, KeepFrontAppStatus>& params) {
    Json::Value jsonParams(Json::objectValue);
    for (const auto& [id, status] : params) {
        Json::Value statusObj(Json::objectValue);
        statusObj["PackageName"] = status.packageName;
        statusObj["Enable"] = status.enable;
        statusObj["RestartInterValSeconds"] = status.restartIntervalSeconds;
        jsonParams[id] = statusObj;
    }
    executeBatchTask("ModifyKeepFrontAppStatus", jsonParams);
}

void BatchTaskOperator::unInstallByPackageName(const std::map<std::string, PackageNameParam>& params) {
    executePackageNameTask("UnInstallByPackageName", params);
}

void BatchTaskOperator::startApp(const std::map<std::string, StartAppParam>& params) {
    Json::Value jsonParams(Json::objectValue);
    for (const auto& [id, app] : params) {
        Json::Value appObj(Json::objectValue);
        appObj["PackageName"] = app.packageName;
        appObj["ActivityName"] = app.activityName;
        jsonParams[id] = appObj;
    }
    executeBatchTask("StartApp", jsonParams);
}

void BatchTaskOperator::stopApp(const std::map<std::string, PackageNameParam>& params) {
    executePackageNameTask("StopApp", params);
}

void BatchTaskOperator::clearAppData(const std::map<std::string, PackageNameParam>& params) {
    executePackageNameTask("ClearAppData", params);
}

void BatchTaskOperator::enableApp(const std::map<std::string, PackageNameParam>& params) {
    executePackageNameTask("EnableApp", params);
}

void BatchTaskOperator::disableApp(const std::map<std::string, PackageNameParam>& params) {
    executePackageNameTask("DisableApp", params);
}

void BatchTaskOperator::startCameraMediaPlay(const std::map<std::string, CameraMediaPlayParam>& params) {
    Json::Value jsonParams(Json::objectValue);
    for (const auto& [id, media] : params) {
        Json::Value mediaObj(Json::objectValue);
        mediaObj["FilePath"] = media.filePath;
        mediaObj["Loops"] = media.loops;
        jsonParams[id] = mediaObj;
    }
    executeBatchTask("StartCameraMediaPlay", jsonParams);
}

void BatchTaskOperator::displayCameraImage(const std::map<std::string, CameraImageParam>& params) {
    Json::Value jsonParams(Json::objectValue);
    for (const auto& [id, img] : params) {
        Json::Value imageObj(Json::objectValue);
        imageObj["FilePath"] = img.filePath;
        jsonParams[id] = imageObj;
    }
    executeBatchTask("DisplayCameraImage", jsonParams);
}

void BatchTaskOperator::addKeepAliveList(const std::map<std::string, AppListParam>& params) {
    executeAppListTask("AddKeepAliveList", params);
}

void BatchTaskOperator::removeKeepAliveList(const std::map<std::string, AppListParam>& params) {
    executeAppListTask("RemoveKeepAliveList", params);
}

void BatchTaskOperator::setKeepAliveList(const std::map<std::string, AppListParam>& params) {
    executeAppListTask("SetKeepAliveList", params);
}

void BatchTaskOperator::describeInstanceProperties(const std::vector<std::string>& instanceIds) {
    executeNoParamTask("DescribeInstanceProperties", instanceIds);
}

void BatchTaskOperator::describeKeepFrontAppStatus(const std::vector<std::string>& instanceIds) {
    executeNoParamTask("DescribeKeepFrontAppStatus", instanceIds);
}

void BatchTaskOperator::stopCameraMediaPlay(const std::vector<std::string>& instanceIds) {
    executeNoParamTask("StopCameraMediaPlay", instanceIds);
}

void BatchTaskOperator::describeCameraMediaPlayStatus(const std::vector<std::string>& instanceIds) {
    executeNoParamTask("DescribeCameraMediaPlayStatus", instanceIds);
}

void BatchTaskOperator::describeKeepAliveList(const std::vector<std::string>& instanceIds) {
    executeNoParamTask("DescribeKeepAliveList", instanceIds);
}

void BatchTaskOperator::clearKeepAliveList(const std::vector<std::string>& instanceIds) {
    executeNoParamTask("ClearKeepAliveList", instanceIds);
}

void BatchTaskOperator::listUserApps(const std::vector<std::string>& instanceIds) {
    executeNoParamTask("ListUserApps", instanceIds);
}

void BatchTaskOperator::mute(const std::map<std::string, MuteParam>& params) {
    Json::Value jsonParams(Json::objectValue);
    for (const auto& [id, param] : params) {
        Json::Value muteObj(Json::objectValue);
        muteObj["Mute"] = param.mute;
        jsonParams[id] = muteObj;
    }
    executeBatchTask("Mute", jsonParams);
}

void BatchTaskOperator::mediaSearch(const std::map<std::string, MediaSearchParam>& params) {
    Json::Value jsonParams(Json::objectValue);
    for (const auto& [id, param] : params) {
        Json::Value searchObj(Json::objectValue);
        searchObj["Keyword"] = param.keyword;
        jsonParams[id] = searchObj;
    }
    executeBatchTask("MediaSearch", jsonParams);
}

void BatchTaskOperator::reboot(const std::vector<std::string>& instanceIds) {
    executeNoParamTask("Reboot", instanceIds);
}

void BatchTaskOperator::listAllApps(const std::vector<std::string>& instanceIds) {
    executeNoParamTask("ListAllApps", instanceIds);
}

void BatchTaskOperator::moveAppBackground(const std::vector<std::string>& instanceIds) {
    executeNoParamTask("MoveAppBackground", instanceIds);
}

void BatchTaskOperator::addAppInstallBlackList(const std::map<std::string, AppListParam>& params) {
    executeAppListTask("AddAppInstallBlackList", params);
}

void BatchTaskOperator::removeAppInstallBlackList(const std::map<std::string, AppListParam>& params) {
    executeAppListTask("RemoveAppInstallBlackList", params);
}

void BatchTaskOperator::setAppInstallBlackList(const std::map<std::string, AppListParam>& params) {
    executeAppListTask("SetAppInstallBlackList", params);
}

void BatchTaskOperator::describeAppInstallBlackList(const std::vector<std::string>& instanceIds) {
    executeNoParamTask("DescribeAppInstallBlackList", instanceIds);
}

void BatchTaskOperator::clearAppInstallBlackList(const std::vector<std::string>& instanceIds) {
    executeNoParamTask("ClearAppInstallBlackList", instanceIds);
}

std::string BatchTaskOperator::getInstanceImage(const std::string& instanceId, int quality) {
    constexpr int BUFFER_SIZE = 4096;
    std::vector<char> buffer(BUFFER_SIZE, 0);

    TcrClientHandle client = tcr_client_get_instance();
    TcrAndroidInstance tcrOperator = tcr_client_get_android_instance(client);
    if (tcrOperator == nullptr) {
        Logger::error("tcr_client_init() should be called before calling getInstanceImage");
        return {};
    }

    if (tcr_instance_get_image(tcrOperator, buffer.data(), BUFFER_SIZE,
                               instanceId.c_str(), 0, 0, quality)) {
        return std::string(buffer.data());
    }
    return {};
}

BatchTaskOperator::BatchResult BatchTaskOperator::parseBatchResult(const std::string& jsonData) {
    BatchResult result;
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errors;
    std::istringstream stream(jsonData);

    if (!Json::parseFromStream(builder, stream, &root, &errors)) {
        Logger::error("Failed to parse batch result JSON: " + errors);
        return result;
    }

    if (!root.isObject()) {
        Logger::error("Batch result is not a JSON object");
        return result;
    }

    for (const auto& key : root.getMemberNames()) {
        const Json::Value& value = root[key];
        if (!value.isObject()) continue;

        int code = value.get("Code", -1).asInt();

        ResultItem item;
        item.instanceId = key;
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "";
        item.message = Json::writeString(writerBuilder, value);

        if (code == 0) {
            result.successItems.push_back(item);
        } else {
            result.failureItems.push_back(item);
        }
    }

    return result;
}

void BatchTaskOperator::executeBatchTask(const std::string& taskType, const Json::Value& params) {
    Json::Value inputJson(Json::objectValue);
    inputJson["TaskType"] = taskType;
    inputJson["Params"] = params;

    Json::StreamWriterBuilder writerBuilder;
    writerBuilder["indentation"] = "";
    std::string jsonString = Json::writeString(writerBuilder, inputJson);

    char* output = nullptr;
    size_t output_size = 0;

    TcrClientHandle client = tcr_client_get_instance();
    TcrAndroidInstance tcrOperator = tcr_client_get_android_instance(client);
    if (tcrOperator == nullptr) {
        Logger::error("tcr_client_init() should be called before calling executeBatchTask");
        return;
    }

    TcrErrorCode result = tcr_instance_request(tcrOperator, jsonString.c_str(), &output, &output_size);

    if (result != TCR_SUCCESS) {
        Logger::error("Batch task failed with error code: " + std::to_string(static_cast<int>(result)));
        if (onBatchTaskFailed) {
            onBatchTaskFailed(static_cast<int>(result), "Batch task execution failed");
        }
        return;
    }

    std::string jsonData(output, (output_size > 0 && output[output_size - 1] == '\0')
                                     ? output_size - 1
                                     : output_size);
    BatchResult batchResult = parseBatchResult(jsonData);

    if (onBatchTaskCompleted) {
        onBatchTaskCompleted(batchResult);
    }

    if (output) {
        tcr_instance_free_result(tcrOperator, output);
    }
}
