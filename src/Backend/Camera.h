#pragma once

#include "../input/KeyEvent.h"
#include "DeltaTime.hpp"
#include "glm/ext/matrix_clip_space.hpp"

#include <glm/glm.hpp>

enum class CameraMode
{
  ORBITAL,
  PLAYER,
  PLAIN
};

enum class ProjectionType { Perspective = 0, Orthographic = 1 };

struct Camera
{
  static void Init(float fov, float aspectRatio, float nearClip, float farClip);

  static void OnUpdate(DeltaTime dt);

  static float GetDistance();
  static void SetDistance(float distance);

  static void SetViewportSize(float width, float height);
  static void SetViewportSize(uint32_t width, uint32_t height);

  static void SetMode(CameraMode mode);

  static const glm::mat4& GetViewMatrix();
  static const glm::mat4& GetProjection();
  static glm::mat4 GetViewProjection();
  static glm::mat4 GetNonRotationViewProjection();
  static glm::mat4 GetOrtoProjection();

  static void UpdateProjection();

  static glm::vec3 GetUpDirection();
  static glm::vec3 GetRightDirection();
  static glm::vec3 GetForwardDirection();
  static const glm::vec3& GetPosition();
  static glm::quat GetOrientation();

  static float GetPitch();
  static float GetYaw();

  static void SetPerspective(float verticalFOV, float nearClip, float farClip);
  static void SetOrthographic(float size, float nearClip, float farClip);

private:

  static void RecalculateProjection();
  static void UpdateView();

  static bool OnMouseScroll(MouseScrolledEvent& e);

  static void MousePan(const glm::vec2& delta);
  static void MouseRotate(float xoffset, float yoffset, bool constrainPitch = true);
  static void MouseZoom(float delta);

  static glm::vec3 CalculatePosition();

  static std::pair<float, float> PanSpeed();
  static float RotationSpeed();
  static float ZoomSpeed();
  static void UpdateCameraVectors();

};
