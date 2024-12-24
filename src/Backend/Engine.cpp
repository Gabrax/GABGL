#include "Engine.h"
#include "BackendLogger.h"
#include "MainWindow.h"
#include "StartWindow.h"

#define BIND_EVENT(x) std::bind(&Engine::x,this,std::placeholders::_1)

Engine::Engine()
{
	Log::Init();
	Run();
}

Engine::~Engine()
{

}


void Engine::Run()
{
    m_StartWindow = Window::Create<StartWindow>({ "GABGL", 800, 600 });
    m_StartWindow->SetEventCallback(BIND_EVENT(OnEvent));

    while (m_isRunning)
    {
        if (!StartWindow::isClosed())
        {
            m_StartWindow->Update();
        }
        else
        {
            if (!m_MainWindow) // Only create MainWindow once
            {
                m_MainWindow = Window::Create<MainWindow>({ "Main Window", 1000, 600 });
                m_MainWindow->SetEventCallback(BIND_EVENT(OnEvent));
            }

            m_MainWindow->Update();

            if (StartWindow::isClosed())
            {
                m_StartWindow.reset();
            }
        }
    }
}

void Engine::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT(OnWindowClose));
}

bool Engine::OnWindowClose(WindowCloseEvent& e)
{
	m_isRunning = false;
	return true;
}

bool Engine::OnWindowResize(WindowResizeEvent& e)
{

	if (e.GetWidth() == 0 || e.GetHeight() == 0)
	{
		m_Minimized = true;
		return false;
	}

	m_Minimized = false;
	//Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());

	return false;
}