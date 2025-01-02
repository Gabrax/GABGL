#include "Engine.h"
#include "Backend/BackendLogger.h"
#include "Backend/MainWindow.h"
#include "Backend/StartWindow.h"
#include "Renderer/Renderer.h"

Engine* Engine::s_Instance = nullptr;

Engine::Engine()
{
	s_Instance = this;
	Log::Init();
	Run();
}

Engine::~Engine()
{
	
}

void Engine::Run()
{
	SetupStartWindow();

    while (m_isRunning)
    {
		GABGL_PROFILE_SCOPE("Main Loop");

		DeltaTime dt;

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (!m_StartWindow->isClosed())
        {
			if (!m_Minimized)
			{	
				RenderEditorLayers();
			}

            m_StartWindow->Update();
        }
        else
        {
			if (m_StartWindow->isClosed() && !closed)
			{
				CleanupStartWindow();
			}

            if (!m_MainWindow) 
            {
				SetupMainWindow();
            }

			if (!m_Minimized)
			{
				RenderLayers(dt);
				RenderEditorLayers();
			}

            m_MainWindow->Update();

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
	Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());

	return false;
}

void Engine::SetupStartWindow()
{
	m_StartWindow = Window::Create<StartWindow>({ "GABGL", 600, 300 });
	m_StartWindow->SetEventCallback(BIND_EVENT(OnEvent));

	m_ImGuiLayer = new ImGuiLayer(m_StartWindow.get());
	PushOverlay(m_ImGuiLayer);
	m_StartEditorlayer = new StartEditor;
	PushLayer(m_StartEditorlayer);
}

void Engine::SetupMainWindow()
{
	m_MainWindow = Window::Create<MainWindow>({ "GABGL - " + GetCurrentProject(), 1000, 600 });
	m_MainWindow->SetEventCallback(BIND_EVENT(OnEvent));
	Renderer::Init();
	m_ImGuiLayer = new ImGuiLayer(m_MainWindow.get());
	PushOverlay(m_ImGuiLayer);
	m_MainEditorlayer = new MainEditor;
	PushLayer(m_MainEditorlayer);
	m_RendererLayer = new Renderer2D;
	PushLayer(m_RendererLayer);
}

void Engine::RenderLayers(DeltaTime& dt)
{
	for (Layer* layer : m_LayerStack)
		layer->OnUpdate(dt);
}

void Engine::RenderEditorLayers()
{
	m_ImGuiLayer->Begin();
	{
		for (Layer* layer : m_LayerStack)
			layer->OnImGuiRender();
	}
	m_ImGuiLayer->End();
}

void Engine::CleanupStartWindow()
{
	closed = true;
	m_LayerStack.PopOverlay(m_ImGuiLayer);
	delete m_ImGuiLayer;
	m_ImGuiLayer = nullptr;

	m_LayerStack.PopLayer(m_StartEditorlayer);
	delete m_StartEditorlayer;
	m_StartEditorlayer = nullptr;
}
