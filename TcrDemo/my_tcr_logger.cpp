#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include "my_tcr_logger.h"
#include <iostream>
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/msvc_sink.h"

MyTcrLogger::MyTcrLogger()
{
	try {
		// 将日志输出到“logs/tcrlogs.log”文件中，每个文件限制大小为300MB，最多保留10个文件
	// 创建控制台输出目标
		auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		console_sink->set_level(spdlog::level::trace);

		// 创建输出调试器输出目标
		auto msvc_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
		msvc_sink->set_level(spdlog::level::trace);

		// 创建日志文件输出目标
		auto max_size = 1024 * 1024 * 300;
		auto max_num = 10;
		auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/tcrlogs.log", max_size, max_num);
		file_sink->set_level(spdlog::level::trace);

		logger_ = std::make_shared<spdlog::logger>("tcr_logger", spdlog::sinks_init_list{ console_sink, msvc_sink, file_sink });

		// 打印时间、线程id、日志级别、[TCR] 内容
		logger_->set_pattern("%Y-%m-%d %H:%M:%S.%e  tid:%t  %^%L%$  [TCRDEMO]%v");

		//当遇到WARNING级别以上的马上刷新到日志
		logger_->flush_on(spdlog::level::level_enum::warn);

		logger_->set_level(spdlog::level::trace);

		//每三秒刷新一次
		spdlog::flush_every(std::chrono::seconds(3));

		spdlog::register_logger(logger_);
	}
	catch (const spdlog::spdlog_ex& ex) {
		std::cout << "Log init failed: " << ex.what() << std::endl;
		return;
	}

}

MyTcrLogger::~MyTcrLogger()
{

}

void MyTcrLogger::t(const char* tag, const char* log)
{
	if (logger_ != nullptr) {
		logger_->trace("{}   {}", tag, log);
	}
}

void MyTcrLogger::d(const char* tag, const char* log)
{
	if (logger_ != nullptr) {
		logger_->debug("{}   {}", tag, log);
	}
}

void MyTcrLogger::i(const char* tag, const char* log)
{
	if (logger_ != nullptr) {
		logger_->info("{}   {}", tag, log);
	}
}

void MyTcrLogger::w(const char* tag, const char* log)
{
	if (logger_ != nullptr) {
		logger_->warn("{}   {}", tag, log);
	}
}

void MyTcrLogger::e(const char* tag, const char* log)
{
	if (logger_ != nullptr) {
		logger_->error("{}   {}", tag, log);
	}
}


