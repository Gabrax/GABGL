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
  bool OnWindowClose(WindowCloseEvent& e);
	bool OnWindowResize(WindowResizeEvent& e);

	bool m_Minimized = false;
	bool m_isRunning = true;
	bool m_closed = false;
private:
	static Engine* s_Instance;
};
