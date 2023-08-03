#ifndef TCRSDK_ASYNC_CALLBACK_H_
#define TCRSDK_ASYNC_CALLBACK_H_

#include <string>
#include "tcr_code.h"

namespace tcrsdk {
    /**
     * @file async_callback.h
     *
     * @brief This interface defines a general async task callback schema.
     */
    class __declspec(dllexport) AsyncCallback {
    public:
        /**
         * @brief This is called when the async task is executed successfully.
         *
         * @param result The result of the task execution.
         */
        virtual void OnSuccess(const std::string& result) = 0;

        /**
         * @brief This is called when the async task is executed failed.
         *
         * @param code The error code of the task execution.
         * @param msg The error message of the task execution.
         */
        virtual void OnFailure(const TcrCode& code, const std::string& msg) = 0;
    };
}

#endif