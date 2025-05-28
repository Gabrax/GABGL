#pragma once

#include "../backend/Layer.h"
#include "../input/Event.h"
#include "../input/EngineEvent.h"
#include "../input/KeyEvent.h"
#include "../backend/Camera.h"
#include "../backend/windowbase.h"

struct GAME : Layer
{
  GAME() : Layer("Game") {};
  virtual ~GAME() = default;
  
  void OnAttach() override;
	void OnDetach() override;
	void OnEvent(Event& e) override;
	void OnUpdate(DeltaTime dt) override;
private:
	bool OnKeyPressed(KeyPressedEvent& e);
	bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
private:
  Camera m_Camera;
  WindowBase* m_WindowRef = nullptr;
};

