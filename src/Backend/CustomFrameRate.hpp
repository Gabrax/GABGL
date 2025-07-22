#pragma once

#include <chrono>
#include <thread>

struct CustomFrameRate
{
  using Clock = std::chrono::high_resolution_clock;
  using TimePoint = std::chrono::time_point<Clock>;

  inline static void SetTargetFPS(float fps)
  {
    targetFPS = fps;
    frameDelay = 1000.0f / targetFPS;
  }

  inline static void BeginFrame()
  { 
    frameStart = Clock::now();
  }

  inline static void EndFrame()
  {
    auto frameEnd = Clock::now();
    std::chrono::duration<float, std::milli> elapsed = frameEnd - frameStart;

    if (elapsed.count() < frameDelay)
    {
      std::this_thread::sleep_for(std::chrono::duration<float, std::milli>(frameDelay - elapsed.count()));
    }
  }

private:
  inline static TimePoint frameStart;
  inline static float targetFPS = 60.0f;
  inline static float frameDelay = 1000.0f / 60.0f;
};

