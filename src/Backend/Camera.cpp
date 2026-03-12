#include "Camera.h"

#include "../backend/Logger.h"
#include "../input/UserInput.h"
#include "../input/KeyEvent.h"

#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include "ModelManager.h"


static CameraMode m_Mode = CameraMode::PLAYER;
static ProjectionType m_ProjectionType = ProjectionType::Perspective;

// Perspective parameters
static float m_PerspectiveFOV = glm::radians(45.0f);
static float m_PerspectiveNear = 0.001f;
static float m_PerspectiveFar = 2000.0f;

// Orthographic parameters
static float m_OrthographicSize = 10.0f;
static float m_OrthographicNear = -1.0f;
static float m_OrthographicFar = 1.0f;

static float m_AspectRatio = 16.0f / 9.0f;

static glm::mat4 m_Projection = glm::mat4(1.0f);
static glm::mat4 m_ViewMatrix;

static glm::vec3 m_Position = { 0.0f, 5.0f, 5.0f };
static glm::vec3 m_FocalPoint = { 0.0f, 0.0f, 0.0f };

static glm::vec2 m_InitialMousePosition = { 0.0f, 0.0f };

static glm::vec3 m_FPS_Position = { 0.0f, 5.0f, 5.0f };
static float m_FPS_Yaw = 0.0f;
static float m_FPS_Pitch = 0.0f;

static glm::vec3 m_Orbital_FocalPoint = { 0.0f, 0.0f, 0.0f };
static float m_Orbital_Distance = 100.0f;
static float m_Orbital_Yaw = 0.0f;
static float m_Orbital_Pitch = 0.0f;

static float m_MovementSpeed = 5.0f;
static float m_MouseSensitivity = 1.0f;
static glm::vec3 m_Front = glm::vec3(0.0f, 0.0f, -1.0f);
static glm::vec3 m_Up = glm::vec3(0.0f, 1.0f, 0.0f);
static glm::vec3 m_WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
static glm::vec3 m_Right = glm::vec3(1.0f, 0.0f, 0.0f);

static float m_Distance = 0.0f;
static float m_Pitch = 0.0f;
static float m_Yaw = 0.0f;

static float m_ViewportWidth = 0;
static float m_ViewportHeight = 0;

void Camera::Init(float fov, float aspectRatio, float nearClip, float farClip)
{
  m_ProjectionType = ProjectionType::Perspective;
  m_PerspectiveFOV = glm::radians(fov);
  m_PerspectiveNear = nearClip;
  m_PerspectiveFar = farClip;
  m_AspectRatio = aspectRatio;
  m_Distance = 1000.0f;
  m_Pitch = 0.0f;
  m_Yaw = 0.0f;

  RecalculateProjection();
  UpdateView();
}

