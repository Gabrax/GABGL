#pragma once

#include "../input/EngineEvent.h"
#include "../input/KeyEvent.h"
#include "../backend/DeltaTime.h"

#include <glm/glm.hpp>

struct Camera
{
	Camera() = default;
	Camera(float fov, float aspectRatio, float nearClip, float farClip);
	Camera(const glm::mat4& projection)
		: m_Projection(projection) {}

	~Camera() = default;

	void OnUpdate(DeltaTime dt);
	void OnEvent(Event& e);

	inline float GetDistance() const { return m_Distance; }
	inline void SetDistance(float distance) { m_Distance = distance; }

	inline void SetViewportSize(float width, float height) { m_ViewportWidth = width; m_ViewportHeight = height; UpdateProjection(); }

	const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
	const glm::mat4& GetProjection() const { return m_Projection; }
	glm::mat4 GetViewProjection() const { return m_Projection * m_ViewMatrix; }

	glm::vec3 GetUpDirection() const;
	glm::vec3 GetRightDirection() const;
	glm::vec3 GetForwardDirection() const;
	const glm::vec3& GetPosition() const { return m_Position; }
	glm::quat GetOrientation() const;

	float GetPitch() const { return m_Pitch; }
	float GetYaw() const { return m_Yaw; }
private:
	void UpdateProjection();
	void UpdateView();

	bool OnMouseScroll(MouseScrolledEvent& e);

	void MousePan(const glm::vec2& delta);
	void MouseRotate(const glm::vec2& delta);
	void MouseZoom(float delta);

	glm::vec3 CalculatePosition() const;

	std::pair<float, float> PanSpeed() const;
	float RotationSpeed() const;
	float ZoomSpeed() const;
private:

	glm::mat4 m_Projection = glm::mat4(1.0f);

	float m_FOV = 45.0f, m_AspectRatio = 1.778f, m_NearClip = 0.1f, m_FarClip = 1000.0f;

	glm::mat4 m_ViewMatrix;
	glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
	glm::vec3 m_FocalPoint = { 0.0f, 0.0f, 0.0f };

	glm::vec2 m_InitialMousePosition = { 0.0f, 0.0f };

	float m_Distance = 10.0f;
	float m_Pitch = 0.0f, m_Yaw = 0.0f;

	float m_ViewportWidth = 1280, m_ViewportHeight = 720;
};
