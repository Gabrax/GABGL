#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include <glm/glm.hpp>

struct DeltaTime;
struct Model;

// Minimal native DirectX 12 rendering path used while the OpenGL renderer is
// migrated incrementally. The implementation intentionally keeps all D3D12
// headers out of the rest of the engine.
struct DirectX12Renderer
{
  static bool Init(void* nativeWindow, uint32_t width, uint32_t height);
  static void Shutdown();

  static bool Resize(uint32_t width, uint32_t height);
  static bool BeginFrame();
  static bool EndFrame(bool vSync);
  static bool IsInitialized();

  static bool InitSceneRenderer();
  static void ShutdownSceneRenderer();
  static void ResetSceneResources();
  static bool UploadModel(const std::shared_ptr<Model>& model);

  static void DrawScene(DeltaTime& dt, const std::function<void()>& sceneLogic,
                        bool advanceSimulation = true);
  static void BeginScene();
  static void EndScene();
  static void DrawQuad(const glm::mat4& transform, const glm::vec4& color);
  static void DrawText(const std::string& text, const glm::vec2& position,
                       float size, const glm::vec4& color);
};
