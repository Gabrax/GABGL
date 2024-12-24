#pragma once

#include "Windowbase.h"
#include "BackendScopeRef.h"

struct Engine
{
	Engine();
	virtual ~Engine();
	void Run();
private:
	Scope<Window> m_Window;
	bool m_isRunning = true;
};