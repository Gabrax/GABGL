#include "Camera.h"

#include "../backend/BackendLogger.h"
#include "../input/UserInput.h"
#include "../input/KeyEvent.h"

#include <glfw/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

Camera::Camera(float fov, float aspectRatio, float nearClip, float farClip)
    : m_ProjectionType(ProjectionType::Perspective),
      m_PerspectiveFOV(glm::radians(fov)),
      m_AspectRatio(aspectRatio),
      m_PerspectiveNear(nearClip),
      m_PerspectiveFar(farClip),
      m_Distance(10.0f),
      m_Pitch(0.0f),
      m_Yaw(0.0f)
{
    RecalculateProjection();
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
    m_Position = CalculatePosition();

    glm::quat orientation = GetOrientation();
    m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
    m_ViewMatrix = glm::inverse(m_ViewMatrix);
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
	if (Input::IsKeyPressed(Key::LeftAlt))
	{
		const glm::vec2& mouse{ Input::GetMouseX(), Input::GetMouseY() };
		glm::vec2 delta = (mouse - m_InitialMousePosition) * 0.003f;
		m_InitialMousePosition = mouse;

		if (Input::IsMouseButtonPressed(Mouse::ButtonMiddle))
			MousePan(delta);
		else if (Input::IsMouseButtonPressed(Mouse::ButtonLeft))
			MouseRotate(delta);
		else if (Input::IsMouseButtonPressed(Mouse::ButtonRight))
			MouseZoom(delta.y);
	}

	UpdateView();
}

void Camera::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<MouseScrolledEvent>(BIND_EVENT(Camera::OnMouseScroll));
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

void Camera::MouseRotate(const glm::vec2& delta)
{
    float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
    m_Yaw += yawSign * delta.x * RotationSpeed();
    m_Pitch += delta.y * RotationSpeed();

    // Clamp pitch to avoid flipping over
    const float pitchLimit = glm::radians(89.0f);
    m_Pitch = glm::clamp(m_Pitch, -pitchLimit, pitchLimit);
}

void Camera::MouseZoom(float delta)
{
	m_Distance -= delta * ZoomSpeed();
	if (m_Distance < 1.0f)
	{
		m_FocalPoint += GetForwardDirection();
		m_Distance = 1.0f;
	}
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
	return m_FocalPoint - GetForwardDirection() * m_Distance;
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

        m_Projection = glm::ortho(orthoLeft, orthoRight,
            orthoBottom, orthoTop, m_OrthographicNear, m_OrthographicFar);
    }
}
