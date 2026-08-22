#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "RenderCommon.h"

struct DeltaTime;
struct Model;
struct ParticleRenderInstance;
struct Texture;

// Native DirectX 12 implementation. D3D12 headers and objects stay confined
// to this translation unit and its backend adapter.
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
  static bool UploadSkybox(const std::shared_ptr<Texture>& cubemap);
  static void SetModelPreviews(const std::vector<RenderModelPreview>& previews);

  static bool InitImGui();
  static void ShutdownImGui();
  static void BeginImGuiFrame();
  static void RenderImGuiDrawData();
  static uint64_t GetEditorTextureID();

  static void DrawScene(DeltaTime& dt, const std::function<void()>& sceneLogic,
                        bool advanceSimulation, bool renderForEditor,
                        const RenderEffectSettings& effects);
  static void DrawParticles(const std::vector<ParticleRenderInstance>& instances);
  static void BeginDebugLines();
  static void DrawDebugLine(const glm::vec3& start, const glm::vec3& end,
                            const glm::vec4& color);
  static void EndDebugLines();
  static void DrawPhysicsDebug();
  static void PrepareScreenUI(const glm::vec4& clearColor, bool clear);
  static void BeginScene();
  static void EndScene();
  static void DrawQuad(const glm::mat4& transform, const glm::vec4& color);
  static void DrawText(const std::string& text, const glm::vec2& position,
                       float size, const glm::vec4& color);
};
