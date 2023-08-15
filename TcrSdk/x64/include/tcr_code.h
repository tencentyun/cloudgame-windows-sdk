#ifndef TCRSDK_TCR_CODE_H_
#define TCRSDK_TCR_CODE_H_

#include <stdint.h>

namespace tcrsdk {
    /**
     * @file tcr_code.h
     * 
     * @brief This interface defines common codes of TcrSdk.
     */
	enum class __declspec(dllexport) TcrCode: int32_t {

    ////////////////////////   Common status code   ////////////////////////
    /**
     * @brief This indicates that the result is success.
     */
    SUCCESS = 0,

    /**
     * @brief This indicates that the request is timed out.
     */
    ERR_TIMEOUT = 100010,

    /**
     * @brief This indicates that the request parameter is invalid.
     */
    ERR_INVALID_PARAMS = 100011,

    /**
     * @brief This indicates that some unknown error has happened.
     */
    ERR_UNKNOWN = 100012,

    /**
     * @brief This indicates that some internal error has happened.
     */
    ERR_INTERNAL_ERROR = 100013,

    /**
     * @brief This indicates that the context state is illegal.
     */
    ERR_STATE_ILLEGAL = 100014,

    ////////////////////////   DataChannel status code   ////////////////////////
    /**
     * @brief The starting error code of the data channel module.
     */
    ERR_DATA_CHANNEL_BASE_CODE = 102000,
    /**
     * @brief This indicates that the data channel is created failed.
     */
    ERR_CREATE_FAILURE = ERR_DATA_CHANNEL_BASE_CODE + 1,
    /**
     * @brief This indicates that the data channel has been closed.
     */
    ERR_CLOSED = ERR_DATA_CHANNEL_BASE_CODE + 2

};
}

#endif
