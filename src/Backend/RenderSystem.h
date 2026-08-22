#pragma once

#include "DeltaTime.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

struct Font;
struct Model;
struct Texture;
struct RenderModelPreview;

// API-neutral entry point for rendering a game scene. The selected backend
// always owns the scene pass; this type does not provide an OpenGL fallback.
struct RenderSystem
{
  static void Initialize();
  static void Shutdown();
  static void DrawScene(DeltaTime& dt, const std::function<void()>& sceneLogic,
                        bool advanceSimulation = true);
  static void SwitchRenderState();
  static void ApplyDisplaySettings();
  static void ApplyGraphicsSettings();

  static void PrepareScreenUI(const glm::vec4& clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
                              bool clear = true);
  static void BeginUI();
  static void EndUI();
  static void DrawQuad(const glm::vec2& position, const glm::vec2& size, float rotation,
                       const glm::vec4& color);
  static void DrawText(const Font* font, const std::string& text, const glm::vec2& position,
                       float size, const glm::vec4& color = glm::vec4(1.0f));
  static void DrawLoadingScreen();
  static void DrawScreenOverlay(float opacity, const glm::vec3& color = glm::vec3(0.0f));

  static bool UploadSkybox(const std::shared_ptr<Texture>& cubemap);
  static void FinalizeModelUpload();
  static void ResetModelDrawCommands();
  static void RegisterModelDrawCommand(const std::string& modelName, uint32_t vertexCount,
                                       uint32_t indexCount);
  static void UpdateModelInstances(const std::shared_ptr<Model>& model);
  static void SetModelRendered(const std::shared_ptr<Model>& model, bool rendered);
  static void SetModelPreviews(const std::vector<RenderModelPreview>& previews);
};
