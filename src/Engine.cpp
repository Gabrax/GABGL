#include "engine.h"

#include <iostream>

#include "backend/BackendLogger.h"
#include "input/KeyCodes.h"
#include "input/UserInput.h"
#include "backend/RendererAPI.h"

Engine* Engine::s_Instance = nullptr;

Engine::Engine()
{
  s_Instance = this;
  Log::Init();
  Run();
}

Engine::~Engine() = default;

void Engine::Run()
{
  m_Window = WindowBase::Create<Window>({ "GABGL", 1000, 600 });
	m_Window->SetEventCallback(BIND_EVENT(OnEvent));

	RendererAPI::Init();
  while(m_isRunning)
  {

    DeltaTime dt;

    RendererAPI::Clear();

    if(Input::IsKeyPressed(Key::E)) m_Window->SetFullscreen(true);
    if(Input::IsKeyPressed(Key::R)) m_Window->SetFullscreen(false);

    if (!m_Minimized)
		{
			RenderLayers(dt);
			/*RenderEditorLayers();*/
		}

    m_Window->Update();
  }
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
	RendererAPI::OnWindowResize(e.GetWidth(), e.GetHeight());

	return false;
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

/*void Engine::SetupMainWindow()*/
/*{*/
/*	m_MainWindow = Window::Create<MainWindow>({ "GABGL", 1000, 600 });*/
/*	m_MainWindow->SetEventCallback(BIND_EVENT(OnEvent));*/
/*	m_ImGuiLayer = new ImGuiLayer(m_MainWindow.get());*/
/*	PushOverlay(m_ImGuiLayer);*/
/*	m_MainEditorlayer = new MainEditor;*/
/*	PushLayer(m_MainEditorlayer);*/
/*}*/

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

