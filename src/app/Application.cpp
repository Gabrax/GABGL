#include "Application.h"

#include "../backend/Renderer.h"
#include "../input/KeyCodes.h"
#include "../input/UserInput.h"
#include "glm/fwd.hpp"
#include "../backend/AudioManager.h"
#include "../backend/LightManager.h"
#include "../backend/ModelManager.h"

Application::Application() 
{
  AudioManager::SetListenerVolume(0.05f);
  AudioManager::PlayMusic("night",true);
  LightManager::AddLight(LightType::POINT, glm::vec4(1.0f,1.0f,0.0,1.0f), glm::vec3(5.0f), glm::vec3(1.0f), glm::vec3(1.0f));
  LightManager::AddLight(LightType::POINT, glm::vec4(1.0f,1.0f,1.0,1.0f), glm::vec3(15.0f,5.0f,15.0f), glm::vec3(1.0f), glm::vec3(1.0f)); 
  /*LightManager::AddLight(LightType::DIRECT, glm::vec4(1.0f), glm::vec3(0.0f), glm::vec3(-2.0f, -4.0f, -1.0f), glm::vec3(1.0f));*/

  Transform housetransform;
  housetransform.SetPosition(glm::vec3(0.0f));
  ModelManager::SetInitialModelTransform("objHouse", housetransform.GetTransform());
  Transform pistoltransform;
  pistoltransform.SetPosition(glm::vec3(15.0f,4.0f,13.0f));
  ModelManager::SetInitialModelTransform("pistol", pistoltransform.GetTransform());
  Transform pistolammotransform;
  pistolammotransform.SetPosition(glm::vec3(11.0f,4.0f,5.0f));
  ModelManager::SetInitialModelTransform("pistolammo", pistolammotransform.GetTransform());
  Transform shotguntransform;
  shotguntransform.SetPosition(glm::vec3(14.0f,4.0f,11.0f));
  ModelManager::SetInitialModelTransform("shotgun", shotguntransform.GetTransform());
  Transform shotgunammotransform;
  shotgunammotransform.SetPosition(glm::vec3(12.0f,4.0f,7.0f));
  ModelManager::SetInitialModelTransform("shotgunammo", shotgunammotransform.GetTransform());
  Transform aidkittransform;
  aidkittransform.SetPosition(glm::vec3(13.0f,4.0f,9.0f));
  ModelManager::SetInitialModelTransform("aidkit", aidkittransform.GetTransform());
  Transform harrytransform;
  harrytransform.SetPosition(glm::vec3(5.0f,0.0f,0.0f));
  ModelManager::SetInitialControllerTransform("harry", harrytransform,1.0f,1.0f,true);
  Transform zombietransform;
  zombietransform.SetPosition(glm::vec3(10.0f,0.0f,0.0f));
  ModelManager::SetInitialControllerTransform("zombie", zombietransform,1.0f,1.0f,true);
}

void Application::OnUpdate(DeltaTime& dt)
{
  Renderer::DrawScene(dt, 
    [&dt]()
    {
    },
    [&dt]()
    {
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
      Renderer::SetFullscreen("select1", true);
			break;
		}
		case Key::T:
		{
      Renderer::SetFullscreen("select2", false);
			break;
		}
    case Key::Tab:
    {
      Renderer::SwitchRenderState();
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


