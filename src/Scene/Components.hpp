#pragma once

#include <glm/glm.hpp>
#include "../Backend/UUID.h"
#include "../Renderer/Texture.h"

struct IDComponent
{
    UUID ID;

    IDComponent() = default;
    IDComponent(const IDComponent&) = default;
};

struct TransformComponent
{
    glm::vec3 Position = glm::vec3(0);
    glm::vec3 Rotation = glm::vec3(0);
    glm::vec3 Scale = glm::vec3(1);

    TransformComponent() = default;
    TransformComponent(const TransformComponent&) = default;
    TransformComponent(const glm::vec3& position)
        : Position(position) {}

    glm::mat4 GetTransform() const
    {
        glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));

        return glm::translate(glm::mat4(1.0f), Position)
            * rotation
            * glm::scale(glm::mat4(1.0f), Scale);
    }
    glm::vec3 to_forward_vector() {
        glm::quat q = glm::quat(rotation);
        return glm::normalize(q * glm::vec3(0.0f, 0.0f, 1.0f));
    }
    glm::vec3 to_right_vector() {
        glm::quat q = glm::quat(rotation);
        return glm::normalize(q * glm::vec3(1.0f, 0.0f, 0.0f));
    }
};

struct TagComponent
{
    std::string Tag;

    TagComponent() = default;
    TagComponent(const TagComponent&) = default;
    TagComponent(const std::string& tag)
        : Tag(tag) {}
};

struct CameraComponent
{

};

struct SpriteComponent
{
    glm::vec4 Color(1);
    Ref<Texture> Texture;
    float TilingFactor = 1.0f;

    SpriteRendererComponent() = default;
    SpriteRendererComponent(const SpriteRendererComponent&) = default;
    SpriteRendererComponent(const glm::vec4& color)
        : Color(color) {}
};

struct MeshComponent
{

};
