#pragma once

#include <string>
#include <mutex>
#include <format>

#define DEBUG 1

struct Logger
{
  enum class Level
  {
      Trace,
      Info,
      Warn,
      Error,
      Critical
  };

  static void Init();
  static void SetLevel(Level level);

  template<typename... Args>
  static void Log(Level level, std::format_string<Args...> fmt, Args&&... args)
  {
#if DEBUG
      if (level < s_LogLevel)
          return;

      std::lock_guard<std::mutex> lock(s_Mutex);

      auto message = std::format(fmt, std::forward<Args>(args)...);
      Print(level, message);
#endif
  }

private:
  static void Print(Level level, const std::string& message);
  static const char* LevelToString(Level level);
  static const char* LevelColor(Level level);

private:
  static inline Level s_LogLevel = Level::Trace;
  static inline std::mutex s_Mutex;
};

#ifdef DEBUG
    #define GABGL_TRACE(...)    Logger::Log(Logger::Level::Trace, __VA_ARGS__)
    #define GABGL_INFO(...)     Logger::Log(Logger::Level::Info, __VA_ARGS__)
    #define GABGL_WARN(...)     Logger::Log(Logger::Level::Warn, __VA_ARGS__)
    #define GABGL_ERROR(...)    Logger::Log(Logger::Level::Error, __VA_ARGS__)
    #define GABGL_CRITICAL(...) Logger::Log(Logger::Level::Critical, __VA_ARGS__)
#else
    #define GABGL_TRACE(...)
    #define GABGL_INFO(...)
    #define GABGL_WARN(...)
    #define GABGL_ERROR(...)
    #define GABGL_CRITICAL(...)
#endif

#define GABGL_ASSERT(x, ...) \
{ \
    if(!(x)) \
    { \
        GABGL_ERROR("Assertion Failed: {}", std::format(__VA_ARGS__)); \
        __debugbreak(); \
    } \
}
