#pragma once

#include <memory>

#include "backend/window.h"
#include "input/EngineEvent.h"

struct Engine
{
  Engine(); 
	virtual ~Engine();
	void Run();
  void OnEvent(Event& e);
	inline static Engine& GetInstance() { return *s_Instance; }
	inline Window& GetMainWindow() { return *m_Window; }

private:
  std::unique_ptr<Window> m_Window;
	static Engine* s_Instance;
};
