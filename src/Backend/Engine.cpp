#include "Engine.h"
#include "BackendLogger.h"

Engine::Engine()
{
	Log::Init();
	m_Window = Scope<Window>(Window::Create());
	Run();
}

Engine::~Engine()
{

}

void Engine::Run()
{
	while (m_isRunning)
	{
		glClear(GL_COLOR_BUFFER_BIT);
		m_Window->Update();
	}
}