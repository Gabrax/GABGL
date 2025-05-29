#pragma once
#include <cstdint>

struct DeltaTime
{
    DeltaTime();

    operator float() const { return m_Time; }

    float GetSeconds() const { return m_Time; }
    float GetMilliseconds() const { return m_Time * 1000.0f; }
    uint32_t GetFPS() const { return m_Time > 0.0f ? static_cast<uint32_t>(1.0f / m_Time) : 0; }
private:
    float m_Time;
    static float s_LastFrameTime; // Static variable to track the last frame time
};

