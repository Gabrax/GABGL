#pragma once 

#include <cstddef>
#include <cstdint>
#include <vector>
#include <memory>
#include "Light.h"
#include "LoadSSBO.h" 
#include "glad/glad.h"

#define RED     LightManager::Color::Red
#define GREEN   LightManager::Color::Green
#define BLUE    LightManager::Color::Blue
#define YELLOW  LightManager::Color::Yellow
#define WHITE   LightManager::Color::White
#define PURPLE  LightManager::Color::Purple

struct LightManager {

    enum Color
    {
      Red,
      Green,
      Blue,
      Yellow,
      White,
      Purple
    };

    LightManager() {
        ssbo.PreAllocate(sizeof(glm::vec3) * maxLights + sizeof(int32_t)); 
        numLights = 0; 
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo.GetHandle());
    }

    void UpdateLightPositionsInSSBO() {
        std::vector<glm::vec3> positions;
        for (const auto& lightPair : lights) {
            positions.emplace_back(lightPair.second.position);
        }

        std::vector<int8_t> buffer;
        buffer.resize(sizeof(int32_t) + positions.size() * sizeof(glm::vec3));

        std::memcpy(buffer.data(), &numLights, sizeof(int32_t));
        std::memcpy(buffer.data() + sizeof(int32_t), positions.data(), positions.size() * sizeof(glm::vec3));

        ssbo.Update(buffer.size(), buffer.data());

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);


        //DEBUG//
        std::vector<int8_t> debugBuffer(buffer.size());
        glGetNamedBufferSubData(ssbo.GetHandle(), 0, buffer.size(), debugBuffer.data()); // Retrieve SSBO data

        int* numLightsPtr = reinterpret_cast<int*>(debugBuffer.data());
        std::cout << "numLights: " << *numLightsPtr << std::endl;

        glm::vec3* positionsPtr = reinterpret_cast<glm::vec3*>(debugBuffer.data() + sizeof(int32_t));
        for (size_t i = 0; i < (*numLightsPtr); ++i) {
            std::cout << "positions[" << i << "]: "
                      << positionsPtr[i].x << ", "
                      << positionsPtr[i].y << ", "
                      << positionsPtr[i].z << std::endl;
        }
        //DEBUG//
    }

    void AddLight(const Color& color, const glm::vec3& position, const glm::vec3& scale = glm::vec3(1.0f), const float& rotation = 0.0f) {

        if (numLights >= maxLights) {
            std::cout << "Max number of lights reached!" << std::endl;
            return;
        }

        std::unique_ptr<Light> newLight = std::make_unique<Light>();
        newLight->setLightColor(GetColor(color)); // Set the light color using the enum

        LightData lightData = { position, scale, rotation };
        lights.push_back({ std::move(newLight), lightData });
        numLights++; 
        UpdateLightPositionsInSSBO();
    }

    void RemoveLight(int index) {
        if (index >= 0 && index < lights.size()) {
            lights.erase(lights.begin() + index);
            numLights--; 
            UpdateLightPositionsInSSBO();
        }
    }

    void RenderLights() {
        for (const auto& lightPair : lights) {
            lightPair.first->Render(lightPair.second.position, lightPair.second.scale, lightPair.second.rotation);
        }
    }

private:


    glm::vec4 GetColor(Color color) {
      switch (color) {
          case Color::Red:    return glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
          case Color::Green:  return glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
          case Color::Blue:   return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
          case Color::Yellow: return glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
          case Color::White:  return glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
          case Color::Purple: return glm::vec4(0.5f, 0.0f, 0.5f, 1.0f);
          default:     return glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); // Default to white
      }
    }

    struct LightData {
      glm::vec3 position;
      glm::vec3 scale;
      float rotation;
    };

    std::vector<std::pair<std::unique_ptr<Light>, LightData>> lights; 
    SSBO ssbo;
    int32_t numLights;  
    const uint32_t maxLights = 10; 
};
