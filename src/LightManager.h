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
        ssbo.PreAllocate(sizeof(glm::vec4) * maxLights + sizeof(int32_t)); 
        numLights = 0; 
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo.GetHandle());
    }

    void UpdateLightDataInSSBO() {
        // Align `numLights` to 16 bytes for std430 layout
        const size_t numLightsOffset = 16; // First element starts aligned
        const size_t positionsOffset = numLightsOffset; // `vec4` starts immediately after

        // Calculate buffer size: padded `numLights` + positions
        size_t bufferSize = positionsOffset + (numLights * sizeof(glm::vec4));

        // Resize buffer
        std::vector<int8_t> buffer(bufferSize);

        // Copy `numLights` (aligned to 16 bytes)
        std::memcpy(buffer.data(), &numLights, sizeof(int32_t));

        // Copy positions to the buffer
        for (size_t i = 0; i < lights.size(); ++i) {
            glm::vec4 positionWithW(
                lights[i].second.position.x,
                lights[i].second.position.y,
                lights[i].second.position.z,
                1.0f
            );
            std::memcpy(buffer.data() + positionsOffset + (i * sizeof(glm::vec4)), &positionWithW, sizeof(glm::vec4));
        }

        ssbo.Update(bufferSize, buffer.data());

        // Synchronize data with GPU
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // DEBUG // 
        std::vector<int8_t> debugBuffer(bufferSize);
        glGetNamedBufferSubData(ssbo.GetHandle(), 0, bufferSize, debugBuffer.data());

        int* debugNumLights = reinterpret_cast<int*>(debugBuffer.data());
        std::cout << "numLights: " << *debugNumLights << std::endl;

        glm::vec4* debugPositions = reinterpret_cast<glm::vec4*>(debugBuffer.data() + positionsOffset);
        for (size_t i = 0; i < *debugNumLights; ++i) {
            std::cout << "positions[" << i << "]: "
                      << debugPositions[i].x << ", "
                      << debugPositions[i].y << ", "
                      << debugPositions[i].z << ", "
                      << debugPositions[i].w << std::endl;
        }
        // DEBUG //
    }

    void AddLight(const Color& color, const glm::vec3& position, const glm::vec3& scale = glm::vec3(1.0f), const float& rotation = 0.0f) {

        if (numLights == maxLights - 1) {
            std::cout << "Max number of lights reached!" << '\n';
            return;
        }

        std::unique_ptr<Light> newLight = std::make_unique<Light>();
        newLight->setLightColor(GetColor(color));
        glm::vec4 lightColor = GetColor(color);

        glm::vec4 positionWithW(position, 1.0f);
        LightData lightData = { positionWithW, scale, rotation, lightColor };

        lights.push_back({ std::move(newLight), lightData });
        numLights++; 
        UpdateLightDataInSSBO();
    }

    void RemoveLight(int index) {
        if (index >= 0 && index < lights.size()) {
            lights.erase(lights.begin() + index);
            numLights--; 
            UpdateLightDataInSSBO();
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
      glm::vec4 position;
      glm::vec3 scale;
      float rotation;
      glm::vec4 color;
    };

    std::vector<std::pair<std::unique_ptr<Light>, LightData>> lights; 
    SSBO ssbo;
    int32_t numLights;  
    const uint32_t maxLights = 10; 
};
