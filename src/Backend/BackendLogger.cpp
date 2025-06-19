#include "BackendLogger.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>


std::shared_ptr<spdlog::logger> Logger::s_CoreLogger;

void Logger::Init()
{
	std::vector<spdlog::sink_ptr> logSinks;
	logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("GABGL.log", true));

	logSinks[0]->set_pattern("%^[%T] %n: %v%$");
	logSinks[1]->set_pattern("[%T] [%l] %n: %v");

	s_CoreLogger = std::make_shared<spdlog::logger>("GABGL", begin(logSinks), end(logSinks));
	spdlog::register_logger(s_CoreLogger);
	s_CoreLogger->set_level(spdlog::level::trace);
	s_CoreLogger->flush_on(spdlog::level::trace);
}
