#pragma once

#include <QMutex>
#include <QObject>
#include <QString>
#include <QStringList>

/**
 * @brief 应用配置单例
 *
 * 从可执行文件旁边的 config.json 读取配置项。
 * 若文件不存在或字段缺失，保留内置默认值。
 *
 * 支持从 config/ 子目录扫描多个配置文件，运行时通过
 * switchConfig() 动态切换，所有属性带 NOTIFY 信号供 QML 绑定。
 */
class AppConfig : public QObject {
  Q_OBJECT

  Q_PROPERTY(QString instanceIds READ instanceIds NOTIFY instanceIdsChanged)
  Q_PROPERTY(QStringList configNames READ configNames NOTIFY configNamesChanged)
  Q_PROPERTY(QString currentConfigName READ currentConfigName NOTIFY currentConfigNameChanged)

 public:
  static AppConfig* instance();

  QString baseUrl() const;
  QString apiPath() const;
  QString instanceIds() const;

  /** 扫描到的配置名列表（不含后缀），不含"默认"条目 */
  QStringList configNames() const;

  /** 当前激活的配置名；若使用默认 config.json 则为空字符串 */
  QString currentConfigName() const;

  /**
   * @brief 切换配置
   * @param configName 配置名（不含后缀）；传空字符串或 "默认" 表示恢复默认
   */
  Q_INVOKABLE void switchConfig(const QString& configName);

 signals:
  void baseUrlChanged();
  void apiPathChanged();
  void instanceIdsChanged();
  void configNamesChanged();
  void currentConfigNameChanged();

 private:
  explicit AppConfig(QObject* parent = nullptr);
  ~AppConfig() override = default;

  AppConfig(const AppConfig&) = delete;
  AppConfig& operator=(const AppConfig&) = delete;

  /** 从指定路径加载配置文件，字段缺失时保留当前值 */
  void load(const QString& filePath);

  /** 扫描 <appDir>/config/ 目录，填充 m_configNames */
  void scanConfigs();

  static AppConfig* s_instance;
  static QMutex s_mutex;

  QString m_baseUrl = "https://test-accelerator-biz-server.cai.crtrcloud.com";
  QString m_apiPath = "/CreateAndroidInstancesAccessToken";
  QString m_instanceIds = "";

  QStringList m_configNames;
  QString m_currentConfigName = "";  // 空字符串表示使用默认 config.json
};
