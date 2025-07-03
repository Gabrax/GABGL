#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <optional>
#include <vector>

enum class LightType : int32_t
{
  POINT = 0,
  DIRECT = 1,
  SPOT = 2
};

struct LightManager
{
  static void Init();
  static void AddLight(const LightType& type, const glm::vec4& color, const glm::vec3& position, const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f));
  static void EditLight(int32_t index, const std::optional<glm::vec4>& newColor = std::nullopt, 
             const std::optional<glm::vec3>& newPosition = std::nullopt,
             const std::optional<glm::vec3>& newRotation = std::nullopt,    
             const std::optional<glm::vec3>& newScale = std::nullopt);
  static void RemoveLight(int32_t index);
  static int32_t GetLightsQuantity();
  static int32_t GetPointLightsQuantity();
  static std::vector<glm::vec3> GetPointLightPositions();

private:
  static void UpdateSSBOLightData();
  static void ResizeLightBuffers(uint32_t newMax);
};
