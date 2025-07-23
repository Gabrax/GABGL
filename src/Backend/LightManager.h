#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <optional>
#include <vector>

enum class LightType : int32_t
{
  DIRECT = 0,
  POINT = 1,
  SPOT = 2
};

struct LightManager
{
  static void Init();
  static void AddLight(const LightType& type, const glm::vec3& color, const glm::vec3& position, const glm::vec3& rotation = glm::vec3(0.0f));
  static void EditLight(int32_t index, const std::optional<glm::vec3>& newColor = std::nullopt, 
             const std::optional<glm::vec3>& newPosition = std::nullopt,
             const std::optional<glm::vec3>& newRotation = std::nullopt);
  static void RemoveLight(int32_t index);
  static int32_t GetLightsQuantity();
  static int32_t GetPointLightsQuantity();
  static std::vector<glm::vec3> GetPointLightPositions();
  static glm::vec3 GetDirectLightRotation();
  static bool DirectLightEmpty();
  static bool PointLightEmpty();

private:
  static void UpdateSSBOLightData();
  static void ResizeLightBuffers(uint32_t newMax);
};
