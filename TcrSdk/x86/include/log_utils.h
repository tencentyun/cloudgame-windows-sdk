#ifndef LOG_UTILS_H_
#define LOG_UTILS_H_

#include "tcr_logger.h"
#include <memory>
namespace tcrsdk {
	/**
	 * @file log_utils.h
	 * 
	 * @brief This class can be used to print logs created by TcrSdk.
	 */
	class __declspec(dllexport) LogUtils
	{
	public:
		/**
		 * @brief Set a logger for the TcrSdk.
		 *
		 * By default, the TcrSdk will print logs to a temporary file on the sdcard as well as the android device logs.
		 * If you set a custom logger via this method, the TcrSdk will then only callback logs to your
		 * logger.
		 *
		 * This method can be called before {@see TcrSession(TcrSession::Observer).
		 *
		 * @param logger the TcrLogger object to be set. It should not be null.
		 */
		static void SetLogger(std::shared_ptr<TcrLogger>logger);

		/*
		 * @brief Print trace level log
		 * 
		 * NOTE: Print only in 'Debug' run mode
		 */
		static void t(const char* tag, const char* log);  // trace
		/*
		 * @brief Print debug level log
		 * 
		 * NOTE: Print only in 'Debug' run mode
		 */
		static void d(const char* tag, const char* log);  // debug
		/*
		 * @brief Print info level log
		 */
		static void i(const char* tag, const char* log);  // info
		/*
		 * @brief Print warn level log
		 */
		static void w(const char* tag, const char* log);  // warn
		/*
		 * @brief Print error level log
		 */
		static void e(const char* tag, const char* log);  // error

	private:
		LogUtils() = delete;
		~LogUtils() = delete;
		static std::weak_ptr<TcrLogger>logger_;
	};
}

#endif // !LOG_UTILS_H_



