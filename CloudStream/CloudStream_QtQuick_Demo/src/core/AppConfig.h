#pragma once

#include <QMutex>
#include <QObject>
#include <QString>

/**
 * @brief 应用配置单例
 *
 * 从可执行文件旁边的 config.json 读取配置项。
 * 若文件不存在或字段缺失，保留内置默认值。
 */
class AppConfig : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString instanceIds READ instanceIds CONSTANT)

 public:
  static AppConfig* instance();

  QString baseUrl() const;
  QString apiPath() const;
  QString instanceIds() const;

 private:
  explicit AppConfig(QObject* parent = nullptr);
  ~AppConfig() override = default;

  AppConfig(const AppConfig&) = delete;
  AppConfig& operator=(const AppConfig&) = delete;

  void load();

  static AppConfig* s_instance;
  static QMutex s_mutex;

  QString m_baseUrl = "https://test-accelerator-biz-server.cai.crtrcloud.com";
  QString m_apiPath = "/CreateAndroidInstancesAccessToken";
  QString m_instanceIds = "";
};
