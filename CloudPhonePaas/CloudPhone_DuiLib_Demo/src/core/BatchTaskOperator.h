#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

#include <json/json.h>

#include "tcr_c_api.h"

/**
 * @brief Wraps tcr_instance_request() for batch operations on cloud phone instances.
 *
 * All 38 batch task types go through executeBatchTask() which calls tcr_instance_request().
 * Results are parsed into BatchResult with success/failure items.
 */
class BatchTaskOperator {
public:
    struct ResultItem {
        std::string instanceId;
        std::string message;
    };

    struct BatchResult {
        std::vector<ResultItem> successItems;
        std::vector<ResultItem> failureItems;
    };

    enum class SensorType {
        Accelerometer,
        Gyroscope,
        Unknown
    };

    // Callbacks
    std::function<void(const BatchResult&)> onBatchTaskCompleted;
    std::function<void(int, const std::string&)> onBatchTaskFailed;

    BatchTaskOperator() = default;

    // ---- Parameter structs ----

    struct Resolution {
        int width = 0;
        int height = 0;
        int dpi = 0;
    };

    struct GPSInfo {
        double longitude = 0.0;
        double latitude = 0.0;
    };

    struct TextParam {
        std::string text;
    };

    struct SensorData {
        std::string type;
        std::vector<double> values;
        int accuracy = 3;
    };

    struct TransMessage {
        std::string packageName;
        std::string msg;
    };

    struct InstanceProperties {
        Json::Value deviceInfo;
        Json::Value proxyInfo;
        GPSInfo gpsInfo;
        Json::Value simInfo;
        Json::Value localeInfo;
        Json::Value languageInfo;
        Json::Value extraProperties;  // Json::arrayValue
    };

    struct KeepFrontAppStatus {
        std::string packageName;
        bool enable = false;
        int restartIntervalSeconds = 0;
    };

    struct PackageNameParam {
        std::string packageName;
    };

    struct StartAppParam {
        std::string packageName;
        std::string activityName;
    };

    struct CameraMediaPlayParam {
        std::string filePath;
        int loops = 1;
    };

    struct CameraImageParam {
        std::string filePath;
    };

    struct AppListParam {
        std::vector<std::string> appList;
    };

    struct MuteParam {
        bool mute = false;
    };

    struct MediaSearchParam {
        std::string keyword;
    };

    // ---- Batch operations (38 types) ----

    void modifyResolution(const std::map<std::string, Resolution>& params);
    void modifyGPS(const std::map<std::string, GPSInfo>& params);
    void paste(const std::map<std::string, TextParam>& params);
    void sendClipboard(const std::map<std::string, TextParam>& params);
    void modifySensor(const std::map<std::string, SensorData>& params);
    void shake(const std::vector<std::string>& instanceIds);
    void blow(const std::vector<std::string>& instanceIds);
    void sendTransMessage(const std::map<std::string, TransMessage>& params);
    void modifyInstanceProperties(const std::map<std::string, InstanceProperties>& params);
    void modifyKeepFrontAppStatus(const std::map<std::string, KeepFrontAppStatus>& params);
    void unInstallByPackageName(const std::map<std::string, PackageNameParam>& params);
    void startApp(const std::map<std::string, StartAppParam>& params);
    void stopApp(const std::map<std::string, PackageNameParam>& params);
    void clearAppData(const std::map<std::string, PackageNameParam>& params);
    void enableApp(const std::map<std::string, PackageNameParam>& params);
    void disableApp(const std::map<std::string, PackageNameParam>& params);
    void startCameraMediaPlay(const std::map<std::string, CameraMediaPlayParam>& params);
    void displayCameraImage(const std::map<std::string, CameraImageParam>& params);
    void addKeepAliveList(const std::map<std::string, AppListParam>& params);
    void removeKeepAliveList(const std::map<std::string, AppListParam>& params);
    void setKeepAliveList(const std::map<std::string, AppListParam>& params);
    void describeInstanceProperties(const std::vector<std::string>& instanceIds);
    void describeKeepFrontAppStatus(const std::vector<std::string>& instanceIds);
    void stopCameraMediaPlay(const std::vector<std::string>& instanceIds);
    void describeCameraMediaPlayStatus(const std::vector<std::string>& instanceIds);
    void describeKeepAliveList(const std::vector<std::string>& instanceIds);
    void clearKeepAliveList(const std::vector<std::string>& instanceIds);
    void listUserApps(const std::vector<std::string>& instanceIds);
    void mute(const std::map<std::string, MuteParam>& params);
    void mediaSearch(const std::map<std::string, MediaSearchParam>& params);
    void reboot(const std::vector<std::string>& instanceIds);
    void listAllApps(const std::vector<std::string>& instanceIds);
    void moveAppBackground(const std::vector<std::string>& instanceIds);
    void addAppInstallBlackList(const std::map<std::string, AppListParam>& params);
    void removeAppInstallBlackList(const std::map<std::string, AppListParam>& params);
    void setAppInstallBlackList(const std::map<std::string, AppListParam>& params);
    void describeAppInstallBlackList(const std::vector<std::string>& instanceIds);
    void clearAppInstallBlackList(const std::vector<std::string>& instanceIds);

    // Get instance screenshot URL
    std::string getInstanceImage(const std::string& instanceId, int quality = 20);

private:
    BatchResult parseBatchResult(const std::string& jsonData);
    void executeBatchTask(const std::string& taskType, const Json::Value& params);
    void executeNoParamTask(const std::string& taskType, const std::vector<std::string>& instanceIds);
    void executeAppListTask(const std::string& taskType, const std::map<std::string, AppListParam>& params);
    void executePackageNameTask(const std::string& taskType, const std::map<std::string, PackageNameParam>& params);
};
