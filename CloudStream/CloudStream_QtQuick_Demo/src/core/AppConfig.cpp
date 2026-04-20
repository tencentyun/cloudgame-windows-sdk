#include "AppConfig.h"

#include <QCoreApplication>
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

AppConfig::AppConfig(QObject* parent) : QObject(parent) { load(); }

void AppConfig::load() {
  QString configPath = QCoreApplication::applicationDirPath() + "/config.json";
  QFile file(configPath);
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
}

QString AppConfig::baseUrl() const { return m_baseUrl; }

QString AppConfig::apiPath() const { return m_apiPath; }

QString AppConfig::instanceIds() const { return m_instanceIds; }
