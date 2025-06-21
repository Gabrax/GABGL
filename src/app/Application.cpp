#include "Application.h"

#include "../backend/Renderer.h"
#include "../input/KeyCodes.h"
#include "../input/UserInput.h"
#include "glm/fwd.hpp"
#include "../backend/AudioManager.h"
#include "../backend/LightManager.h"

Application::Application() 
{
  AudioManager::SetListenerVolume(0.05f);
  AudioManager::PlayMusic("night",true);
  LightManager::AddLight(LightType::POINT, glm::vec4(1.0f), glm::vec3(5.0f), glm::vec3(1.0f), glm::vec3(1.0f));
  /*LightManager::AddLight(LightType::DIRECT, glm::vec4(1.0f), glm::vec3(50.0f), glm::vec3(1.0f), glm::vec3(1.0f));*/
}

void Application::OnUpdate(DeltaTime& dt)
{
  Renderer::RenderScene(dt, [&dt]()
  {
    Renderer::DrawLine(glm::vec3(2.0f), glm::vec3(1.0f,1.0f,0.0f), glm::vec4(1.0f));
    Renderer::DrawCubeContour(glm::vec3(2.0f), glm::vec3(1.0f), glm::vec4(1.0f));
    Renderer::DrawCube({glm::vec3(2.0f,0.0f,0.0f)});

    Renderer::DrawModel(dt,ModelManager::GetModel("objHouse"),glm::vec3(0.0f),glm::vec3(1.0f),glm::vec3(0.0f));
    Renderer::DrawModel(dt,ModelManager::GetModel("MaleSurvivor1"),glm::vec3(0.0f,0.0f,0.0f),glm::vec3(1.0f),glm::vec3(0.0f));
    Renderer::DrawModel(dt,ModelManager::GetModel("Zombie_Idle"),glm::vec3(10.0f,0.0f,0.0f),glm::vec3(5.0f),glm::vec3(0.0f));
    Renderer::DrawModel(dt,ModelManager::GetModel("harry"), glm::vec3(5.0f,0.0f,0.0f), glm::vec3(500.03f), glm::vec3(0.0f,0.0f,0.0f));

    Renderer::DrawSkybox("night");
    Renderer::Draw2DText("FPS: " + std::to_string(dt.GetFPS()), glm::vec2(100.0f,50.0f), 0.5f, glm::vec4(1.0f,1.0f,3.0f,1.0f));

    if (Input::IsKeyPressed(Key::Z))
    {
        ModelManager::GetModel("harry")->StartBlendToAnimation(1, 0.8f); 
    }
    else
    {
        ModelManager::GetModel("harry")->StartBlendToAnimation(0, 0.8f); 
    }
  });
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
      Renderer::SetFullscreen("select1", true);
			break;
		}
		case Key::T:
		{
      Renderer::SetFullscreen("select2", false);
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


