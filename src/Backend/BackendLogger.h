#pragma once

#include <memory>
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h> 
#pragma warning(pop)

#define DEBUG 1

struct Logger
{
	static void Init();
	inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
private:
	static std::shared_ptr<spdlog::logger> s_CoreLogger;
};

template<typename OStream, glm::length_t L, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::vec<L, T, Q>& vector)
{
	return os << glm::to_string(vector);
}

template<typename OStream, glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::mat<C, R, T, Q>& matrix)
{
	return os << glm::to_string(matrix);
}

template<typename OStream, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, glm::qua<T, Q> quaternion)
{
	return os << glm::to_string(quaternion);
}

// Core log macros
#ifdef DEBUG
	#define GABGL_TRACE(...)    ::Logger::GetCoreLogger()->trace(__VA_ARGS__)
	#define GABGL_INFO(...)     ::Logger::GetCoreLogger()->info(__VA_ARGS__)
	#define GABGL_WARN(...)     ::Logger::GetCoreLogger()->warn(__VA_ARGS__)
	#define GABGL_ERROR(...)    ::Logger::GetCoreLogger()->error(__VA_ARGS__)
	#define GABGL_CRITICAL(...) ::Logger::GetCoreLogger()->critical(__VA_ARGS__)
#else
	#define GABGL_TRACE(...)    
	#define GABGL_INFO(...)     
	#define GABGL_WARN(...)     
	#define GABGL_ERROR(...)    
	#define GABGL_CRITICAL(...) 
#endif

#define GABGL_ASSERT(x,...) { if(!(x)) { GABGL_ERROR("Assertion Failed: {0}",__VA_ARGS__); __debugbreak(); } }
/*#define GABGL_ASSERT(x) { if(!(x)) { GABGL_ERROR("Assertion Failed"); __debugbreak(); } }*/

#define BIND_EVENT(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

#include <chrono>
#include <vector>

struct ProfileResult
{
	const char* Name;
	float Time;
};

template<typename Func>
struct Profiler
{
	inline Profiler(const char* name, Func&& func) : m_Name(name), m_Stop(false), m_Func(func)
	{
		m_Start = std::chrono::high_resolution_clock::now();
	}

	inline ~Profiler()
	{
		if (!m_Stop) Stop();
	}

	inline void Stop()
	{
		auto endPoint = std::chrono::high_resolution_clock::now();

		long long start = std::chrono::time_point_cast<std::chrono::milliseconds>(m_Start).time_since_epoch().count();
		long long end = std::chrono::time_point_cast<std::chrono::milliseconds>(endPoint).time_since_epoch().count();

		m_Stop = true;
		float duration = (end - start) * 0.001f;
		m_Func({ m_Name, duration });
	}

private:
	Func m_Func;
	bool m_Stop;
	const char* m_Name;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
};

inline std::vector<ProfileResult> s_ProfileResults;

#ifdef DEBUG
	#define GABGL_PROFILE_SCOPE(name) Profiler profiler##__LINE__(name, [&](ProfileResult pr){ s_ProfileResults.push_back(pr);})
#else
	#define GABGL_PROFILE_SCOPE(name)
#endif

struct Timer
{
	Timer()
	{
		Reset();
	}

	void Reset()
	{
		m_Start = std::chrono::high_resolution_clock::now();
	}

	float Elapsed()
	{
		return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count() * 0.001f * 0.001f * 0.001f;
	}

	float ElapsedMillis()
	{
		return Elapsed() * 1000.0f;
	}

private:
	std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
};
