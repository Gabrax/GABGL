#include "backend/ModelManager.h"
#include "backend/Logger.h"
#include "backend/AudioManager.h"
#include "backend/PhysX.h"
#include "backend/FontManager.h"
#include "backend/Settings.h"
#include "backend/SceneManager.h"
#include "backend/Window.h"
#include "backend/RenderBackend.h"
#include "backend/RenderSystem.h"

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
      if (const std::string_view argument(argv[i]); argument == "--dx12" || argument == "--renderer=dx12")
        api = GraphicsAPI::DirectX12;
      else if (argument == "--opengl" || argument == "--renderer=opengl")
        api = GraphicsAPI::OpenGL;
      else if (argument == "--renderer" && i + 1 < argc)
      {
        if (const std::string_view value(argv[++i]); value == "dx12" || value == "directx12") api = GraphicsAPI::DirectX12;
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
  if (!RenderBackend::Select(graphicsAPI))
  {
    GABGL_ERROR("Unsupported graphics API");
    return 1;
  }
  GABGL_INFO("Selected graphics API: {}", GraphicsAPIName(graphicsAPI));

  const std::string windowTitle = graphicsAPI == GraphicsAPI::OpenGL
    ? "GABGL" : std::string("GABGL - ") + RenderBackend::Get().GetName();
  Window::Init(windowTitle, Settings::GetWindowWidth(), Settings::GetWindowHeight(), graphicsAPI);
  if (!RenderBackend::Get().InitializeDevice(
        Window::GetNativeHandle(), Window::GetWidth(), Window::GetHeight()))
  {
    GABGL_ERROR("Could not initialize the {} backend", RenderBackend::Get().GetName());
    if (graphicsAPI == GraphicsAPI::DirectX12)
      GABGL_ERROR("Configure with -DGABGL_ENABLE_DX12=ON to include DirectX 12");
    Window::Terminate();
    return 1;
  }

  AudioManager::Init();
  if (RenderBackend::Capabilities().OpenGLContext) FontManager::Init();
  PhysX::Init();
  RenderSystem::Initialize();
  ModelManager::Init();

  AudioManager::SetMusicVolume(Settings::GetMusicVolume());
  AudioManager::SetSFXVolume(Settings::GetSFXVolume());
  RenderSystem::ApplyDisplaySettings();

  SceneManager::LoadScene("menu");

  while (Window::IsRunning())
  {
    const auto frameStart = std::chrono::steady_clock::now();
    Window::PollEvents();

    if (Window::IsMinimized())
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
      continue;
    }

    if (!RenderBackend::Get().BeginFrame(Window::GetWidth(), Window::GetHeight()))
    {
      Window::RequestClose();
      continue;
    }

    DeltaTime dt;
    SceneManager::Update(dt);

    if (!RenderBackend::Get().EndFrame(Settings::GetVSync()))
      Window::RequestClose();

    LimitFrameRate(frameStart);
  }

  SceneManager::Shutdown();
  AudioManager::Terminate();
  ModelManager::Shutdown();
  RenderBackend::ClearLights();
  RenderSystem::Shutdown();
  if (RenderBackend::Capabilities().OpenGLContext) FontManager::Shutdown();
  PhysX::Shutdown();
  RenderBackend::Get().ShutdownDevice();
  Window::Terminate();

  return 0;
}
