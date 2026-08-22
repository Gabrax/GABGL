#include "RenderSystem.h"

#include "OpenGLRenderer.h"
#include "Camera.h"
#include "FontManager.h"
#include "RenderBackend.h"
#include "SceneManager.h"
#include "Window.h"

#include <algorithm>
#include <chrono>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

namespace
{
  float UIScale()
  {
    return std::max(0.25f, std::min(
      static_cast<float>(Window::GetWidth()) / 1280.0f,
      static_cast<float>(Window::GetHeight()) / 720.0f));
  }

  float AnimationTime()
  {
    static const auto start = std::chrono::steady_clock::now();
    return std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
  }

  bool WorldToScreen(const glm::vec3& worldPosition, glm::vec2& screenPosition)
  {
    const glm::vec4 clip = Camera::GetViewProjection() * glm::vec4(worldPosition, 1.0f);
    if (clip.w <= 0.001f) return false;
    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
    if (ndc.x < -1.0f || ndc.x > 1.0f || ndc.y < -1.0f || ndc.y > 1.0f ||
        ndc.z < -1.0f || ndc.z > 1.0f)
      return false;
    screenPosition = {
      (ndc.x * 0.5f + 0.5f) * static_cast<float>(Window::GetWidth()),
      (ndc.y * 0.5f + 0.5f) * static_cast<float>(Window::GetHeight())
    };
    return true;
  }

  void DrawInteractionLabels(const DeltaTime& dt)
  {
    glm::vec3 playerPosition;
    if (!SceneManager::GetPlayerPosition(playerPosition)) return;
    const Font* font = FontManager::GetFont("dpcomic");
    const float scale = UIScale();
    const uint64_t focusedEntity = SceneManager::GetFocusedEntityID();

    RenderSystem::BeginUI();
    for (const SceneEntity& entity : SceneManager::GetEntities())
    {
      if (!entity.active || !entity.interactable || entity.player) continue;
      const float distance = glm::length(entity.transform.GetPosition() - playerPosition);
      if (distance > entity.interactionRange) continue;

      glm::vec2 screenPosition;
      if (!WorldToScreen(entity.transform.GetPosition() + glm::vec3(0.0f, entity.labelHeight, 0.0f),
                         screenPosition))
        continue;

      const float fadeStart = entity.interactionRange * 0.7f;
      const float fadeLength = std::max(entity.interactionRange - fadeStart, 0.001f);
      const float alpha = 1.0f - glm::clamp((distance - fadeStart) / fadeLength, 0.0f, 1.0f);
      const bool focused = entity.id == focusedEntity;
      const std::string& name = entity.itemName.empty() ? entity.name : entity.itemName;
      RenderSystem::DrawText(font, name, screenPosition, (focused ? 0.48f : 0.4f) * scale,
        focused ? glm::vec4(1.0f, 0.88f, 0.38f, alpha) : glm::vec4(1.0f, 1.0f, 1.0f, alpha));
      if (focused)
        RenderSystem::DrawText(font, entity.pickable ? "E / X - PICK UP" : "E / X - INTERACT",
          screenPosition - glm::vec2(0.0f, 27.0f * scale), 0.27f * scale,
          glm::vec4(0.9f, 0.9f, 0.9f, alpha));
    }
    RenderSystem::DrawText(font, "FPS: " + std::to_string(dt.GetFPS()), glm::vec2(100.0f, 50.0f) * scale,
                           0.5f * scale, glm::vec4(1.0f));
    RenderSystem::EndUI();
  }
}

void RenderSystem::Initialize() { OpenGLRenderer::Init(); }
void RenderSystem::Shutdown() { OpenGLRenderer::Shutdown(); }

void RenderSystem::DrawScene(DeltaTime& dt, const std::function<void()>& sceneLogic,
                             bool advanceSimulation)
{
  const RenderEffectSettings effects = RenderBackend::GetEffectSettings();
  const bool renderForEditor = OpenGLRenderer::IsRenderingEditor();

  RenderBackend::Get().DrawScene(dt, sceneLogic, advanceSimulation, renderForEditor, effects);

  if (RenderBackend::Capabilities().NativeSceneRenderer)
  {
    if (renderForEditor)
      OpenGLRenderer::DrawEditorFrameBuffer(RenderBackend::Get().GetEditorTextureID());
    else if (advanceSimulation)
      DrawInteractionLabels(dt);
  }
}

