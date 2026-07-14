#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "core/BatchTaskOperator.h"

/**
 * @brief Thin adapter that dispatches dialog type + params to BatchTaskOperator methods.
 */
class BatchTaskOperatorModel {
public:
    explicit BatchTaskOperatorModel(BatchTaskOperator* batchTaskOperator);

    // Main dispatch: maps dialogType string to the correct BatchTaskOperator call
    void handleDialogSignal(const std::string& dialogType,
                            const std::vector<std::string>& instanceIds,
                            const std::map<std::string, std::string>& params = {});

    // Callback to show result dialog in the UI
    std::function<void(const std::string& title, const std::string& message)> onShowDialog;

private:
    BatchTaskOperator* m_batchTaskOperator;

    void onBatchTaskCompleted(const BatchTaskOperator::BatchResult& result);
    void onBatchTaskFailed(int errorCode, const std::string& errorMessage);
};
