#pragma once 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

struct Transform
{
    Transform() = default;
    Transform(const Transform&) = default;
    Transform(const glm::vec3& position) : m_Position(position) {}
    Transform(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale) : m_Position(position), m_Rotation(rotation), m_Scale(scale) {}

    glm::mat4 GetTransform() const
    {
        glm::mat4 rotation = glm::toMat4(glm::quat(m_Rotation));

        return glm::translate(glm::mat4(1.0f), m_Position)
            * rotation
            * glm::scale(glm::mat4(1.0f), m_Scale);
    }
    glm::vec3 to_forward_vector() {
        glm::quat q = glm::quat(m_Rotation);
        return glm::normalize(q * glm::vec3(0.0f, 0.0f, 1.0f));
    }
    glm::vec3 to_right_vector() {
        glm::quat q = glm::quat(m_Rotation);
        return glm::normalize(q * glm::vec3(1.0f, 0.0f, 0.0f));
    }

    inline void SetPosition(const glm::vec3& position) { m_Position = position; }
    inline glm::vec3 GetPosition() { return m_Position; }
    inline void SetRotation(const glm::vec3& rotation) { m_Rotation = rotation; }
    inline glm::vec3 GetRotation() { return m_Rotation; }
    inline void SetScale(const glm::vec3& scale) { m_Scale = scale; }
    inline glm::vec3 GetScale() { return m_Scale; }

private:

  glm::vec3 m_Position = glm::vec3(0);
  glm::vec3 m_Rotation = glm::vec3(0);
  glm::vec3 m_Scale = glm::vec3(1);
};
