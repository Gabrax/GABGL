#include "engine.h"

#include "backend/BackendLogger.h"
#include "backend/Audio.h"
#include "backend/LayerStack.h"
#include "backend/Renderer.h"
#include "game/Application.h"
#include "game/AssetManager.h"
#include "backend/PhysX.h"

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

  AudioSystem::Init();
  PhysX::Init();
	Renderer::Init();
  AssetManager::LoadAssets();

  Application* m_Game = new Application;
  LayerStack::PushLayer(m_Game);

  while(m_isRunning)
  {
    DeltaTime dt;

    Renderer::Clear();

    if (!m_Minimized)
		{
      LayerStack::OnUpdate(dt);
		}

    m_Window->Update();
  }
}

void Engine::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT(OnWindowClose));
	dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT(OnWindowResize));

  auto& m_LayerStack = LayerStack::GetLayers();
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

