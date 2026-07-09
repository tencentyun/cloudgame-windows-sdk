#include "AndroidInstanceModel.h"

#include "services/ApiService.h"
#include "utils/Logger.h"
#include "viewmodels/MultiStreamViewModel.h"

// 静态成员初始化
InstanceImageProvider* AndroidInstanceModel::s_imageProvider = new InstanceImageProvider();

AndroidInstanceModel::AndroidInstanceModel(ApiService* apiService, BatchTaskOperator* op, QObject* parent)
    : QObject(parent), m_apiService(apiService) {
  // 连接信号
  connect(m_apiService, &ApiService::loginSuccess, this, &AndroidInstanceModel::onLoginSuccess);
  connect(m_apiService, &ApiService::instancesReceived, this, &AndroidInstanceModel::onInstancesReceived);
}

AndroidInstanceModel::~AndroidInstanceModel() {
  if (m_multiStreamViewModel) {
    m_multiStreamViewModel->closeSession();
  }
}

void AndroidInstanceModel::setMultiStreamViewModel(MultiStreamViewModel* viewModel) {
  m_multiStreamViewModel = viewModel;
}

QVariantList AndroidInstanceModel::instances() const {
  QVariantList list;
  for (const auto& instance : m_instances) {
    QVariantMap map;
    map["AndroidInstanceId"] = instance.AndroidInstanceId;
    map["AndroidInstanceRegion"] = instance.AndroidInstanceRegion;
    map["State"] = instance.State;
    list.append(map);
  }
  return list;
}

void AndroidInstanceModel::onLoginSuccess(const QString& userType) {
  m_apiService->describeAndroidInstances(
      0, 200, {"cai-251197962-fe2d8imcyfh", "cai-251197962-fe2df5kvpil", "cai-251197962-fe2dhv9ztl8"});
}

void AndroidInstanceModel::onInstancesReceived(const QList<AndroidInstance>& instances, int totalCount) {
  m_instances = instances;

  // 1. 提取所有实例ID
  QStringList instanceIds;
  for (const auto& instance : m_instances) {
    instanceIds.append(instance.AndroidInstanceId);
  }

  // 2. 调用创建安卓实例令牌API
  m_apiService->createAndroidInstancesAccessToken(instanceIds);

  // 3. 连接令牌创建成功信号（注意：避免重复连接，使用disconnect断开之前的连接）
  disconnect(m_apiService, &ApiService::androidInstancesAccessTokenCreated, this, nullptr);
  connect(m_apiService, &ApiService::androidInstancesAccessTokenCreated, this,
          [this, instanceIds](const QString& accessInfo, const QString& token) {
            // 4. 初始化TCR SDK
            std::string tokenStr = token.toStdString();
            std::string accessInfoStr = accessInfo.toStdString();

            TcrConfig config = {tokenStr.c_str(), accessInfoStr.c_str()};
            TcrClientHandle client = tcr_client_get_instance();
            TcrErrorCode result = tcr_client_init(client, &config);
            if (result != TCR_SUCCESS) {
              Logger::error(QString("Init TCR SDK failed"));
              return;
            }

            // 5. 通过MultiStreamViewModel创建多实例视频流会话
            if (m_multiStreamViewModel) {
              m_multiStreamViewModel->initialize(instanceIds, accessInfo, token);
              // 使用全部实例数量作为并发拉流数（后续可通过可见性动态切换）
              m_multiStreamViewModel->connectMultipleInstances(instanceIds, instanceIds.size());
            }

            emit instancesChanged();  // 通知QML刷新视图
          });
}

void AndroidInstanceModel::refreshInstances() {
  Logger::debug("AndroidInstanceModel::refreshInstances");

  // 1. 关闭当前多实例流会话
  if (m_multiStreamViewModel) {
    m_multiStreamViewModel->closeSession();
  }

  // 2. 清空本地实例列表
  m_instances.clear();
  emit instancesChanged();

  // 3. 重新拉取实例列表
  m_apiService->describeAndroidInstances(0, 200);
}

InstanceImageProvider* AndroidInstanceModel::imageProvider() { return s_imageProvider; }
