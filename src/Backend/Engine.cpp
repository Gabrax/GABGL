#include "Engine.h"
#include "BackendLogger.h"
#include "MainWindow.h"
#include "StartWindow.h"
#include "../Editor/SCEditor.h"

Engine* Engine::s_Instance = nullptr;

Engine::Engine()
{
	GABGL_ASSERT(!s_Instance, "Instance already exists!");
	s_Instance = this;
	Log::Init();
	Run();
}

Engine::~Engine()
{

}

void Engine::Run()
{
    m_StartWindow = Window::Create<StartWindow>({ "GABGL", 600, 300 });
    m_StartWindow->SetEventCallback(BIND_EVENT(OnEvent));

    m_ImGuiLayer = new ImGuiLayer(m_StartWindow.get());
    PushOverlay(m_ImGuiLayer);
	EditorLayer* editlayer = new EditorLayer;
	PushLayer(editlayer);

    while (m_isRunning)
    {
        float time = (float)glfwGetTime();
		DeltaTime deltatime = time - m_LastFrameTime;
        m_LastFrameTime = time;

		glClear(GL_COLOR_BUFFER_BIT);

        if (!StartWindow::isClosed())
        {
			if (!m_Minimized)
			{	
				m_ImGuiLayer->Begin();
				{
					for (Layer* layer : m_LayerStack)
						layer->OnImGuiRender();
				}
				m_ImGuiLayer->End();
			}
            m_StartWindow->Update();
        }
        else
        {
            if (!m_MainWindow) // Only create MainWindow once
            {
                m_MainWindow = Window::Create<MainWindow>({ "Main Window", 1000, 600 });
                m_MainWindow->SetEventCallback(BIND_EVENT(OnEvent));
				m_ImGuiLayer = new ImGuiLayer(m_MainWindow.get());
				PushOverlay(m_ImGuiLayer);
            }

			if (!m_Minimized)
			{
				for (Layer* layer : m_LayerStack)
					layer->OnUpdate(deltatime);

				m_ImGuiLayer->Begin();
				{
					for (Layer* layer : m_LayerStack)
						layer->OnImGuiRender();
				}
				m_ImGuiLayer->End();
			}

            m_MainWindow->Update();

            if (StartWindow::isClosed())
            {
				m_LayerStack.PopOverlay(m_ImGuiLayer);
				m_LayerStack.PopLayer(editlayer);
                m_StartWindow.reset();
            }
        }
    }
}

void Engine::PushLayer(Layer* layer)
{
	m_LayerStack.PushLayer(layer);
	layer->OnAttach();
}

void Engine::PushOverlay(Layer* layer)
{
	m_LayerStack.PushOverlay(layer);
	layer->OnAttach();
}

void Engine::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT(OnWindowClose));
	dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT(OnWindowResize));

	for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
	{
		if (e.Handled)
			break;
		(*it)->OnEvent(e);
	}
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