#ifndef MY_TCR_LOGGER_H_
#define MY_TCR_LOGGER_H_

#include "tcr_logger.h"
#include <memory>
#include "spdlog/spdlog.h"

class MyTcrLogger : public tcrsdk::TcrLogger
{
public:
	MyTcrLogger();
	~MyTcrLogger();
	// Í¨¹ý TcrLogger ¼Ì³Ð
	virtual void t(const char* tag, const char* log) override;
	virtual void d(const char* tag, const char* log) override;
	virtual void i(const char* tag, const char* log) override;
	virtual void w(const char* tag, const char* log) override;
	virtual void e(const char* tag, const char* log) override;

private:
	std::shared_ptr<spdlog::logger>logger_;
};

#endif // !MY_TCR_LOGGER_H_



