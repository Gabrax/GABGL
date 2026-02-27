#pragma once

#include <chrono>
#include <vector>
#include "glad/glad.h"

struct ProfileResult
{
  const char* Name;
  float CPUTime;
  float GPUTime;
};

struct Profiler
{
  Profiler(const char* name);
  ~Profiler();

  static void Init();
  static void Shutdown();
  static void BeginFrame();
  static GLuint AcquireQuery();
  static void Submit(const char* name, float cpuTime, GLuint query);
  static const std::vector<ProfileResult>& GetResults();
  static void ResolveFrame(uint32_t frameIndex);

private:
  const char* m_Name;
  GLuint m_Query = 0;
  std::chrono::high_resolution_clock::time_point m_Start;
};

#define GABGL_RESOLVE_GPU_QUERIES() Profiler::BeginFrame()
#define GABGL_PROFILE_SCOPE(name) Profiler profiler##__LINE__(name)

