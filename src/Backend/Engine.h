#pragma once

#include "Windowbase.h"
#include "BackendScopeRef.h"
#include "../Input/ApplicationEvent.h"

struct Engine
{
	Engine();
	virtual ~Engine();
	void Run();
	void OnEvent(Event& e);
private:
	bool OnWindowClose(WindowCloseEvent& e);
	bool OnWindowResize(WindowResizeEvent& e);
	Scope<Window> m_StartWindow;
	Scope<Window> m_MainWindow;
	bool m_Minimized = false;
	bool m_isRunning = true;
};