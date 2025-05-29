#include "game.h"

#include "../backend/Renderer2D.h"
#include "../input/KeyCodes.h"
#include "../input/UserInput.h"
#include "../engine.h"
#include "../backend/RendererAPI.h"

void GAME::OnAttach()
{
  m_WindowRef = &Engine::GetInstance().GetMainWindow();
  m_Camera = Camera(45.0f, (float)m_WindowRef->GetWidth() / (float)m_WindowRef->GetHeight(), 0.1f, 1000.0f);
  m_Camera.SetViewportSize((float)m_WindowRef->GetWidth(), (float)m_WindowRef->GetHeight());
  m_Camera.SetOrthographic(10.0f, 0.1f, 1000.0f);
  m_Camera.SetProjectionType(Camera::ProjectionType::Perspective);

  FramebufferSpecification fbSpec;
	fbSpec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
	fbSpec.Width = Engine::GetInstance().GetMainWindow().GetWidth();
	fbSpec.Height = Engine::GetInstance().GetMainWindow().GetHeight();
	m_Framebuffer = Framebuffer::Create(fbSpec);
}

void GAME::OnDetach()
{

}

void GAME::OnEvent(Event& e)
{
	if (m_SceneState == SceneState::Edit)
	{
		m_Camera.OnEvent(e);
	}

	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT(GAME::OnKeyPressed));
	dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT(GAME::OnMouseButtonPressed));
}

void GAME::OnUpdate(DeltaTime dt)
{
  if (FramebufferSpecification spec = m_Framebuffer->GetSpecification();
		m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f && // zero sized framebuffer is invalid
		(spec.Width != m_ViewportSize.x || spec.Height != m_ViewportSize.y))
	{
		m_Framebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		//m_CameraController.OnResize(m_ViewportSize.x, m_ViewportSize.y);
		m_Camera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
	}
	Renderer2D::ResetStats();
	m_Framebuffer->Bind();
	RendererAPI::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
	RendererAPI::Clear();
	m_Framebuffer->ClearAttachment(1, -1);

  Renderer2D::BeginScene(m_Camera);
  
  Renderer2D::DrawQuad(glm::vec2(0.0f,-2.0f),glm::vec2(1.0f),glm::vec4(2.0f));
  Renderer2D::DrawText("HEHE", glm::vec2(0.0f), 0.01f, glm::vec4(2.0f,1.0f,1.0f,1.0f));

  Renderer2D::EndScene();

	m_Framebuffer->Unbind();

  switch (m_SceneState)
	{
		case SceneState::Edit:
		{
      m_Camera.OnUpdate(dt);

      m_Editor.OnImGuiRender(m_Framebuffer);
			break;
		}
		case SceneState::Play:
		{
      m_Camera.OnUpdate(dt);

      Renderer2D::RenderFullscreenFramebufferTexture(m_Framebuffer->GetColorAttachmentRendererID());
			break;
		}
	}
}

bool GAME::OnKeyPressed(KeyPressedEvent& e)
{
	// Shortcuts
	if (e.IsRepeat())
		return false;

	bool control = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
	bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);

	switch (e.GetKeyCode())
	{
		case Key::R:
		{
      m_WindowRef->SetFullscreen(true);
			break;
		}
		case Key::T:
		{
      m_WindowRef->SetFullscreen(false);
			break;
		}
		case Key::Q:
		{
	    m_SceneState = SceneState::Edit;
			break;
		}
		case Key::E:
		{
	    m_SceneState = SceneState::Play;
			break;
		}
  }

	return false;
}

bool GAME::OnMouseButtonPressed(MouseButtonPressedEvent& e)
{
	if (e.GetMouseButton() == Mouse::ButtonLeft)
	{
		/*if (m_ViewportHovered && !ImGuizmo::IsOver() && !Input::IsKeyPressed(Key::LeftAlt))*/
		/*	m_SelectionContext = m_HoveredEntity;*/
	}
	return false;
}


