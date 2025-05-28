#include "game.h"

#include "../backend/Renderer2D.h"
#include "../input/KeyCodes.h"
#include "../input/UserInput.h"
#include "../engine.h"

void GAME::OnAttach()
{
  m_WindowRef = &Engine::GetInstance().GetMainWindow();
  m_Camera = Camera(45.0f, (float)m_WindowRef->GetWidth() / (float)m_WindowRef->GetHeight(), 0.1f, 1000.0f);
  m_Camera.SetViewportSize((float)m_WindowRef->GetWidth(), (float)m_WindowRef->GetHeight());
  m_Camera.SetOrthographic(10.0f, 0.1f, 1000.0f);
  m_Camera.SetProjectionType(Camera::ProjectionType::Perspective);
}

void GAME::OnDetach()
{

}

void GAME::OnEvent(Event& e)
{

}

void GAME::OnUpdate(DeltaTime dt)
{
  if(Input::IsKeyPressed(Key::E)) m_WindowRef->SetFullscreen(true);
  if(Input::IsKeyPressed(Key::R)) m_WindowRef->SetFullscreen(false);

  m_Camera.OnUpdate(dt);
  Renderer2D::BeginScene(m_Camera);
  
  Renderer2D::DrawQuad(glm::vec2(0.0f,-2.0f),glm::vec2(1.0f),glm::vec4(2.0f));
  Renderer2D::DrawText("HEHE", glm::vec2(0.0f), 0.01f, glm::vec4(3.0f,1.0f,1.0f,1.0f));

  Renderer2D::EndScene();
}

