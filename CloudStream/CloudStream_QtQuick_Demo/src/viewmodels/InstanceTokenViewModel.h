#pragma once

#include <QObject>
#include <QStringList>
#include "../services/ApiService.h"

/**
 * @brief 实例AccesToken视图模型
 * 
 * 负责处理云手机实例访问令牌的创建流程，作为UI层和业务逻辑层之间的桥梁。
 * 
 * @note 在生产环境中，实例ID通常从服务端获取。
 *       本演示工程中通过QML界面手动输入实例ID。
 * 
 * 主要功能：
 * - 接收用户输入的实例ID列表和用户IP
 * - 调用ApiService创建访问令牌
 * - 向UI层发送结果通知（成功/失败）
 */
class InstanceTokenViewModel : public QObject {
    Q_OBJECT
    
    // Qt属性：用于QML绑定
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY isBusyChanged)
    
public:
    /**
     * @brief 构造函数
     * @param apiService API服务实例指针（依赖注入）
     * @param parent 父对象指针
     */
    explicit InstanceTokenViewModel(ApiService* apiService, QObject *parent = nullptr);
    
    /**
     * @brief 创建实例访问令牌
     * 
     * 解析并验证输入参数，然后调用ApiService创建访问令牌。
     * 
     * @param instanceIds 实例ID列表（逗号分隔的字符串）
     * @param userIp 用户IP地址（可选，用于模拟用户位置）
     * 
     * @note 此方法可从QML直接调用（Q_INVOKABLE）
     * @note 如果当前正在处理请求（isBusy=true），则忽略新请求
     */
    Q_INVOKABLE void createAccessToken(const QString& instanceIds, const QString& userIp);
    
    /**
     * @brief 获取忙碌状态
     * @return true表示正在处理请求，false表示空闲
     */
    bool isBusy() const { return m_isBusy; }
    
signals:
    /**
     * @brief 访问令牌创建成功信号
     * @param accessInfo 访问信息（包含连接地址等）
     * @param token 访问令牌字符串
     */
    void accessTokenCreated(const QString& accessInfo, const QString& token);
    
    /**
     * @brief 错误发生信号
     * @param errorMessage 错误描述信息
     */
    void errorOccurred(const QString& errorMessage);
    
    /**
     * @brief 忙碌状态变化信号
     * 
     * 当isBusy属性值改变时发出，用于通知QML更新UI状态。
     */
    void isBusyChanged();
    
private slots:
    /**
     * @brief 处理访问令牌创建成功
     * 
     * 响应ApiService的androidInstancesAccessTokenCreated信号。
     * 
     * @param accessInfo 访问信息
     * @param token 访问令牌
     */
    void onAccessTokenCreated(const QString& accessInfo, const QString& token);
    
    /**
     * @brief 处理API错误
     * 
     * 响应ApiService的apiError信号。
     * 
     * @param errorCode 错误代码
     * @param message 错误描述
     */
    void onApiError(const QString& errorCode, const QString& message);
    
private:
    ApiService* m_apiService;  ///< API服务实例（不拥有所有权）
    bool m_isBusy = false;     ///< 请求处理状态标志
};