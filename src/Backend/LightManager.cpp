#include "LightManager.h"

#include "Buffer.h"
#include <vector>
#include <memory>
#include "BackendLogger.h"

struct LightData 
{
  glm::vec3 position;
  glm::vec3 rotation;
  glm::vec4 color;
  LightType type;
};

struct LightManagerData
{
  std::shared_ptr<StorageBuffer> LightPosStorageBuffer;
  std::shared_ptr<StorageBuffer> LightRotationStorageBuffer;
  std::shared_ptr<StorageBuffer> LightQuantityStorageBuffer;
  std::shared_ptr<StorageBuffer> LightColorStorageBuffer;
  std::shared_ptr<StorageBuffer> LightTypeStorageBuffer;

  std::vector<std::shared_ptr<LightData>> lights;

  int32_t numLights = 0;
  int32_t numPointLights = 0;
  int32_t numDirectLights = 0;
  uint32_t maxLights = 30;
  uint32_t maxPointLights = maxLights - 10;

} s_Data;

void LightManager::Init()
{
  ResizeLightBuffers(s_Data.maxLights);
}

void LightManager::AddLight(const LightType& type, const glm::vec3& color, const glm::vec3& position, const glm::vec3& rotation)
{
  if (type == LightType::DIRECT && s_Data.numDirectLights == 1)
  {
    GABGL_ERROR("Only one directional light allowed!");
    return;
  }
  if (s_Data.numLights >= s_Data.maxLights) ResizeLightBuffers(s_Data.maxLights * 2);

  glm::vec4 newColor = glm::vec4(color.x,color.y,color.z,1.0);

  std::shared_ptr<LightData> lightData = std::make_shared<LightData>(position, rotation, newColor, type);
  s_Data.lights.push_back(lightData);

  if (type == LightType::POINT) s_Data.numPointLights++;
  if (type == LightType::DIRECT) s_Data.numDirectLights++;
  s_Data.numLights++;

  UpdateSSBOLightData();
}

void LightManager::EditLight(int32_t index, const std::optional<glm::vec3>& newColor, const std::optional<glm::vec3>& newPosition, const std::optional<glm::vec3>& newRotation)
{
  if (index < 0 || index >= s_Data.lights.size()) {
      GABGL_ERROR("Invalid light index: " + std::to_string(index));
      return;
  }

  std::shared_ptr<LightData>& lightData = s_Data.lights[index];

  glm::vec4 color = glm::vec4(newColor->x,newColor->y,newColor->z,1.0);

  if (newPosition) lightData->position = *newPosition;
  if (newRotation) lightData->rotation = *newRotation;
  if (newColor) lightData->color = color;

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

void LightManager::ResizeLightBuffers(uint32_t newMax)
{
  s_Data.maxLights = newMax;
  s_Data.maxPointLights = newMax - 10;

  s_Data.LightPosStorageBuffer = StorageBuffer::Create(sizeof(glm::vec4) * newMax, 0);
  s_Data.LightRotationStorageBuffer = StorageBuffer::Create(sizeof(glm::vec4) * newMax, 1);
  s_Data.LightQuantityStorageBuffer = StorageBuffer::Create(sizeof(uint32_t), 2);
  s_Data.LightColorStorageBuffer = StorageBuffer::Create(sizeof(glm::vec4) * newMax, 3);
  s_Data.LightTypeStorageBuffer = StorageBuffer::Create(sizeof(uint32_t) * newMax, 4);
}

void LightManager::UpdateSSBOLightData()
{
  s_Data.LightQuantityStorageBuffer->SetData(sizeof(int32_t), &s_Data.numLights);
  
  size_t alignedVec4Size = 16; // vec3 aligned to vec4 size (12 bytes data + 4 bytes padding)
  size_t alignedbufferSize = s_Data.numLights * alignedVec4Size;
  std::vector<int8_t> bufferPositions(alignedbufferSize);

  for (size_t i = 0; i < s_Data.lights.size(); ++i) std::memcpy(bufferPositions.data() + (i * alignedVec4Size), &s_Data.lights[i]->position, sizeof(glm::vec3));
  s_Data.LightPosStorageBuffer->SetData(alignedbufferSize, bufferPositions.data());

  std::vector<int8_t> bufferRotations(alignedbufferSize);

  for (size_t i = 0; i < s_Data.lights.size(); ++i) std::memcpy(bufferRotations.data() + (i * alignedVec4Size), &s_Data.lights[i]->rotation, sizeof(glm::vec3));
  s_Data.LightRotationStorageBuffer->SetData(alignedbufferSize, bufferRotations.data());

  size_t bufferSizeTypes = s_Data.numLights * sizeof(int32_t);
  std::vector<int32_t> bufferTypes(s_Data.numLights);

  for (size_t i = 0; i < s_Data.lights.size(); ++i) bufferTypes[i] = static_cast<int32_t>(s_Data.lights[i]->type);
  s_Data.LightTypeStorageBuffer->SetData(bufferSizeTypes, bufferTypes.data());

  size_t bufferSizeColors = s_Data.numLights * sizeof(glm::vec4);
  std::vector<int8_t> bufferColors(bufferSizeColors);
  for (size_t i = 0; i < s_Data.lights.size(); ++i) std::memcpy(bufferColors.data() + (i * sizeof(glm::vec4)), &s_Data.lights[i]->color, sizeof(glm::vec4));
  s_Data.LightColorStorageBuffer->SetData(bufferSizeColors, bufferColors.data());

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

std::vector<glm::vec3> LightManager::GetPointLightPositions()
{
  std::vector<glm::vec3> positions;
  for (const auto& light : s_Data.lights)
  {
      if (light->type == LightType::POINT)
          positions.push_back(light->position);
  }
  return positions;
}

glm::vec3 LightManager::GetDirectLightRotation()
{
  glm::vec3 rotation;
  for (const auto& light : s_Data.lights)
  {
    if (light->type == LightType::DIRECT)
        rotation = light->rotation;
  }
  return rotation;
}

int32_t LightManager::GetLightsQuantity()
{
  return s_Data.numLights;
}

int32_t LightManager::GetPointLightsQuantity()
{
  return s_Data.numPointLights;
}

bool LightManager::DirectLightEmpty()
{
  return s_Data.numDirectLights == 0;
}

bool LightManager::PointLightEmpty()
{
  return s_Data.numPointLights == 0;
}


