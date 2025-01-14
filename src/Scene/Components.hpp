#pragma once

#include "../Backend/UUID.h"
#include "../Renderer/Texture.h"
#include "SceneCamera.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>


struct IDComponent
{
    UUID ID;

    IDComponent() = default;
    IDComponent(const IDComponent&) = default;
    explicit IDComponent(const UUID& id) : ID(id) {}
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
        glm::quat q = glm::quat(Rotation);
        return glm::normalize(q * glm::vec3(0.0f, 0.0f, 1.0f));
    }
    glm::vec3 to_right_vector() {
        glm::quat q = glm::quat(Rotation);
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
    SceneCamera Camera;
    bool Primary = true; 
    bool FixedAspectRatio = false;

    CameraComponent() = default;
    CameraComponent(const CameraComponent&) = default;
};

struct TextureComponent
{
    glm::vec4 Color = glm::vec4(1);
    Ref<Texture> Texture;
    float TilingFactor = 1.0f;

    TextureComponent() = default;
    TextureComponent(const TextureComponent&) = default;
    TextureComponent(const glm::vec4& color)
        : Color(color) {}
};

struct MeshComponent
{

};

struct AudioComponent
{

};

struct TextComponent
{
    std::string TextString;
    //Ref<Font> FontAsset = Font::GetDefault();
    glm::vec4 Color = glm::vec4(1);
    float Kerning = 0.0f;
    float LineSpacing = 0.0f;
};

template<typename... Component>
struct ComponentGroup {};

using AllComponents = ComponentGroup<TransformComponent, TextureComponent, CameraComponent, TextComponent>;
