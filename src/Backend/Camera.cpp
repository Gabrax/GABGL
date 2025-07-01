#include "Camera.h"

#include "../backend/BackendLogger.h"
#include "../input/UserInput.h"
#include "../input/KeyEvent.h"

#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

Camera::Camera(const glm::vec3& position) : m_Position(position)
{
    RecalculateProjection();
    UpdateView();
};

Camera::Camera(float fov, float aspectRatio, float nearClip, float farClip)
    : m_ProjectionType(ProjectionType::Perspective),
      m_PerspectiveFOV(glm::radians(fov)),
      m_AspectRatio(aspectRatio),
      m_PerspectiveNear(nearClip),
      m_PerspectiveFar(farClip),
      m_Distance(1000.0f),
      m_Pitch(0.0f),
      m_Yaw(0.0f)
{
    RecalculateProjection();
    UpdateView();
}

Camera::Camera(const glm::mat4& projection) : m_Projection(projection)
{
    RecalculateProjection();
    UpdateView();
};

void Camera::SetMode(CameraMode mode)
{
  if (mode == m_Mode) return;

  // Save current mode's state before switching
  if (m_Mode == CameraMode::FPS)
  {
    m_FPS_Position = m_Position;
    m_FPS_Yaw = m_Yaw;
    m_FPS_Pitch = m_Pitch;
  }
  else if (m_Mode == CameraMode::ORBITAL)
  {
    m_Orbital_FocalPoint = m_FocalPoint;
    m_Orbital_Distance = m_Distance;
    m_Orbital_Yaw = m_Yaw;
    m_Orbital_Pitch = m_Pitch;
  }

  // Load new mode's state
  if (mode == CameraMode::FPS)
  {
    m_Position = m_FPS_Position;
    m_Yaw = m_FPS_Yaw;
    m_Pitch = m_FPS_Pitch;
    UpdateCameraVectors();
  }
  else if (mode == CameraMode::ORBITAL)
  {
    m_FocalPoint = m_Orbital_FocalPoint;
    m_Distance = m_Orbital_Distance;
    m_Yaw = m_Orbital_Yaw;
    m_Pitch = m_Orbital_Pitch;
    m_Position = CalculatePosition();
    UpdateCameraVectors();
  }

  m_Mode = mode;
  UpdateView();
}

void Camera::UpdateProjection()
{
    // Deprecated, you can call RecalculateProjection() instead
  m_AspectRatio = m_ViewportWidth / m_ViewportHeight;

  m_Projection = glm::perspective(m_PerspectiveFOV, m_AspectRatio, m_PerspectiveNear, m_PerspectiveFar);
}

void Camera::UpdateView()
{
  if (m_Mode == CameraMode::ORBITAL)
  {
      m_ViewMatrix = glm::lookAt(m_Position, m_FocalPoint, m_Up);
  }
  else
  {
      m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_Front, m_Up);
  }
}

std::pair<float, float> Camera::PanSpeed() const
{
	float x = std::min(m_ViewportWidth / 1000.0f, 2.4f); // max = 2.4f
	float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

	float y = std::min(m_ViewportHeight / 1000.0f, 2.4f); // max = 2.4f
	float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

	return { xFactor, yFactor };
}

float Camera::RotationSpeed() const
{
	return 0.8f;
}

float Camera::ZoomSpeed() const
{
	float distance = m_Distance * 0.2f;
	distance = std::max(distance, 0.0f);
	float speed = distance * distance;
	speed = std::min(speed, 100.0f); // max speed = 100
	return speed;
}

void Camera::OnUpdate(DeltaTime dt)
{
  glm::vec2 mouse{ Input::GetMouseX(), Input::GetMouseY() };
  glm::vec2 delta = (mouse - m_InitialMousePosition) * 0.1f;
  m_InitialMousePosition = mouse;

  if (m_Mode == CameraMode::ORBITAL)
  {
      if (Input::IsMouseButtonPressed(Mouse::ButtonRight))
      {
          MouseRotate(delta.x, delta.y);
      }

      float zoomDelta = 0.0f;
      if (Input::IsKeyPressed(Key::F)) // Zoom in
          zoomDelta = 0.5f;
      if (Input::IsKeyPressed(Key::G)) // Zoom out
          zoomDelta = -0.5f;

      if (zoomDelta != 0.0f)
          MouseZoom(zoomDelta * dt * 10.0f); // scale zoom speed by dt

      m_Position = CalculatePosition();
      UpdateView();
  }
  else 
  {
      MouseRotate(delta.x, -delta.y);

      float velocity = m_MovementSpeed * dt;

      if (Input::IsKeyPressed(Key::W))
          m_Position += m_Front * velocity;
      if (Input::IsKeyPressed(Key::S))
          m_Position -= m_Front * velocity;
      if (Input::IsKeyPressed(Key::A))
          m_Position -= m_Right * velocity;
      if (Input::IsKeyPressed(Key::D))
          m_Position += m_Right * velocity;
      if (Input::IsKeyPressed(Key::Space))
          m_Position += m_Up * velocity;
      if (Input::IsKeyPressed(Key::LeftControl))
          m_Position -= m_Up * velocity;

      UpdateView();
  }
}

