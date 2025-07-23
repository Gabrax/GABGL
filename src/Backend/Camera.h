#pragma once

#include "../input/EngineEvent.h"
#include "../input/KeyEvent.h"
#include "DeltaTime.hpp"
#include "BackendLogger.h"

#include <glm/glm.hpp>

enum class CameraMode
{
  ORBITAL,
  FPS
};

struct Camera
{
  enum class ProjectionType { Perspective = 0, Orthographic = 1 };

  Camera() = default;
  Camera(const glm::vec3& position);
  Camera(float fov, float aspectRatio, float nearClip, float farClip);
  Camera(const glm::mat4& projection);
  ~Camera() = default;

  void OnUpdate(DeltaTime dt);

  inline float GetDistance() const { return m_Distance; }
  inline void SetDistance(float distance) { m_Distance = distance; }

  void SetViewportSize(float width, float height);
  void SetViewportSize(uint32_t width, uint32_t height);

  void SetMode(CameraMode mode);

  inline const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
  inline const glm::mat4& GetProjection() const { return m_Projection; }
  inline glm::mat4 GetViewProjection() const { return m_Projection * m_ViewMatrix; }
  inline glm::mat4 GetNonRotationViewProjection() const { return m_Projection * glm::mat4(glm::mat3(m_ViewMatrix)); }
  inline glm::mat4 GetOrtoProjection() const { return glm::ortho(0.0f, static_cast<float>(m_ViewportWidth), 0.0f, static_cast<float>(m_ViewportHeight)); }

  void UpdateProjection();

  glm::vec3 GetUpDirection() const;
  glm::vec3 GetRightDirection() const;
  glm::vec3 GetForwardDirection() const;
  inline const glm::vec3& GetPosition() const { return m_Position; }
  glm::quat GetOrientation() const;

  float GetPitch() const { return m_Pitch; }
  float GetYaw() const { return m_Yaw; }

  void SetPerspective(float verticalFOV, float nearClip, float farClip);
  void SetOrthographic(float size, float nearClip, float farClip);

  inline float GetPerspectiveVerticalFOV() const { return m_PerspectiveFOV; }
  inline void SetPerspectiveVerticalFOV(float verticalFov) { m_PerspectiveFOV = verticalFov; RecalculateProjection(); }
  inline float GetPerspectiveNearClip() const { return m_PerspectiveNear; }
  inline void SetPerspectiveNearClip(float nearClip) { m_PerspectiveNear = nearClip; RecalculateProjection(); }
  inline float GetPerspectiveFarClip() const { return m_PerspectiveFar; }
  inline void SetPerspectiveFarClip(float farClip) { m_PerspectiveFar = farClip; RecalculateProjection(); }

  inline float GetOrthographicSize() const { return m_OrthographicSize; }
  inline void SetOrthographicSize(float size) { m_OrthographicSize = size; RecalculateProjection(); }
  inline float GetOrthographicNearClip() const { return m_OrthographicNear; }
  inline void SetOrthographicNearClip(float nearClip) { m_OrthographicNear = nearClip; RecalculateProjection(); }
  inline float GetOrthographicFarClip() const { return m_OrthographicFar; }
  inline void SetOrthographicFarClip(float farClip) { m_OrthographicFar = farClip; RecalculateProjection(); }

  inline ProjectionType GetProjectionType() const { return m_ProjectionType; }
  inline void SetProjectionType(ProjectionType type) { m_ProjectionType = type; RecalculateProjection(); }

private:
  void RecalculateProjection();
  void UpdateView();

  bool OnMouseScroll(MouseScrolledEvent& e);

  void MousePan(const glm::vec2& delta);
  void MouseRotate(float xoffset, float yoffset, bool constrainPitch = true);
  void MouseZoom(float delta);

  glm::vec3 CalculatePosition() const;

  std::pair<float, float> PanSpeed() const;
  float RotationSpeed() const;
  float ZoomSpeed() const;
  void UpdateCameraVectors();

private:

  CameraMode m_Mode = CameraMode::FPS;
  ProjectionType m_ProjectionType = ProjectionType::Perspective;

  // Perspective parameters
  float m_PerspectiveFOV = glm::radians(45.0f);
  float m_PerspectiveNear = 0.001f;
  float m_PerspectiveFar = 2000.0f;

  // Orthographic parameters
  float m_OrthographicSize = 10.0f;
  float m_OrthographicNear = -1.0f;
  float m_OrthographicFar = 1.0f;

  float m_AspectRatio = 16.0f / 9.0f;

  glm::mat4 m_Projection = glm::mat4(1.0f);
  glm::mat4 m_ViewMatrix;

  glm::vec3 m_Position = { 0.0f, 5.0f, 5.0f };
  glm::vec3 m_FocalPoint = { 0.0f, 0.0f, 0.0f };

  glm::vec2 m_InitialMousePosition = { 0.0f, 0.0f };

  glm::vec3 m_FPS_Position = { 0.0f, 5.0f, 5.0f };
  float m_FPS_Yaw = 0.0f;
  float m_FPS_Pitch = 0.0f;

  glm::vec3 m_Orbital_FocalPoint = { 0.0f, 0.0f, 0.0f };
  float m_Orbital_Distance = 100.0f;
  float m_Orbital_Yaw = 0.0f;
  float m_Orbital_Pitch = 0.0f;

  float m_MovementSpeed = 5.0f;
  float m_MouseSensitivity = 1.0f;
  glm::vec3 m_Front = glm::vec3(0.0f, 0.0f, -1.0f);
  glm::vec3 m_Up = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 m_WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 m_Right = glm::vec3(1.0f, 0.0f, 0.0f);

  float m_Distance = 0.0f;
  float m_Pitch = 0.0f;
  float m_Yaw = 0.0f;

  float m_ViewportWidth = 0;
  float m_ViewportHeight = 0;
};
