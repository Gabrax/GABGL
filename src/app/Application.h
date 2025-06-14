#pragma once

#include "../backend/LayerStack.h"
#include "../input/Event.h"
#include "../input/KeyEvent.h"

struct Application : Layer
{
  Application();
  virtual ~Application() = default;
  
	void OnEvent(Event& e) override;
	void OnUpdate(DeltaTime& dt) override;

private:
	bool OnKeyPressed(KeyPressedEvent& e);
	bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
};