void RenderSystem::SwitchRenderState() { OpenGLRenderer::SwitchRenderState(); }
void RenderSystem::ApplyDisplaySettings() { OpenGLRenderer::ApplyDisplaySettings(); }
void RenderSystem::ApplyGraphicsSettings() { OpenGLRenderer::ApplyGraphicsSettings(); }

void RenderSystem::PrepareScreenUI(const glm::vec4& clearColor, const bool clear)
{
  RenderBackend::Get().PrepareScreenUI(clearColor, clear);
}

void RenderSystem::BeginUI() { RenderBackend::Get().BeginUI(); }
void RenderSystem::EndUI() { RenderBackend::Get().EndUI(); }

void RenderSystem::DrawQuad(const glm::vec2& position, const glm::vec2& size, const float rotation,
                            const glm::vec4& color)
{
  const glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f)) *
    glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f)) *
    glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));
  RenderBackend::Get().DrawQuad(transform, color);
}

void RenderSystem::DrawText(const Font* font, const std::string& text, const glm::vec2& position,
                            const float size, const glm::vec4& color)
{
  RenderBackend::Get().DrawText(font, text, position, size, color);
}

void RenderSystem::DrawLoadingScreen()
{
  PrepareScreenUI(glm::vec4(0.008f, 0.012f, 0.025f, 1.0f));
  const float time = AnimationTime();
  std::string label = "LOADING";
  label.append(static_cast<size_t>(static_cast<int>(time * 2.5f) % 4), '.');
  const float pulse = 0.72f + std::sin(time * 3.0f) * 0.18f;
  const float width = static_cast<float>(Window::GetWidth());
  const float height = static_cast<float>(Window::GetHeight());
  const float scale = UIScale();

  BeginUI();
  DrawText(FontManager::GetFont("dpcomic"), label, glm::vec2(width * 0.5f, height * 0.5f), 0.82f * scale,
           glm::vec4(0.72f, 0.86f, 1.0f, pulse));
  DrawQuad(glm::vec2(width * 0.5f + std::sin(time * 1.8f) * 55.0f * scale, height * 0.44f),
           glm::vec2(68.0f, 3.0f) * scale, 0.0f, glm::vec4(0.28f, 0.58f, 0.92f, 0.65f));
  EndUI();
}

void RenderSystem::DrawScreenOverlay(float opacity, const glm::vec3& color)
{
  opacity = std::clamp(opacity, 0.0f, 1.0f);
  if (opacity <= 0.0f) return;
  PrepareScreenUI({}, false);
  BeginUI();
  DrawQuad(glm::vec2(Window::GetWidth() * 0.5f, Window::GetHeight() * 0.5f),
           glm::vec2(Window::GetWidth(), Window::GetHeight()), 0.0f, glm::vec4(color, opacity));
  EndUI();
}

bool RenderSystem::UploadSkybox(const std::shared_ptr<Texture>& cubemap)
{
  return RenderBackend::Get().UploadSkybox(cubemap);
}

void RenderSystem::FinalizeModelUpload()
{
  RenderBackend::Get().FinalizeModelUpload();
}

void RenderSystem::ResetModelDrawCommands()
{
  RenderBackend::Get().ResetModelDrawCommands();
}

void RenderSystem::RegisterModelDrawCommand(const std::string& modelName, const uint32_t vertexCount,
                                            const uint32_t indexCount)
{
  RenderBackend::Get().RegisterModelDrawCommand(modelName, vertexCount, indexCount);
}

void RenderSystem::UpdateModelInstances(const std::shared_ptr<Model>& model)
{
  RenderBackend::Get().UpdateModelInstances(model);
}

void RenderSystem::SetModelRendered(const std::shared_ptr<Model>& model, const bool rendered)
{
  RenderBackend::Get().SetModelRendered(model, rendered);
}
