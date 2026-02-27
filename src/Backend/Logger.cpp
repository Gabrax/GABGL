#include "Logger.h"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>

void Logger::Init()
{
  s_LogLevel = Level::Trace;
}

void Logger::SetLevel(Level level)
{
  s_LogLevel = level;
}

const char* Logger::LevelToString(Level level)
{
  switch (level)
  {
    case Level::Trace:    return "TRACE";
    case Level::Info:     return "INFO ";
    case Level::Warn:     return "WARN ";
    case Level::Error:    return "ERROR";
    case Level::Critical: return "CRIT ";
  }
  return "";
}

const char* Logger::LevelColor(Level level)
{
  switch (level)
  {
    case Level::Trace:    return "\033[37m"; // white
    case Level::Info:     return "\033[32m"; // green
    case Level::Warn:     return "\033[33m"; // yellow
    case Level::Error:    return "\033[31m"; // red
    case Level::Critical: return "\033[41m"; // red background
  }
  return "\033[0m";
}

void Logger::Print(Level level, const std::string& message)
{
  std::cout
      << LevelColor(level)
      << "[" << LevelToString(level) << "] "
      << message
      << "\033[0m"
      << std::endl;
}
