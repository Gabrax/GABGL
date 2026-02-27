#include "Profiler.h"

#include <array>

constexpr uint32_t GABGL_MAX_QUERIES_PER_FRAME = 256;
constexpr uint32_t GABGL_QUERY_LATENCY = 3; // triple buffering

struct GPUQueryData
{
  const char* Name;
  float CPUTime;
  GLuint QueryID;
};

static uint32_t s_CurrentFrame = 0;
static uint32_t s_QueryIndex = 0;

static std::array<
    std::array<GLuint, GABGL_MAX_QUERIES_PER_FRAME>,
    GABGL_QUERY_LATENCY
> s_QueryPool;

static std::array<
    std::vector<GPUQueryData>,
    GABGL_QUERY_LATENCY
> s_FrameQueries;

static std::vector<ProfileResult> s_Results;

Profiler::Profiler(const char* name) : m_Name(name)
{
  m_Start = std::chrono::high_resolution_clock::now();

  m_Query = this->AcquireQuery();
  if (m_Query) glBeginQuery(GL_TIME_ELAPSED, m_Query);
}

Profiler::~Profiler()
{
  if (!m_Query) return;

  glEndQuery(GL_TIME_ELAPSED);

  auto end = std::chrono::high_resolution_clock::now();

  float cpuTime = std::chrono::duration<float, std::milli>(end - m_Start).count();

  this->Submit(m_Name, cpuTime, m_Query);
}

void Profiler::Init()
{
  for (uint32_t i = 0; i < GABGL_QUERY_LATENCY; i++)
  {
      glGenQueries(GABGL_MAX_QUERIES_PER_FRAME, s_QueryPool[i].data());
  }
}

void Profiler::Shutdown()
{
  for (uint32_t i = 0; i < GABGL_QUERY_LATENCY; i++)
  {
      glDeleteQueries(GABGL_MAX_QUERIES_PER_FRAME, s_QueryPool[i].data());
  }
}

void Profiler::BeginFrame()
{
  s_Results.clear();

  s_CurrentFrame = (s_CurrentFrame + 1) % GABGL_QUERY_LATENCY;
  s_QueryIndex = 0;

  ResolveFrame((s_CurrentFrame + 1) % GABGL_QUERY_LATENCY);
}

GLuint Profiler::AcquireQuery()
{
  if (s_QueryIndex >= GABGL_MAX_QUERIES_PER_FRAME)
      return 0;

  return s_QueryPool[s_CurrentFrame][s_QueryIndex++];
}

void Profiler::Submit(const char* name, float cpuTime, GLuint query)
{
  s_FrameQueries[s_CurrentFrame].push_back({
      name, cpuTime, query
  });
}

const std::vector<ProfileResult>& Profiler::GetResults()
{
  return s_Results;
}

void Profiler::ResolveFrame(uint32_t frameIndex)
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
