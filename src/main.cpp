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
#include "backend/DirectX12Renderer.h"

#include <thread>
#include <chrono>
#include <string_view>

// Prevent accidentally selecting integrated GPU
extern "C" {
	__declspec(dllexport) unsigned __int32 AmdPowerXpressRequestHighPerformance = 0x1;
	__declspec(dllexport) unsigned __int32 NvOptimusEnablement = 0x1;
}

namespace
{
  GraphicsAPI ParseGraphicsAPI(int argc, char** argv)
  {
    GraphicsAPI api = Settings::GetGraphicsAPI();
    for (int i = 1; i < argc; ++i)
    {
      const std::string_view argument(argv[i]);
      if (argument == "--dx12" || argument == "--renderer=dx12")
        api = GraphicsAPI::DirectX12;
      else if (argument == "--opengl" || argument == "--renderer=opengl")
        api = GraphicsAPI::OpenGL;
      else if (argument == "--renderer" && i + 1 < argc)
      {
        const std::string_view value(argv[++i]);
        if (value == "dx12" || value == "directx12") api = GraphicsAPI::DirectX12;
        else if (value == "opengl") api = GraphicsAPI::OpenGL;
      }
    }
    return api;
  }

  void LimitFrameRate(const std::chrono::steady_clock::time_point frameStart)
  {
    if (const uint32_t fpsLimit = Settings::GetFPSLimit(); !Settings::GetVSync() && fpsLimit > 0)
    {
      const auto targetFrameTime = std::chrono::duration<double>(1.0 / static_cast<double>(fpsLimit));
      if (const auto elapsed = std::chrono::steady_clock::now() - frameStart; elapsed < targetFrameTime)
        std::this_thread::sleep_for(targetFrameTime - elapsed);
    }
  }
}

int main(int argc, char** argv)
{
  Logger::Init();
  Settings::Init();
  const GraphicsAPI graphicsAPI = ParseGraphicsAPI(argc, argv);
  GraphicsAPIState::Set(graphicsAPI);
  GABGL_INFO("Selected graphics API: {}", GraphicsAPIName(graphicsAPI));

  if (graphicsAPI == GraphicsAPI::DirectX12)
  {
#ifndef GABGL_ENABLE_DX12
    GABGL_ERROR("This build does not include DirectX 12. Configure with -DGABGL_ENABLE_DX12=ON.");
    return 1;
#else
    Window::Init("GABGL - DirectX 12", Settings::GetWindowWidth(), Settings::GetWindowHeight(), graphicsAPI);
    if (!DirectX12Renderer::Init(Window::GetNativeHandle(), Window::GetWidth(), Window::GetHeight()))
    {
      Window::Terminate();
      return 1;
    }

    AudioManager::Init();
    LightManager::Init();
    PhysX::Init();
    Renderer::Init();
    ModelManager::Init();

    AudioManager::SetMusicVolume(Settings::GetMusicVolume());
    AudioManager::SetSFXVolume(Settings::GetSFXVolume());
    Renderer::ApplyDisplaySettings();
    SceneManager::LoadScene("menu");

    while (Window::IsRunning())
    {
      const auto frameStart = std::chrono::steady_clock::now();
      Window::Update();

      if (Window::IsMinimized())
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        continue;
      }

      if (!DirectX12Renderer::Resize(Window::GetWidth(), Window::GetHeight()) ||
          !DirectX12Renderer::BeginFrame())
      {
        Window::RequestClose();
        continue;
      }

      DeltaTime dt;
      SceneManager::Update(dt);

      if (!DirectX12Renderer::EndFrame(Settings::GetVSync()))
        Window::RequestClose();
      LimitFrameRate(frameStart);
    }

    SceneManager::Shutdown();
    AudioManager::Terminate();
    ModelManager::Shutdown();
    LightManager::Shutdown();
    Renderer::Shutdown();
    PhysX::Shutdown();
    DirectX12Renderer::Shutdown();
    Window::Terminate();
    return 0;
#endif
  }

  Window::Init("GABGL", Settings::GetWindowWidth(), Settings::GetWindowHeight(), graphicsAPI);
  AudioManager::Init();
  LightManager::Init();
  FontManager::Init();
  PhysX::Init();
  Renderer::Init();
  ModelManager::Init();

  AudioManager::SetMusicVolume(Settings::GetMusicVolume());
  AudioManager::SetSFXVolume(Settings::GetSFXVolume());
  Renderer::ApplyDisplaySettings();

  SceneManager::LoadScene("menu");

  while (Window::IsRunning())
  {
    const auto frameStart = std::chrono::steady_clock::now();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    DeltaTime dt;

    SceneManager::Update(dt);

    Window::Update();

    LimitFrameRate(frameStart);
  }

  SceneManager::Shutdown();
  AudioManager::Terminate();
  ModelManager::Shutdown();
  LightManager::Shutdown();
  Renderer::Shutdown();
  FontManager::Shutdown();
  PhysX::Shutdown();
  Window::Terminate();

  return 0;
}
