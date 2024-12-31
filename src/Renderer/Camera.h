#pragma once

#include <glm/glm.hpp>

struct Camera
{
    virtual void SetProjection(float width, float height) = 0;
    virtual glm::mat4 GetViewProjectionMatrix() const = 0;

    virtual ~Camera() = default;

protected:
    glm::mat4 m_ProjectionMatrix;
    glm::mat4 m_ViewMatrix;
};