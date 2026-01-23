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

  /*LightManager::AddLight(LightType::SPOT, glm::vec3(1.0f,1.0f,1.0), glm::vec3(15.0f,15.0f,15.0f), glm::vec3(0.0,-1.0,0.0)); */

  Transform transform;
  transform.SetPosition(glm::vec3(5.0f));
  /*ModelManager::SetInitialModelTransform("sphere", transform.GetTransform());*/
  LightManager::AddLight(LightType::POINT, glm::vec3(1.0f,1.0f,0.0), transform.GetPosition(), glm::vec3(1.0f));
  LightManager::AddLight(LightType::DIRECT, glm::vec3(0.3, 0.32, 0.4), glm::vec3(0.0f), glm::vec3(-2.0f, -4.0f, -1.0f));
  Transform terraintransform;
  terraintransform.SetPosition(glm::vec3(-200.0f,-200.0f,50.0f));
  ModelManager::SetInitialModelTransform("outer_terrain", terraintransform.GetTransform());
  Transform housetransform;
  housetransform.SetPosition(glm::vec3(0.0f));
  ModelManager::SetInitialModelTransform("objHouse", housetransform.GetTransform());
  Transform pistoltransform;
  pistoltransform.SetPosition(glm::vec3(15.0f,4.0f,13.0f));
  ModelManager::SetInitialModelTransform("pistol", pistoltransform.GetTransform());
  ModelManager::SetRender("pistol_convex", false);
  Transform pistolammotransform;
  pistolammotransform.SetPosition(glm::vec3(11.0f,4.0f,5.0f));
  ModelManager::SetInitialModelTransform("pistolammo", pistolammotransform.GetTransform());
  ModelManager::SetRender("pistolammo_convex", false);
  Transform shotguntransform;
  shotguntransform.SetPosition(glm::vec3(14.0f,4.0f,11.0f));
  ModelManager::SetInitialModelTransform("shotgun", shotguntransform.GetTransform());
  ModelManager::SetRender("shotgun_convex", false);
  Transform shotgunammotransform;
  shotgunammotransform.SetPosition(glm::vec3(12.0f,4.0f,7.0f));
  ModelManager::SetInitialModelTransform("shotgunammo", shotgunammotransform.GetTransform());
  ModelManager::SetRender("shotgunammo_convex", false);
  Transform aidkittransform;
  aidkittransform.SetPosition(glm::vec3(13.0f,4.0f,9.0f));
  ModelManager::SetInitialModelTransform("aidkit", aidkittransform.GetTransform());
  ModelManager::SetRender("aidkit_convex", false);
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
    [&dt](){
  
      if (Input::IsKeyPressed(Key::X))
      {
          ModelManager::GetModel("harry")->StartBlendToAnimation(1, 0.8f);
          /*ModelManager::MoveController("harry", Movement::FORWARD,5.0f,dt);*/
      }
      else
      {
          ModelManager::GetModel("harry")->StartBlendToAnimation(0, 0.8f); 
      }
      if (Input::IsKeyPressed(Key::C))
      {
          ModelManager::MoveController("harry", Movement::BACKWARD,5.0f,dt);
      }
      if (Input::IsKeyPressed(Key::Z))
      {
          ModelManager::MoveController("harry", Movement::LEFT,5.0f,dt);
      }
      if (Input::IsKeyPressed(Key::V))
      {
          ModelManager::MoveController("harry", Movement::RIGHT,5.0f,dt);
      }

    },[&dt](){}
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


