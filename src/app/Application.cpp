#include "Application.h"

#include "../backend/Renderer.h"
#include "../input/KeyCodes.h"
#include "../input/UserInput.h"
#include "glm/fwd.hpp"
#include "../backend/Audio.h"

Application::Application() 
{
  AudioSystem::SetListenerVolume(0.1f);
  AudioSystem::PlayMusic("menu");
}

void Application::OnUpdate(DeltaTime& dt)
{
  Renderer::RenderScene(dt,
    [&dt]()
    {
       Renderer::DrawLine(glm::vec3(2.0f), glm::vec3(1.0f,1.0f,0.0f), glm::vec4(1.0f));
       Renderer::DrawCubeContour(glm::vec3(2.0f), glm::vec3(1.0f), glm::vec4(1.0f));
       Renderer::DrawCube({glm::vec3(2.0f,0.0f,0.0f)});

       Renderer::DrawModel(dt,"objHouse",glm::vec3(0.0f),glm::vec3(1.0f),90.0f);
       Renderer::DrawModel(dt,"MaleSurvivor1",glm::vec3(0.0f,0.0f,0.0f),glm::vec3(1.0f),90.0f);

       Renderer::DrawSkybox("night");
       Renderer::Draw2DText("FPS: " + std::to_string(dt.GetFPS()), glm::vec2(100.0f,50.0f), 0.5f, glm::vec4(1.0f,1.0f,3.0f,1.0f));
    }
  );
}

void Application::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT(Application::OnKeyPressed));
	dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT(Application::OnMouseButtonPressed));
}

bool Application::OnKeyPressed(KeyPressedEvent& e)
{
	// Shortcuts
	if (e.IsRepeat())
		return false;

	switch (e.GetKeyCode())
	{
		case Key::R:
		{
      Renderer::SetFullscreen("sound1", true);
			break;
		}
		case Key::T:
		{
      Renderer::SetFullscreen("sound2", false);
			break;
		}
	}

	return false;
}

bool Application::OnMouseButtonPressed(MouseButtonPressedEvent& e)
{
	/*if (e.GetMouseButton() == Mouse::ButtonLeft)*/
	/*{*/
	/*	if (m_ViewportHovered && !ImGuizmo::IsOver() && !Input::IsKeyPressed(Key::LeftAlt))*/
	/*		m_SelectionContext = m_HoveredEntity;*/
	/*}*/
	return false;
}


