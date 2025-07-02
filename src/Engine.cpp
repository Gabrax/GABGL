#include "engine.h"

#include "backend/BackendLogger.h"
#include "backend/AudioManager.h"
#include "backend/LayerStack.h"
#include "backend/LightManager.h"
#include "backend/Renderer.h"
#include "app/Application.h"
#include "backend/AssetManager.h"
#include "backend/PhysX.h"
#include "backend/FontManager.h"

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

  AudioManager::Init();
  LightManager::Init();
  FontManager::Init();
  PhysX::Init();
	Renderer::Init();

  AssetManager::StartLoadingAssets();

  while(m_Window->IsRunning())
  {
    Renderer::ClearBuffers();

    DeltaTime dt;

    if (!AssetManager::LoadingComplete())
    {
      AssetManager::UpdateLoading();

      Renderer::DrawLoadingScreen();

      if(AssetManager::LoadingComplete())
      {
        Application* m_App = new Application;
        LayerStack::PushLayer(m_App);
      }
    }
    else
    {
      if (!m_Window->IsMinimized()) LayerStack::OnUpdate(dt);
    }

    m_Window->Update();
  }
}


