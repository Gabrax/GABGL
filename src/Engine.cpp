#include "engine.h"

#include "backend/Logger.h"
#include "backend/AudioManager.h"
#include "backend/LightManager.h"
#include "backend/ModelManager.h"
#include "backend/Renderer.h"
#include "backend/PhysX.h"
#include "backend/FontManager.h"
#include "backend/CustomFrameRate.hpp"
#include "backend/Config.h"
#include "backend/SceneManager.h"

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

  Config config;

  m_Window = Window::Create({ "GABGL", config.GetWindowWidth(), config.GetWindowHeight() });

  AudioManager::Init();
  LightManager::Init();
  FontManager::Init();
  PhysX::Init();
	Renderer::Init();
  ModelManager::Init();

  SceneManager::LoadScene();

  CustomFrameRate::SetTargetFPS(20);

  while (m_Window->IsRunning())
  {
    CustomFrameRate::BeginFrame();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    DeltaTime dt;

    SceneManager::Update(dt);

    m_Window->Update();
    CustomFrameRate::EndFrame();
  }
}

