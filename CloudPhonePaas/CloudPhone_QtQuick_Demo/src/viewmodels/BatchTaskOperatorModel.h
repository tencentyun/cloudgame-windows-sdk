#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include "core/BatchTaskOperator.h"
#include "utils/Logger.h"

/**
 * @brief 批量任务操作模型，封装批量操作相关逻辑
 *
 * 该模型负责与BatchTaskOperator交互，处理批量任务相关的信号和槽，供QML界面调用。
 */
class BatchTaskOperatorModel : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param batchTaskOperator 批量任务操作器指针
     * @param parent 父对象
     */
    explicit BatchTaskOperatorModel(BatchTaskOperator* batchTaskOperator, QObject *parent = nullptr);

    /**
     * @brief 处理QML对话框信号
     * @param dialogType 对话框类型
     * @param instanceIds 实例ID列表
     * @param params 额外参数
     */
    Q_INVOKABLE void handleDialogSignal(const QString& dialogType, const QStringList& instanceIds, const QVariantMap& params = QVariantMap());

signals:
    /**
     * @brief 通知QML显示对话框
     * @param title 标题
     * @param message 消息内容
     */
    void showDialog(const QString &title, const QString &message);

private slots:
    /**
     * @brief 批量任务完成时回调
     * @param result 批量任务结果
     */
    void onBatchTaskCompleted(const BatchTaskOperator::BatchResult &result);

    /**
     * @brief 批量任务失败时回调
     * @param errorCode 错误码
     * @param errorMessage 错误信息
     */
    void onBatchTaskFailed(int errorCode, const QString &errorMessage);

private:
    BatchTaskOperator* m_batchTaskOperator; ///< 批量任务操作器指针
};
