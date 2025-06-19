#include "engine.h"

#include "backend/BackendLogger.h"
#include "backend/Audio.h"
#include "backend/LayerStack.h"
#include "backend/Renderer.h"
#include "app/Application.h"
#include "backend/AssetManager.h"
#include "backend/PhysX.h"

Engine* Engine::s_Instance = nullptr;

Engine::Engine()
{
  s_Instance = this;
  Run();
}

Engine::~Engine() = default;

void Engine::Run()
{
  Logger::Init();

  m_Window = Window::Create({ "GABGL", 1000, 600 });

  AudioSystem::Init();
  PhysX::Init();
	Renderer::Init();
	AssetManager::LoadAssets();

  Application* m_App = new Application;
  LayerStack::PushLayer(m_App);

  while(m_Window->IsRunning())
  {
    DeltaTime dt;

    Renderer::Clear();

    if (!m_Window->IsMinimized()) LayerStack::OnUpdate(dt);

    m_Window->Update();
  }
}