bool Camera::OnMouseScroll(MouseScrolledEvent& e)
{
	float delta = e.GetYOffset() * 0.1f;
	MouseZoom(delta);
	UpdateView();
	return false;
}

void Camera::MousePan(const glm::vec2& delta)
{
	auto [xSpeed, ySpeed] = PanSpeed();
	m_FocalPoint += -GetRightDirection() * delta.x * xSpeed * m_Distance;
	m_FocalPoint += GetUpDirection() * delta.y * ySpeed * m_Distance;
}

void Camera::MouseRotate(float xoffset, float yoffset, bool constrainPitch)
{
  xoffset *= m_MouseSensitivity;
  yoffset *= m_MouseSensitivity;

  m_Yaw += xoffset;
  m_Pitch += yoffset;

  if (constrainPitch)
  {
    if (m_Pitch > 89.0f) m_Pitch = 89.0f;
    if (m_Pitch < -89.0f) m_Pitch = -89.0f;
  }

  UpdateCameraVectors();
}

void Camera::MouseZoom(float delta)
{
  // Apply zoom only if orbital mode active
  if (m_Mode == CameraMode::ORBITAL)
  {
    // Adjust distance by delta scaled by zoom speed
    m_Distance -= delta * ZoomSpeed();

    // Clamp distance
    if (m_Distance < 1.0f)
        m_Distance = 1.0f;
    if (m_Distance > 1000.0f) // optional max zoom out limit
        m_Distance = 1000.0f;

    // Keep saved orbital distance in sync
    m_Orbital_Distance = m_Distance;

    // Update position based on new distance
    m_Position = CalculatePosition();
    UpdateView();
  }
}

void Camera::UpdateCameraVectors()
{
  glm::vec3 front;
  front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
  front.y = sin(glm::radians(m_Pitch));
  front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
  m_Front = glm::normalize(front);

  // Also re-calculate the Right and Up vector
  m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
  m_Up = glm::normalize(glm::cross(m_Right, m_Front));
}

glm::vec3 Camera::GetUpDirection() const
{
	return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 Camera::GetRightDirection() const
{
	return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 Camera::GetForwardDirection() const
{
	return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::vec3 Camera::CalculatePosition() const
{
  glm::vec3 direction;
  direction.x = cos(glm::radians(m_Pitch)) * cos(glm::radians(m_Yaw));
  direction.y = sin(glm::radians(m_Pitch));
  direction.z = cos(glm::radians(m_Pitch)) * sin(glm::radians(m_Yaw));

  return m_FocalPoint + direction * m_Distance;
}

glm::quat Camera::GetOrientation() const
{
	return glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
}

void Camera::SetPerspective(float verticalFOV, float nearClip, float farClip)
{
	m_ProjectionType = ProjectionType::Perspective;
	m_PerspectiveFOV = verticalFOV;
	m_PerspectiveNear = nearClip;
	m_PerspectiveFar = farClip;
	RecalculateProjection();
}

void Camera::SetOrthographic(float size, float nearClip, float farClip)
{
	m_ProjectionType = ProjectionType::Orthographic;
	m_OrthographicSize = size;
	m_OrthographicNear = nearClip;
	m_OrthographicFar = farClip;
	RecalculateProjection();
}

void Camera::SetViewportSize(uint32_t width, uint32_t height)
{
  GABGL_ASSERT(width > 0 && height > 0, "SIZE BELOW ZERO");
  m_ViewportWidth = static_cast<float>(width);
  m_ViewportHeight = static_cast<float>(height);
  m_AspectRatio = m_ViewportWidth / m_ViewportHeight;
  RecalculateProjection();
}

void Camera::SetViewportSize(float width, float height)
{
  GABGL_ASSERT(width > 0 && height > 0, "SIZE BELOW ZERO");
  m_ViewportWidth = width;
  m_ViewportHeight = height;
  m_AspectRatio = m_ViewportWidth / m_ViewportHeight;
  RecalculateProjection();
}

void Camera::RecalculateProjection()
{
  if (m_ProjectionType == ProjectionType::Perspective)
  {
      // m_PerspectiveFOV is already in radians
      m_Projection = glm::perspective(m_PerspectiveFOV, m_AspectRatio, m_PerspectiveNear, m_PerspectiveFar);
  }
  else
  {
      float orthoLeft = -m_OrthographicSize * m_AspectRatio * 0.5f;
      float orthoRight = m_OrthographicSize * m_AspectRatio * 0.5f;
      float orthoBottom = -m_OrthographicSize * 0.5f;
      float orthoTop = m_OrthographicSize * 0.5f;

      m_Projection = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, m_OrthographicNear, m_OrthographicFar);
  }
}
