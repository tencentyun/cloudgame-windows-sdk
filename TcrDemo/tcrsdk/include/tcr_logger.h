#ifndef TCR_LOGGER_H_
#define TCR_LOGGER_H_

namespace tcrsdk {
	/**
	 * This interface represents the log callbacks of the TcrSdk.
	 *
	 * @see LogUtils#SetLogger(TcrLogger)
	 */
	class __declspec(dllexport) TcrLogger
	{
	public:
		virtual void t(const char* tag, const char* log) = 0;  // trace
		virtual void d(const char* tag, const char* log) = 0;  // debug
		virtual void i(const char* tag, const char* log) = 0;  // info
		virtual void w(const char* tag, const char* log) = 0;  // warn
		virtual void e(const char* tag, const char* log) = 0;  // error
	};
}

#endif

