#pragma once

#include "Settings.h"

#include <algorithm>
#include <array>
#include <cstdint>

#include <glm/glm.hpp>

enum class LightType : uint32_t
{
  DIRECT = 0,
  POINT = 1,
  SPOT = 2
};

struct RenderLight
{
  LightType Type = LightType::POINT;
  glm::vec3 Color{1.0f};
  glm::vec3 Position{0.0f};
  glm::vec3 Direction{0.0f, -1.0f, 0.0f};
};

struct RenderBackendCapabilities
{
  bool NativePresentation = false;
  bool NativeSceneRenderer = false;
  bool NativeModelResources = false;
  bool NativeParticleRenderer = false;
  bool NativeUIRenderer = false;
  bool NativeDebugLines = false;
  bool NativePhysicsDebug = false;
  bool OpenGLContext = false;
  bool FramebufferOriginBottomLeft = false;
  bool PointLightShadows = false;
  bool TimestampProfiler = false;
};

struct RenderEffectSettings
{
  GraphicsQuality ShadowQuality = GraphicsQuality::Medium;
  GraphicsQuality BloomQuality = GraphicsQuality::Low;
  bool PS1Enabled = true;
  float PS1VirtualHeight = 240.0f;
  float PS1ColorLevels = 31.0f;
  float BloomExposure = 0.5f;
  float BloomStrength = 1.5f;
  float BloomFilterRadius = 0.005f;
  float BloomThreshold = 1.0f;
  float Gamma = 1.2f;

  [[nodiscard]] uint32_t DirectionalShadowResolution() const
  {
    switch (ShadowQuality)
    {
      case GraphicsQuality::High: return 4096;
      case GraphicsQuality::Medium: return 2048;
      case GraphicsQuality::Low: return 1024;
      case GraphicsQuality::Off:
      default: return 512;
    }
  }

  [[nodiscard]] uint32_t PointShadowResolution() const
  {
    switch (ShadowQuality)
    {
      case GraphicsQuality::High: return 1024;
      case GraphicsQuality::Medium: return 512;
      case GraphicsQuality::Low: return 256;
      case GraphicsQuality::Off:
      default: return 128;
    }
  }

  [[nodiscard]] uint32_t BloomPassCount() const
  {
    switch (BloomQuality)
    {
      case GraphicsQuality::High: return 6;
      case GraphicsQuality::Medium: return 5;
      case GraphicsQuality::Low: return 3;
      case GraphicsQuality::Off:
      default: return 0;
    }
  }
};

struct RenderDebugSettings
{
  bool Physics = false;
  bool Lights = false;
  bool CullingBounds = false;
  bool Debug2D = false;
};

struct RenderStatistics
{
  uint32_t VisibleInstances = 0;
  uint32_t RenderableInstances = 0;
};

struct WorldBoundingSphere
{
  glm::vec3 center{0.0f};
  float radius = 0.0f;
};

struct RenderFrustum
{
  explicit RenderFrustum(const glm::mat4& viewProjection)
  {
    const glm::mat4 transposed = glm::transpose(viewProjection);
    Planes[0] = transposed[3] + transposed[0];
    Planes[1] = transposed[3] - transposed[0];
    Planes[2] = transposed[3] + transposed[1];
    Planes[3] = transposed[3] - transposed[1];
    Planes[4] = transposed[3] + transposed[2];
    Planes[5] = transposed[3] - transposed[2];
    for (glm::vec4& plane : Planes)
    {
      const float length = glm::length(glm::vec3(plane));
      if (length > 0.000001f) plane /= length;
    }
  }

  [[nodiscard]] bool IntersectsSphere(const glm::vec3& center, float radius) const
  {
    for (const glm::vec4& plane : Planes)
      if (glm::dot(glm::vec3(plane), center) + plane.w < -radius) return false;
    return true;
  }

  std::array<glm::vec4, 6> Planes{};
};

template <typename ModelType>
WorldBoundingSphere CalculateWorldBoundingSphere(const ModelType& model, const glm::mat4& transform)
{
  const glm::vec3 center = glm::vec3(transform * glm::vec4(model.GetBoundsCenter(), 1.0f));
  const glm::vec3 scale(
    glm::length(glm::vec3(transform[0])),
    glm::length(glm::vec3(transform[1])),
    glm::length(glm::vec3(transform[2])));
  const float maxScale = std::max({scale.x, scale.y, scale.z});
  return {center, std::max(model.GetBoundsRadius(), 0.001f) * maxScale};
}
