#pragma once

#include "GraphicsAPI.h"
#include "RenderCommon.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

struct DeltaTime;
struct Font;
struct Model;
struct ParticleRenderInstance;
struct Texture;

class IRenderBackend
{
public:
  virtual ~IRenderBackend() = default;

  [[nodiscard]] virtual GraphicsAPI GetAPI() const = 0;
  [[nodiscard]] virtual const char* GetName() const = 0;
  [[nodiscard]] virtual const RenderBackendCapabilities& GetCapabilities() const = 0;

  virtual bool InitializeDevice(void* nativeWindow, uint32_t width, uint32_t height) = 0;
  virtual void ShutdownDevice() = 0;
  virtual bool BeginFrame(uint32_t width, uint32_t height) = 0;
  virtual bool EndFrame(bool vSync) = 0;

  virtual bool InitializeSceneRenderer() = 0;
  virtual void ShutdownSceneRenderer() = 0;
  virtual void DrawScene(DeltaTime& dt, const std::function<void()>& sceneLogic,
                         bool advanceSimulation, bool renderForEditor,
                         const RenderEffectSettings& effects) = 0;

  virtual bool UploadModel(const std::shared_ptr<Model>& model) = 0;
  virtual bool UploadSkybox(const std::shared_ptr<Texture>& cubemap) = 0;
  virtual void ResetSceneResources() = 0;
  virtual void RegisterModelDrawCommand(const std::string& modelName, uint32_t vertexCount,
                                        uint32_t indexCount) = 0;
  virtual void UpdateModelInstances(const std::shared_ptr<Model>& model) = 0;
  virtual void SetModelRendered(const std::shared_ptr<Model>& model, bool rendered) = 0;
  virtual void FinalizeModelUpload() = 0;
  virtual void ResetModelDrawCommands() = 0;
  virtual bool DrawParticles(const std::vector<ParticleRenderInstance>& instances) = 0;

  virtual bool BeginUI() = 0;
  virtual void PrepareScreenUI(const glm::vec4& clearColor, bool clear) = 0;
  virtual bool EndUI() = 0;
  virtual bool DrawQuad(const glm::mat4& transform, const glm::vec4& color) = 0;
  virtual bool DrawText(const Font* font, const std::string& text, const glm::vec2& position,
                        float size, const glm::vec4& color) = 0;
  virtual bool BeginDebugLines() = 0;
  virtual bool DrawDebugLine(const glm::vec3& start, const glm::vec3& end,
                             const glm::vec4& color) = 0;
  virtual bool EndDebugLines() = 0;
  virtual bool DrawPhysicsDebug() = 0;

  virtual bool InitializeImGuiRenderer() = 0;
  virtual void ShutdownImGuiRenderer() = 0;
  virtual void BeginImGuiFrame() = 0;
  virtual void RenderImGuiDrawData() = 0;
  virtual void RenderImGuiPlatformWindows() = 0;
  [[nodiscard]] virtual uint64_t GetEditorTextureID() const = 0;
  virtual void OnLightsChanged(const std::vector<RenderLight>& lights) = 0;
};

struct RenderBackend
{
  static bool Select(GraphicsAPI api);
  static IRenderBackend& Get();
  static const RenderBackendCapabilities& Capabilities();

  static RenderEffectSettings GetEffectSettings();
  static RenderDebugSettings& DebugSettings();
  static const RenderStatistics& Statistics();
  static void SetStatistics(const RenderStatistics& statistics);
  static bool AddLight(const RenderLight& light);
  static bool UpdateLight(size_t index, const RenderLight& light);
  static bool RemoveLight(size_t index);
  static void ClearLights();
  [[nodiscard]] static const std::vector<RenderLight>& Lights();
};
