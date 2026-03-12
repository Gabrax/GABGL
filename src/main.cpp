#include "backend/ModelManager.h"
#include "backend/Logger.h"
#include "backend/AudioManager.h"
#include "backend/LightManager.h"
#include "backend/Renderer.h"
#include "backend/PhysX.h"
#include "backend/FontManager.h"
#include "backend/Settings.h"
#include "backend/SceneManager.h"
#include "backend/Window.h"

#include <thread>
#include <chrono>

// Prevent accidentally selecting integrated GPU
extern "C" {
	__declspec(dllexport) unsigned __int32 AmdPowerXpressRequestHighPerformance = 0x1;
	__declspec(dllexport) unsigned __int32 NvOptimusEnablement = 0x1;
}

int main()
{
  Logger::Init();
  Settings::Init();
  Window::Init("GABGL", Settings::GetWindowWidth(), Settings::GetWindowHeight());
  AudioManager::Init();
  LightManager::Init();
  FontManager::Init();
  PhysX::Init();
	Renderer::Init();
  ModelManager::Init();

  SceneManager::LoadScene("game");

  while (Window::IsRunning())
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    DeltaTime dt;

    SceneManager::Update(dt);

    std::this_thread::sleep_for(std::chrono::milliseconds(Settings::GetFPS()));

    Window::Update();
  }
}
