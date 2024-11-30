#pragma once 

#include <cstddef>
#include <cstdint>
#include <vector>
#include <memory>
#include "Light.h"
#include "LoadSSBO.h" 
#include "glad/glad.h"



enum class Color
{
  Red,
  Green,
  Blue,
  Yellow,
  Orange,
  Orange_bright,
  White,
  Purple
};

struct LightManager {


    LightManager() {
        // Pre-allocate SSBOs for positions and colors
        ssboPositions.PreAllocate(sizeof(glm::vec4) * maxLights + sizeof(int32_t)); 
        ssboColors.PreAllocate(sizeof(glm::vec4) * maxLights);  // Separate SSBO for colors

        numLights = 0; 
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboPositions.GetHandle());  // Positions SSBO
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssboColors.GetHandle());     // Colors SSBO
    }

    void UpdateLightDataInSSBO() {
        // First, update the positions SSBO
        const size_t numLightsOffset = 16; // Align to 16 bytes for std430 layout
        const size_t positionsOffset = numLightsOffset; // `vec4` starts immediately after
        size_t bufferSizePositions = positionsOffset + (numLights * sizeof(glm::vec4));

        // Create buffer for position data
        std::vector<int8_t> bufferPositions(bufferSizePositions);
        std::memcpy(bufferPositions.data(), &numLights, sizeof(int32_t));

        // Copy positions to the buffer
        for (size_t i = 0; i < lights.size(); ++i) {
            glm::vec4 positionWithW(lights[i].second.position.x, lights[i].second.position.y, lights[i].second.position.z, 1.0f);
            std::memcpy(bufferPositions.data() + positionsOffset + (i * sizeof(glm::vec4)), &positionWithW, sizeof(glm::vec4));
        }

        ssboPositions.Update(bufferSizePositions, bufferPositions.data());

        // Now update the colors SSBO
        size_t bufferSizeColors = numLights * sizeof(glm::vec4);

        std::vector<int8_t> bufferColors(bufferSizeColors);
        for (size_t i = 0; i < lights.size(); ++i) {
            std::memcpy(bufferColors.data() + (i * sizeof(glm::vec4)), &lights[i].second.color, sizeof(glm::vec4));
        }

        ssboColors.Update(bufferSizeColors, bufferColors.data());

        // Synchronize data with GPU
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // DEBUG: Check the updated data
        std::vector<int8_t> debugBufferPositions(bufferSizePositions);
        glGetNamedBufferSubData(ssboPositions.GetHandle(), 0, bufferSizePositions, debugBufferPositions.data());

        int* debugNumLights = reinterpret_cast<int*>(debugBufferPositions.data());
        std::cout << "numLights: " << *debugNumLights << std::endl;

        glm::vec4* debugPositions = reinterpret_cast<glm::vec4*>(debugBufferPositions.data() + positionsOffset);
        for (size_t i = 0; i < *debugNumLights; ++i) {
            std::cout << "positions[" << i << "]: "
                      << debugPositions[i].x << ", "
                      << debugPositions[i].y << ", "
                      << debugPositions[i].z << ", "
                      << debugPositions[i].w << std::endl;
        }

        std::vector<int8_t> debugBufferColors(bufferSizeColors);
        glGetNamedBufferSubData(ssboColors.GetHandle(), 0, bufferSizeColors, debugBufferColors.data());
        glm::vec4* debugColors = reinterpret_cast<glm::vec4*>(debugBufferColors.data());
        for (size_t i = 0; i < *debugNumLights; ++i) {
            std::cout << "colors[" << i << "]: "
                      << debugColors[i].r << ", "
                      << debugColors[i].g << ", "
                      << debugColors[i].b << ", "
                      << debugColors[i].a << std::endl;
        }
    }

    void AddLight(const Color& color, const glm::vec3& position, const glm::vec3& scale = glm::vec3(1.0f), const float& rotation = 0.0f) {
        if (numLights == maxLights - 1) {
            std::cout << "Max number of lights reached!" << '\n';
            return;
        }

        glm::vec4 lightColor = GetColor(color);
        glm::vec4 positionWithW(position, 1.0f);
        LightData lightData = { positionWithW, scale, rotation, lightColor };

        std::unique_ptr<Light> newLight = std::make_unique<Light>();
        newLight->setLightColor(lightColor);

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
            case Color::Orange: return glm::vec4(0.879f, 0.487f, 0.189f, 1.0f);
            case Color::Orange_bright: return glm::vec4(0.892f, 0.633f, 0.153f, 1.0f);
            case Color::White:  return glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            case Color::Purple: return glm::vec4(0.5f, 0.0f, 0.5f, 1.0f);
            default:     return glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); // Default to white
        }
    }

    struct LightData 
    {
        glm::vec4 position;
        glm::vec3 scale;
        float rotation;
        glm::vec4 color;
    };

    std::vector<std::pair<std::unique_ptr<Light>, LightData>> lights;
    SSBO ssboPositions;  // SSBO for positions
    SSBO ssboColors;     // SSBO for colors
    int32_t numLights;
    const uint32_t maxLights = 10;
};