void Camera::SetMode(CameraMode mode)
{
  if (mode == m_Mode) return;

  // Save current mode's state before switching
  if (m_Mode == CameraMode::PLAYER)
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
  if (mode == CameraMode::PLAYER)
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

std::pair<float, float> Camera::PanSpeed()
{
	float x = std::min(m_ViewportWidth / 1000.0f, 2.4f); // max = 2.4f
	float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

	float y = std::min(m_ViewportHeight / 1000.0f, 2.4f); // max = 2.4f
	float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

	return { xFactor, yFactor };
}

float Camera::RotationSpeed()
{
	return 0.8f;
}

float Camera::ZoomSpeed()
{
	float distance = m_Distance * 0.2f;
	distance = std::max(distance, 0.0f);
	float speed = distance * distance;
	speed = std::min(speed, 100.0f); // max speed = 100
	return speed;
}

float m_HeightOffset   = 4.0f;
float m_ShoulderOffset = 15.6f;
float m_FollowDistance = 4.0f;
float m_SmoothSpeed = 5.0f;

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
    if (Input::IsKeyPressed(Key::F)) zoomDelta = 0.5f;
    if (Input::IsKeyPressed(Key::G)) zoomDelta = -0.5f;

    if (zoomDelta != 0.0f) MouseZoom(zoomDelta * dt * 10.0f); // scale zoom speed by dt

    m_Position = CalculatePosition();
    UpdateView();
  }
  else if(m_Mode == CameraMode::PLAYER) 
  {
    MouseRotate(delta.x, -delta.y);

    /*float velocity = m_MovementSpeed * dt;*/
    /**/
    /*if (Input::IsKeyPressed(Key::W)) m_Position += m_Front * velocity;*/
    /*if (Input::IsKeyPressed(Key::S)) m_Position -= m_Front * velocity;*/
    /*if (Input::IsKeyPressed(Key::A)) m_Position -= m_Right * velocity;*/
    /*if (Input::IsKeyPressed(Key::D)) m_Position += m_Right * velocity;*/
    /*if (Input::IsKeyPressed(Key::Space)) m_Position += m_Up * velocity;*/
    /*if (Input::IsKeyPressed(Key::LeftControl)) m_Position -= m_Up * velocity;*/
    /**/
    /*UpdateView();*/

    auto* player = ModelManager::GetModel("harry")->GetController();
    glm::vec3 targetPos = PhysX::PxExtendedVec3toGlmVec3(player->getPosition());

    glm::vec3 direction;
    direction.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    direction.y = sin(glm::radians(m_Pitch));
    direction.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    direction = glm::normalize(direction);

    float distance = (m_Mode == CameraMode::ORBITAL) ? m_Distance : m_FollowDistance;

    glm::vec3 desiredPos = targetPos - direction * distance;

    glm::vec3 right = glm::normalize(glm::cross(direction, glm::vec3(0,1,0)));

    desiredPos += right * m_ShoulderOffset;        // horizontal shoulder
    desiredPos += glm::vec3(0, m_HeightOffset, 0); // vertical offset

    /*float t = 1.0f - std::exp(-m_SmoothSpeed * (float)dt);*/
    /*m_Position = glm::mix(m_Position, desiredPos, t);*/
    m_Position = desiredPos;

    glm::vec3 lookTarget = targetPos + glm::vec3(0, 5.0f, 0);

    m_Front = glm::normalize(lookTarget - m_Position);
    m_Right = glm::normalize(glm::cross(m_Front, glm::vec3(0,1,0)));
    m_Up    = glm::cross(m_Right, m_Front);

    UpdateView();
  }
  else if(m_Mode == CameraMode::PLAIN)
  {
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
  if (m_Mode == CameraMode::ORBITAL)
  {
    m_Distance -= delta * ZoomSpeed();

    if (m_Distance < 1.0f)
        m_Distance = 1.0f;
    if (m_Distance > 1000.0f)
        m_Distance = 1000.0f;

    m_Orbital_Distance = m_Distance;

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

  m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
  m_Up = glm::normalize(glm::cross(m_Right, m_Front));
}

glm::vec3 Camera::GetUpDirection()
{
	/*return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));*/
  return m_Up;
}

glm::vec3 Camera::GetRightDirection()
{
	/*return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));*/
  return m_Right;
}

glm::vec3 Camera::GetForwardDirection()
{
	/*return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));*/
  return m_Front;
}

glm::vec3 Camera::CalculatePosition()
{
  glm::vec3 direction;
  direction.x = cos(glm::radians(m_Pitch)) * cos(glm::radians(m_Yaw));
  direction.y = sin(glm::radians(m_Pitch));
  direction.z = cos(glm::radians(m_Pitch)) * sin(glm::radians(m_Yaw));

  return m_FocalPoint + direction * m_Distance;
}

glm::quat Camera::GetOrientation()
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

const glm::vec3& Camera::GetPosition()
{
  return m_Position;
}

float Camera::GetDistance()
{ 
  return m_Distance;
}

void Camera::SetDistance(float distance)
{ 
  m_Distance = distance;
}

const glm::mat4& Camera::GetViewMatrix()
{ 
  return m_ViewMatrix;
}

const glm::mat4& Camera::GetProjection()
{ 
  return m_Projection;
}

glm::mat4 Camera::GetViewProjection()
{ 
  return m_Projection * m_ViewMatrix;
}

glm::mat4 Camera::GetNonRotationViewProjection()
{ 
  return m_Projection * glm::mat4(glm::mat3(m_ViewMatrix));
}

glm::mat4 Camera::GetOrtoProjection()
{ 
  return glm::ortho(0.0f, static_cast<float>(m_ViewportWidth), 0.0f, static_cast<float>(m_ViewportHeight));
}

float Camera::GetPitch()
{ 
  return m_Pitch;
}

float Camera::GetYaw()
{ 
  return m_Yaw;
}
