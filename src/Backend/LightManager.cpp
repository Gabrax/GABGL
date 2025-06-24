#include "LightManager.h"

#include "Buffer.h"
#include <vector>
#include <memory>
#include "BackendLogger.h"

struct LightData 
{
  glm::vec3 position;
  glm::vec3 rotation;
  glm::vec3 scale;
  glm::vec4 color;
  LightType type;
};

struct LightManagerData
{
  std::shared_ptr<StorageBuffer> LightPosStorageBuffer;
  std::shared_ptr<StorageBuffer> LightQuantityStorageBuffer;
  std::shared_ptr<StorageBuffer> LightColorStorageBuffer;
  std::shared_ptr<StorageBuffer> LightTypeStorageBuffer;

  std::vector<std::shared_ptr<LightData>> lights;

  int32_t numLights = 0;
  const uint32_t maxLights = 30;

} s_Data;

void LightManager::Init()
{
  s_Data.LightPosStorageBuffer = StorageBuffer::Create(sizeof(glm::vec3) * 10, 0);
  s_Data.LightQuantityStorageBuffer= StorageBuffer::Create(sizeof(uint32_t) * 10, 1);
  s_Data.LightColorStorageBuffer= StorageBuffer::Create(sizeof(glm::vec4) * 10, 2);
  s_Data.LightTypeStorageBuffer= StorageBuffer::Create(sizeof(uint32_t) * 10, 3);
}

void LightManager::AddLight(const LightType& type, const glm::vec4& color, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
{
  if (s_Data.numLights == s_Data.maxLights - 1) {
      GABGL_ERROR("Max number of lights reached!");
      return;
  }

  std::shared_ptr<LightData> lightData = std::make_shared<LightData>( position, rotation, scale, color, type );

  s_Data.lights.push_back(lightData);
  s_Data.numLights++;
  UpdateSSBOLightData();
}

void LightManager::EditLight(int32_t index, const std::optional<glm::vec4>& newColor, 
               const std::optional<glm::vec3>& newPosition,
               const std::optional<glm::vec3>& newRotation,    
               const std::optional<glm::vec3>& newScale)
{

  if (index < 0 || index >= s_Data.lights.size()) {
      GABGL_ERROR("Invalid light index: " + std::to_string(index));
      return;
  }

  std::shared_ptr<LightData>& lightData = s_Data.lights[index];

  if (newColor.has_value()) {
      lightData->color = newColor.value();
  }

  if (newPosition.has_value()) {
      lightData->position = newPosition.value();
  }

  if (newRotation.has_value()) {
      lightData->rotation = newRotation.value();
  }

  if (newScale.has_value()) {
      lightData->scale = newScale.value();
  }

  UpdateSSBOLightData();
}

void LightManager::RemoveLight(int32_t index)
{
  if (index >= 0 && index < s_Data.lights.size())
  {
      s_Data.lights.erase(s_Data.lights.begin() + index);
      s_Data.numLights--;
      UpdateSSBOLightData();
  }
}

void LightManager::UpdateSSBOLightData()
{
  s_Data.LightQuantityStorageBuffer->SetData(sizeof(int32_t), &s_Data.numLights);
  
  size_t alignedVec4Size = 16; // vec3 aligned to vec4 size (12 bytes data + 4 bytes padding)
  size_t bufferSizePositions = s_Data.numLights * alignedVec4Size;
  std::vector<int8_t> bufferPositions(bufferSizePositions);

  for (size_t i = 0; i < s_Data.lights.size(); ++i) {
      std::memcpy(bufferPositions.data() + (i * alignedVec4Size), &s_Data.lights[i]->position, sizeof(glm::vec3));
  }
  s_Data.LightPosStorageBuffer->SetData(bufferSizePositions, bufferPositions.data());

  size_t bufferSizeTypes = s_Data.numLights * sizeof(int32_t);
  std::vector<int32_t> bufferTypes(s_Data.numLights);

  for (size_t i = 0; i < s_Data.lights.size(); ++i) {
      bufferTypes[i] = static_cast<int32_t>(s_Data.lights[i]->type);
  }
  s_Data.LightTypeStorageBuffer->SetData(bufferSizeTypes, bufferTypes.data());

  size_t bufferSizeColors = s_Data.numLights * sizeof(glm::vec4);
  std::vector<int8_t> bufferColors(bufferSizeColors);
  for (size_t i = 0; i < s_Data.lights.size(); ++i) {
      std::memcpy(bufferColors.data() + (i * sizeof(glm::vec4)), &s_Data.lights[i]->color, sizeof(glm::vec4));
  }
  s_Data.LightColorStorageBuffer->SetData(bufferSizeColors, bufferColors.data());

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

