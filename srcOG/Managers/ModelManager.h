#pragma once
#include "AnimatedModel.h"
#include "StaticModel.h"
#include <optional>

struct ModelManager {

    ModelManager() = default;

    void AddModelStatic(const std::string& modelpath, const glm::vec3& position, const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f))
    {
        ModelData modelData = { modelpath, position, scale, rotation };

        std::unique_ptr<StaticModel> newModel = std::make_unique<StaticModel>(modelpath);
        vec_staticModels.emplace_back(std::move(newModel), modelData);
    }

    void AddModelAnimated(const std::string& modelpath, const glm::vec3& position, const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f))
    {
        ModelData modelData = { modelpath, position, scale, rotation };

        std::unique_ptr<AnimatedModel> newModel = std::make_unique<AnimatedModel>(modelpath);
        vec_animatedModels.emplace_back(std::move(newModel), modelData);
    }

    void EditStaticModel(int index, const std::optional<glm::vec3>& newPosition = std::nullopt, const std::optional<glm::vec3>& newRotation = std::nullopt, const std::optional<glm::vec3>& newScale = std::nullopt)
    {
        if (index < 0 || index >= vec_staticModels.size()) {
            std::cerr << "Invalid model index: " << index << std::endl;
            return;
        }

        ModelData& modelData = vec_staticModels[index].second;

        if (newPosition.has_value()) {
            modelData.position = newPosition.value();
        }

        if (newRotation.has_value()) {
            modelData.rotation = newRotation.value();
        }

        if (newScale.has_value()) {
            modelData.scale = newScale.value();
        }

    }
  
    void EditAnimatedModel(int index, const std::optional<glm::vec3>& newPosition = std::nullopt, const std::optional<glm::vec3>& newRotation = std::nullopt, const std::optional<glm::vec3>& newScale = std::nullopt)
    {
        if (index < 0 || index >= vec_animatedModels.size()) {
            std::cerr << "Invalid model index: " << index << std::endl;
            return;
        }

        ModelData& modelData = vec_animatedModels[index].second;

        if (newPosition.has_value()) {
            modelData.position = newPosition.value();
        }

        if (newRotation.has_value()) {
            modelData.rotation = newRotation.value();
        }

        if (newScale.has_value()) {
            modelData.scale = newScale.value();
        }

    }
  
    void RemoveStaticModel(int index)
    {
        if (index >= 0 && index < vec_staticModels.size()) {
            vec_staticModels.erase(vec_staticModels.begin() + index);
        }
    }

    void RemoveAnimatedModel(int index)
    {
        if (index >= 0 && index < vec_animatedModels.size()) {
            vec_animatedModels.erase(vec_animatedModels.begin() + index);
        }
    }

    void RenderModels()
    {
        // Render static models
        for (const auto& modelPair : vec_staticModels) {
            modelPair.first->Render(modelPair.second.position, modelPair.second.rotation, modelPair.second.scale);
        }

        // Render animated models
        for (const auto& modelPair : vec_animatedModels) {
            modelPair.first->Render(modelPair.second.position, modelPair.second.rotation, modelPair.second.scale);
        }
    }

    struct ModelData 
    {
        std::string modelpath;
        glm::vec3 position;
        glm::vec3 scale;
        glm::vec3 rotation;
    };

    std::vector<std::pair<std::unique_ptr<StaticModel>, ModelData>> vec_staticModels;
    std::vector<std::pair<std::unique_ptr<AnimatedModel>, ModelData>> vec_animatedModels;
};
