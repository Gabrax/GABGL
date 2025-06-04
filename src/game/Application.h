#pragma once

#include "../backend/Layer.h"
#include "../input/Event.h"
#include "../input/KeyEvent.h"
#include "../backend/Camera.h"
#include "../backend/windowbase.h"
#include "../backend/FrameBuffer.h"
#include "Editor.h"

struct Application : Layer
{
  Application();
  virtual ~Application() = default;
  
	void OnEvent(Event& e) override;
	void OnUpdate(DeltaTime dt) override;
  inline std::shared_ptr<Framebuffer> GetFramebuffer() const { return m_Framebuffer; }

private:
	bool OnKeyPressed(KeyPressedEvent& e);
	bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
private:

  enum class SceneState
	{
		Edit = 0, Play = 1
	};
	SceneState m_SceneState = SceneState::Edit;

  bool m_ViewportFocused = false, m_ViewportHovered = false;
	glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
	glm::vec2 m_ViewportBounds[2];

  uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
  Editor m_Editor;
	std::shared_ptr<Framebuffer> m_Framebuffer;
  Camera m_Camera;
  WindowBase* m_WindowRef = nullptr;
};

