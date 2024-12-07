#pragma once 

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>
#include <memory>
#include "Light.h"
#include "../LoadSSBO.h" 
#include "glad/glad.h"
#include "glm/fwd.hpp"
#include <optional>

struct LightManager {

    LightManager()
    {
        ssbonumLights.PreAllocate(sizeof(int32_t)); 
        ssboPositions.PreAllocate(sizeof(glm::vec3) * maxLights); 
        ssboColors.PreAllocate(sizeof(glm::vec4) * maxLights);  

        numLights = 0; 
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbonumLights.GetHandle());  
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssboPositions.GetHandle());  
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssboColors.GetHandle());     
    }

    void AddLight(const glm::vec4& color, const glm::vec3& position, const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f))
    {
        if (numLights == maxLights - 1) {
            std::cout << "Max number of lights reached!" << '\n';
            return;
        }

        LightData lightData = { position, rotation, scale, color };

        std::unique_ptr<Light> newLight = std::make_unique<Light>();
        newLight->setLightColor(color);

        lights.push_back({ std::move(newLight), lightData });
        numLights++;
        UpdateLightDataInSSBO();
    }

    void EditLight(int index, const std::optional<glm::vec4>& newColor = std::nullopt, 
               const std::optional<glm::vec3>& newPosition = std::nullopt,
               const std::optional<glm::vec3>& newRotation = std::nullopt,    
               const std::optional<glm::vec3>& newScale = std::nullopt) 
    {
      if (index < 0 || index >= lights.size()) {
          std::cerr << "Invalid light index: " << index << std::endl;
          return;
      }

      LightData& lightData = lights[index].second;

      if (newColor.has_value()) {
          lightData.color = newColor.value();
          lights[index].first->setLightColor(lightData.color); 
      }

      if (newPosition.has_value()) {
          lightData.position = newPosition.value();
      }

      if (newRotation.has_value()) {
          lightData.rotation = newRotation.value();
      }

      if (newScale.has_value()) {
          lightData.scale = newScale.value();
      }

      UpdateLightDataInSSBO();
    }

    void RemoveLight(int index)
    {
        if (index >= 0 && index < lights.size()) {
            lights.erase(lights.begin() + index);
            numLights--;
            UpdateLightDataInSSBO();
        }
    }

    void RenderLights()
    {
        for (const auto& lightPair : lights) {
            lightPair.first->Render(lightPair.second.position, lightPair.second.rotation, lightPair.second.scale);
        }
    }

    struct LightData 
    {
        glm::vec3 position;
        glm::vec3 rotation;
        glm::vec3 scale;
        glm::vec4 color;
    };

    std::vector<std::pair<std::unique_ptr<Light>, LightData>> lights;

private:

  SSBO ssbonumLights;  
  SSBO ssboPositions;  
  SSBO ssboColors;     
  int32_t numLights;
  const uint32_t maxLights = 10;

  void UpdateLightDataInSSBO()
  {
        
        ssbonumLights.Update(sizeof(int32_t), &numLights);
        
        size_t alignedVec4Size = 16; // vec3 aligned to vec4 size (12 bytes data + 4 bytes padding)
        size_t bufferSizePositions = numLights * alignedVec4Size;
        std::vector<int8_t> bufferPositions(bufferSizePositions);

        for (size_t i = 0; i < lights.size(); ++i) {
            // Copy the vec3 into the buffer, leaving 4 bytes padding at the end
            std::memcpy(bufferPositions.data() + (i * alignedVec4Size), &lights[i].second.position, sizeof(glm::vec3));
            // Padding is automatically taken care of by leaving the last 4 bytes unused
        }

        ssboPositions.Update(bufferSizePositions, bufferPositions.data());

        // Now update the colors SSBO
        size_t bufferSizeColors = numLights * sizeof(glm::vec4);
        std::vector<int8_t> bufferColors(bufferSizeColors);
        for (size_t i = 0; i < lights.size(); ++i) {
            std::memcpy(bufferColors.data() + (i * sizeof(glm::vec4)), &lights[i].second.color, sizeof(glm::vec4));
        }
        ssboColors.Update(bufferSizeColors, bufferColors.data());

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
};


