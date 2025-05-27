#pragma once

struct DeltaTime
{
    DeltaTime();

    operator float() const { return m_Time; }

    float GetSeconds() const { return m_Time; }
    float GetMilliseconds() const { return m_Time * 1000.0f; }

private:
    float m_Time;
    static float s_LastFrameTime; // Static variable to track the last frame time
};

