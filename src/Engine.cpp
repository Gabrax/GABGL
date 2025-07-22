#include "engine.h"

#include "backend/BackendLogger.h"
#include "backend/AudioManager.h"
#include "backend/LayerStack.h"
#include "backend/LightManager.h"
#include "backend/ModelManager.h"
#include "backend/Renderer.h"
#include "app/Application.h"
#include "backend/AssetManager.h"
#include "backend/PhysX.h"
#include "backend/FontManager.h"
#include "backend/CustomFrameRate.hpp"

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
  ModelManager::Init();
  AssetManager::Init();

  /*CustomFrameRate::SetTargetFPS(60);*/

  while(m_Window->IsRunning())
  {
    /*CustomFrameRate::BeginFrame();*/

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    DeltaTime dt;

    if (!AssetManager::LoadingComplete())
    {
      AssetManager::Update();

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

    /*CustomFrameRate::EndFrame();*/
  }
}


