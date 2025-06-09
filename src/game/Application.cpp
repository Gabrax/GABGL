#include "Application.h"

#include "../backend/Renderer.h"
#include "../input/KeyCodes.h"
#include "../input/UserInput.h"
#include "../engine.h"
#include "glm/fwd.hpp"
#include "../backend/Audio.h"

Application::Application() 
{
  m_WindowRef = &Engine::GetInstance().GetMainWindow();
  m_Camera = Camera(45.0f, (float)m_WindowRef->GetWidth() / (float)m_WindowRef->GetHeight(), 0.001f, 2000.0f);
  m_Camera.SetViewportSize((float)m_WindowRef->GetWidth(), (float)m_WindowRef->GetHeight());

  FramebufferSpecification fbSpec;
	fbSpec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
	fbSpec.Width = Engine::GetInstance().GetMainWindow().GetWidth();
	fbSpec.Height = Engine::GetInstance().GetMainWindow().GetHeight();
	m_Framebuffer = Framebuffer::Create(fbSpec);

  AudioSystem::SetListenerVolume(0.1f);
  AudioSystem::PlayMusic("music");
}

void Application::OnUpdate(DeltaTime& dt)
{
	Renderer::ResetStats();
	m_Framebuffer->Bind();
	Renderer::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
	Renderer::Clear();
	m_Framebuffer->ClearAttachment(1, -1);

  Renderer::BeginScene(m_Camera);

    Renderer::DrawLine(glm::vec3(2.0f), glm::vec3(1.0f,1.0f,0.0f), glm::vec4(1.0f));
    Renderer::DrawModel("res/backpack/backpack.obj",glm::vec3(0.0f),glm::vec3(2.0f),90.0f);
    Renderer::DrawCubeContour(glm::vec3(2.0f), glm::vec3(1.0f), glm::vec4(1.0f));
    Renderer::DrawCube({glm::vec3(2.0f,0.0f,0.0f)});
    Renderer::Draw3DText("FPS: " + std::to_string(dt.GetFPS()), glm::vec2(0.0f), 0.01f, glm::vec4(2.0f,1.0f,1.0f,1.0f));
    Renderer::DrawSkybox("night");
    Renderer::Draw2DQuad(glm::vec2(50.0f),glm::vec2(50.0f),45.0f,glm::vec4(2.0f));
    Renderer::Draw2DText("FPS: " + std::to_string(dt.GetFPS()), glm::vec2(100.0f,50.0f), 0.5f, glm::vec4(1.0f,1.0f,3.0f,1.0f));

  Renderer::EndScene();

	m_Framebuffer->Unbind();

  m_Camera.OnUpdate(dt);
  AudioSystem::UpdateAllMusic();

  switch (m_SceneState)
	{
		case SceneState::Edit:
		{
      m_Camera.SetCursor(true);
      m_Editor.OnImGuiRender(m_Framebuffer->GetColorAttachmentRendererID());
			break;
		}
		case SceneState::Play:
		{
      m_Camera.SetCursor(false);
      Renderer::RenderFullscreenFramebufferTexture(m_Framebuffer->GetColorAttachmentRendererID());
			break;
		}
	}
}

void Application::OnEvent(Event& e)
{
	if (m_SceneState == SceneState::Edit) m_Camera.OnEvent(e);

	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT(Application::OnKeyPressed));
	dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT(Application::OnMouseButtonPressed));
}

bool Application::OnKeyPressed(KeyPressedEvent& e)
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
      m_Framebuffer->Resize((uint32_t)m_WindowRef->GetWidth(), (uint32_t)m_WindowRef->GetHeight());
      m_Camera.SetViewportSize((uint32_t)m_WindowRef->GetWidth(), (uint32_t)m_WindowRef->GetHeight());
      AudioSystem::PlaySound("select1");
			break;
		}
		case Key::T:
		{
      m_WindowRef->SetFullscreen(false);
      m_Framebuffer->Resize((uint32_t)m_WindowRef->GetWidth(), (uint32_t)m_WindowRef->GetHeight());
      m_Camera.SetViewportSize((uint32_t)m_WindowRef->GetWidth(), (uint32_t)m_WindowRef->GetHeight());
      AudioSystem::PlaySound("select2");
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

bool Application::OnMouseButtonPressed(MouseButtonPressedEvent& e)
{
	if (e.GetMouseButton() == Mouse::ButtonLeft)
	{
		/*if (m_ViewportHovered && !ImGuizmo::IsOver() && !Input::IsKeyPressed(Key::LeftAlt))*/
		/*	m_SelectionContext = m_HoveredEntity;*/
	}
	return false;
}


