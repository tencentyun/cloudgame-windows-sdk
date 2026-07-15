#include "AppConfig.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutexLocker>

AppConfig* AppConfig::s_instance = nullptr;
QMutex AppConfig::s_mutex;

AppConfig* AppConfig::instance() {
  QMutexLocker locker(&s_mutex);
  if (!s_instance) {
    s_instance = new AppConfig();
  }
  return s_instance;
}

AppConfig::AppConfig(QObject* parent) : QObject(parent) {
  // 1. 先加载默认配置（位于 config/ 子目录下）
  load(QCoreApplication::applicationDirPath() + "/config/config.json");
  // 2. 再扫描 config/ 子目录中的可选配置
  scanConfigs();
}

// ----------------------------------------------------------------------------
// 私有：从指定文件路径加载配置
// ----------------------------------------------------------------------------
void AppConfig::load(const QString& filePath) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return;
  }

  QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
  file.close();

  if (doc.isNull() || !doc.isObject()) {
    return;
  }

  QJsonObject obj = doc.object();
  if (obj.contains("baseUrl") && obj["baseUrl"].isString()) {
    m_baseUrl = obj["baseUrl"].toString();
  }
  if (obj.contains("apiPath") && obj["apiPath"].isString()) {
    m_apiPath = obj["apiPath"].toString();
  }
  if (obj.contains("instanceIds") && obj["instanceIds"].isString()) {
    m_instanceIds = obj["instanceIds"].toString();
  }
  if (obj.contains("appId")) {
    QJsonValue val = obj["appId"];
    if (val.isString()) {
      m_appId = val.toString();
    } else if (val.isDouble()) {
      m_appId = QString::number(static_cast<qlonglong>(val.toDouble()));
    }
  } else {
    m_appId = "";
  }
}

// ----------------------------------------------------------------------------
// 私有：扫描 <appDir>/config/ 目录，收集可用配置名
// ----------------------------------------------------------------------------
void AppConfig::scanConfigs() {
  QDir configDir(QCoreApplication::applicationDirPath() + "/config");
  if (!configDir.exists()) {
    return;
  }

  QStringList filters;
  filters << "*.json";
  QStringList files = configDir.entryList(filters, QDir::Files, QDir::Name);

  QStringList names;
  for (const QString& fileName : files) {
    // 去掉 .json 后缀
    names.append(fileName.left(fileName.length() - 5));
  }

  m_configNames = names;
  emit configNamesChanged();
}

// ----------------------------------------------------------------------------
// 公共槽：切换配置
// ----------------------------------------------------------------------------
void AppConfig::switchConfig(const QString& configName) {
  QString filePath;
  if (configName.isEmpty() || configName == "默认") {
    filePath = QCoreApplication::applicationDirPath() + "/config/config.json";
    m_currentConfigName = "";
  } else {
    filePath = QCoreApplication::applicationDirPath() + "/config/" + configName + ".json";
    m_currentConfigName = configName;
  }

  load(filePath);

  emit currentConfigNameChanged();
  emit baseUrlChanged();
  emit apiPathChanged();
  emit instanceIdsChanged();
  emit appIdChanged();
}

// ----------------------------------------------------------------------------
// 访问器
// ----------------------------------------------------------------------------
QString AppConfig::baseUrl() const { return m_baseUrl; }

QString AppConfig::apiPath() const { return m_apiPath; }

QString AppConfig::instanceIds() const { return m_instanceIds; }

QString AppConfig::appId() const { return m_appId; }

QStringList AppConfig::configNames() const { return m_configNames; }

QString AppConfig::currentConfigName() const { return m_currentConfigName; }
