#include "engine.h"

#include "backend/Logger.h"
#include "backend/AudioManager.h"
#include "backend/LightManager.h"
#include "backend/ModelManager.h"
#include "backend/Renderer.h"
#include "backend/PhysX.h"
#include "backend/FontManager.h"
#include "backend/Settings.h"
#include "backend/SceneManager.h"

#include <thread>
#include <chrono>

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
  Settings::Init();

  m_Window = Window::Create({ "GABGL", Settings::GetWindowWidth(), Settings::GetWindowHeight() });

  AudioManager::Init();
  LightManager::Init();
  FontManager::Init();
  PhysX::Init();
	Renderer::Init();
  ModelManager::Init();

  SceneManager::LoadScene("game");

  while (m_Window->IsRunning())
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    DeltaTime dt;

    SceneManager::Update(dt);

    std::this_thread::sleep_for(std::chrono::milliseconds(Settings::GetFPS()));

    m_Window->Update();
  }
}

