#pragma once

#include <random>

struct RandomGen
{
  static void Init()
  {
      m_Engine.seed(std::random_device{}());
  }

  static float Float()
  {
      return m_Dist(m_Engine); // [0.0f, 1.0f)
  }

  static float RandomRange(float min, float max)
  {
      return Float() * (max - min) + min;
  }

private:
  inline static std::mt19937 m_Engine;
  inline static std::uniform_real_distribution<float> m_Dist{ 0.0f, 1.0f };
};

