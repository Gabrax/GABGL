#pragma once
#include <cstdint>
#include <GLFW/glfw3.h>

struct DeltaTime
{
  DeltaTime()
  {
    float currentTime = static_cast<float>(glfwGetTime()); 
    m_Time = currentTime - m_LastFrameTime;                
    m_LastFrameTime = currentTime;                         
  }

  inline operator float() const { return m_Time; }

  inline float GetSeconds() const { return m_Time; }
  inline float GetMilliseconds() const { return m_Time * 1000.0f; }
  inline uint32_t GetFPS() const { return m_Time > 0.0f ? static_cast<uint32_t>(1.0f / m_Time) : 0; }

private:
  float m_Time;
  inline static float m_LastFrameTime = 0.0f; // Static variable to track the last frame time
};

