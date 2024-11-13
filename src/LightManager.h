#pragma once 

#include <vector>
#include <memory>
#include "Light.h"

#include "LoadSSBO.h" // Include your SSBO class header here

class LightManager {
public:
    struct LightData {
        glm::vec3 position;
        glm::vec3 scale;
        float rotation;
    };

    LightManager() {
        ssbo.PreAllocate(sizeof(glm::vec3) * maxLights + sizeof(int)); // Preallocate space for lights and numLights
        numLights = 0; // Initialize the number of lights to 0
    }

    void AddLight(const glm::vec3& position, const glm::vec4& color, const glm::vec3& scale = glm::vec3(1.0f), float rotation = 0.0f) {
        std::shared_ptr<Light> newLight = std::make_shared<Light>();
        newLight->getLightColor() = color;

        LightData lightData = { position, scale, rotation };
        lights.push_back({ newLight, lightData });

        numLights++; // Increase the count when a light is added
    }

    void RemoveLight(int index) {
        if (index >= 0 && index < lights.size()) {
            lights.erase(lights.begin() + index);
            numLights--; // Decrease the count when a light is removed
        }
    }

    void UpdateLightPositionsInSSBO() {
        std::vector<glm::vec3> positions;
        for (const auto& lightPair : lights) {
            positions.push_back(lightPair.second.position);
        }

        // Prepare the data to send to the SSBO: numLights followed by the positions
        std::vector<char> buffer;
        buffer.resize(sizeof(int) + positions.size() * sizeof(glm::vec3));

        // Copy numLights at the start of the buffer
        std::memcpy(buffer.data(), &numLights, sizeof(int));

        // Copy the positions array after numLights
        std::memcpy(buffer.data() + sizeof(int), positions.data(), positions.size() * sizeof(glm::vec3));

        // Update the SSBO with the new data
        ssbo.Update(buffer.size(), buffer.data());
    }

    void RenderLights() {
        // Update positions in SSBO before rendering
        UpdateLightPositionsInSSBO();

        // Now bind SSBO to a binding point accessible in the shader
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo.GetHandle());

        // Render each light
        for (const auto& lightPair : lights) {
            lightPair.first->Render(lightPair.second.position, lightPair.second.scale, lightPair.second.rotation);
        }
    }

private:
    std::vector<std::pair<std::shared_ptr<Light>, LightData>> lights;
    SSBO ssbo;
    int numLights;  // Store the number of lights
    const size_t maxLights = 100; // Set a max limit on the number of lights
};
