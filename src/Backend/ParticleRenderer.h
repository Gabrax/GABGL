#pragma once

#include "DeltaTime.hpp"
#include <cstdint>
#include <glm/glm.hpp>

struct ParticleRenderInstance
{
  glm::vec4 PositionAndSize;
  glm::vec4 Color;
  float Rotation = 0.0f;
  glm::vec4 RightAndStyle;
  glm::vec4 Up;
};

struct ParticleRenderer
{
  static void Init();
  static void Shutdown();
  static void Clear();
  static void UpdateAndRender(const DeltaTime& dt);
  static void Emit();
  static void EmitImpact(const glm::vec3& position, const glm::vec3& normal, uint32_t particleCount = 14);
};
