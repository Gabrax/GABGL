#pragma once

#include <memory>
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"
#include "glad/glad.h"

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
#include <array>

constexpr uint32_t GABGL_MAX_QUERIES_PER_FRAME = 256;
constexpr uint32_t GABGL_QUERY_LATENCY = 3; // triple buffering

struct ProfileResult
{
  const char* Name;
  float CPUTime;
  float GPUTime;
};

struct GPUQueryData
{
  const char* Name;
  float CPUTime;
  GLuint QueryID;
};

static inline uint32_t s_CurrentFrame = 0;
static inline uint32_t s_QueryIndex = 0;

static inline std::array<
    std::array<GLuint, GABGL_MAX_QUERIES_PER_FRAME>,
    GABGL_QUERY_LATENCY
> s_QueryPool;

static inline std::array<
    std::vector<GPUQueryData>,
    GABGL_QUERY_LATENCY
> s_FrameQueries;

static inline std::vector<ProfileResult> s_Results;

struct Profiler
{
  Profiler(const char* name) : m_Name(name)
  {
    m_Start = std::chrono::high_resolution_clock::now();

    m_Query = this->AcquireQuery();
    if (m_Query) glBeginQuery(GL_TIME_ELAPSED, m_Query);
  }

  ~Profiler()
  {
    if (!m_Query) return;

    glEndQuery(GL_TIME_ELAPSED);

    auto end = std::chrono::high_resolution_clock::now();

    float cpuTime = std::chrono::duration<float, std::milli>(end - m_Start).count();

    this->Submit(m_Name, cpuTime, m_Query);
  }

  static void Init()
  {
    for (uint32_t i = 0; i < GABGL_QUERY_LATENCY; i++)
    {
        glGenQueries(GABGL_MAX_QUERIES_PER_FRAME, s_QueryPool[i].data());
    }
  }

  static void Shutdown()
  {
    for (uint32_t i = 0; i < GABGL_QUERY_LATENCY; i++)
    {
        glDeleteQueries(GABGL_MAX_QUERIES_PER_FRAME, s_QueryPool[i].data());
    }
  }

  static void BeginFrame()
  {
    s_Results.clear();

    s_CurrentFrame = (s_CurrentFrame + 1) % GABGL_QUERY_LATENCY;
    s_QueryIndex = 0;

    ResolveFrame((s_CurrentFrame + 1) % GABGL_QUERY_LATENCY);
  }

  static GLuint AcquireQuery()
  {
    if (s_QueryIndex >= GABGL_MAX_QUERIES_PER_FRAME)
        return 0;

    return s_QueryPool[s_CurrentFrame][s_QueryIndex++];
  }

  static void Submit(const char* name, float cpuTime, GLuint query)
  {
    s_FrameQueries[s_CurrentFrame].push_back({
        name, cpuTime, query
    });
  }

  static const std::vector<ProfileResult>& GetResults()
  {
    return s_Results;
  }

  static void ResolveFrame(uint32_t frameIndex)
  {
    auto& frameQueries = s_FrameQueries[frameIndex];

    for (auto& q : frameQueries)
    {
      GLuint64 timeElapsed = 0;
      glGetQueryObjectui64v(q.QueryID, GL_QUERY_RESULT, &timeElapsed);

      float gpuTime = timeElapsed / 1'000'000.0f;

      s_Results.push_back({q.Name, q.CPUTime, gpuTime});
    }

    frameQueries.clear();
  }

private:
  const char* m_Name;
  GLuint m_Query = 0;
  std::chrono::high_resolution_clock::time_point m_Start;
};

#define GABGL_RESOLVE_GPU_QUERIES() Profiler::BeginFrame()

#ifdef DEBUG
	#define GABGL_PROFILE_SCOPE(name) Profiler profiler##__LINE__(name)
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
