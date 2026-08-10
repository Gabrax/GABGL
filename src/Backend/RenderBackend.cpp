#include "RenderBackend.h"

#include "DirectX12Renderer.h"
#include "Settings.h"
#include "Window.h"

#include <imgui.h>
#include "backends/imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <stdexcept>

namespace
{
  class OpenGLRenderBackend final : public IRenderBackend
  {
  public:
    [[nodiscard]] GraphicsAPI GetAPI() const override { return GraphicsAPI::OpenGL; }
    [[nodiscard]] const char* GetName() const override { return "OpenGL"; }
    [[nodiscard]] const RenderBackendCapabilities& GetCapabilities() const override
    {
      static constexpr RenderBackendCapabilities capabilities{
        .OpenGLContext = true,
        .FramebufferOriginBottomLeft = true,
        .PointLightShadows = true,
        .TimestampProfiler = true
      };
      return capabilities;
    }

    bool InitializeDevice(void*, uint32_t, uint32_t) override { return true; }
    void ShutdownDevice() override {}
    bool BeginFrame(uint32_t, uint32_t) override
    {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      return true;
    }
    bool EndFrame(bool) override
    {
      Window::Present();
      return true;
    }

    bool InitializeSceneRenderer() override { return true; }
    void ShutdownSceneRenderer() override {}
    bool DrawScene(DeltaTime&, const std::function<void()>&, bool, bool,
                   const RenderEffectSettings&) override { return false; }

    bool UploadModel(const std::shared_ptr<Model>&) override { return false; }
    bool UploadSkybox(const std::shared_ptr<Texture>&) override { return false; }
    void ResetSceneResources() override {}
    bool DrawParticles(const std::vector<ParticleRenderInstance>&) override { return false; }
    void PrepareScreenUI(const glm::vec4& clearColor, bool clear) override
    {
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glViewport(0, 0, Window::GetWidth(), Window::GetHeight());
      if (clear)
      {
        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      }
      glDisable(GL_DEPTH_TEST);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    bool BeginUI() override { return false; }
    bool EndUI() override { return false; }
    bool DrawQuad(const glm::mat4&, const glm::vec4&) override { return false; }
    bool DrawText(const std::string&, const glm::vec2&, float, const glm::vec4&) override
    {
      return false;
    }
    bool BeginDebugLines() override { return false; }
    bool DrawDebugLine(const glm::vec3&, const glm::vec3&, const glm::vec4&) override
    {
      return false;
    }
    bool EndDebugLines() override { return false; }
    bool DrawPhysicsDebug() override { return false; }

    bool InitializeImGuiRenderer() override
    {
      return ImGui_ImplOpenGL3_Init("#version 410");
    }
    void ShutdownImGuiRenderer() override { ImGui_ImplOpenGL3_Shutdown(); }
    void BeginImGuiFrame() override { ImGui_ImplOpenGL3_NewFrame(); }
    void RenderImGuiDrawData() override { ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); }
    void RenderImGuiPlatformWindows() override
    {
      GLFWwindow* backupContext = glfwGetCurrentContext();
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      glfwMakeContextCurrent(backupContext);
    }
    [[nodiscard]] uint64_t GetEditorTextureID() const override { return 0; }
  };

  class DirectX12RenderBackend final : public IRenderBackend
  {
  public:
    [[nodiscard]] GraphicsAPI GetAPI() const override { return GraphicsAPI::DirectX12; }
    [[nodiscard]] const char* GetName() const override { return "DirectX 12"; }
    [[nodiscard]] const RenderBackendCapabilities& GetCapabilities() const override
    {
      static constexpr RenderBackendCapabilities capabilities{
        .NativePresentation = true,
        .NativeSceneRenderer = true,
        .NativeModelResources = true,
        .NativeParticleRenderer = true,
        .NativeUIRenderer = true,
        .NativeDebugLines = true,
        .NativePhysicsDebug = true,
        .PointLightShadows = true
      };
      return capabilities;
    }

    bool InitializeDevice(void* nativeWindow, uint32_t width, uint32_t height) override
    {
      return DirectX12Renderer::Init(nativeWindow, width, height);
    }
    void ShutdownDevice() override { DirectX12Renderer::Shutdown(); }
    bool BeginFrame(uint32_t width, uint32_t height) override
    {
      return DirectX12Renderer::Resize(width, height) && DirectX12Renderer::BeginFrame();
    }
    bool EndFrame(bool vSync) override { return DirectX12Renderer::EndFrame(vSync); }

    bool InitializeSceneRenderer() override { return DirectX12Renderer::InitSceneRenderer(); }
    void ShutdownSceneRenderer() override { DirectX12Renderer::ShutdownSceneRenderer(); }
    bool DrawScene(DeltaTime& dt, const std::function<void()>& sceneLogic,
                   bool advanceSimulation, bool renderForEditor,
                   const RenderEffectSettings& effects) override
    {
      DirectX12Renderer::DrawScene(dt, sceneLogic, advanceSimulation, renderForEditor, effects);
      return true;
    }

    bool UploadModel(const std::shared_ptr<Model>& model) override
    {
      return DirectX12Renderer::UploadModel(model);
    }
    bool UploadSkybox(const std::shared_ptr<Texture>& cubemap) override
    {
      return DirectX12Renderer::UploadSkybox(cubemap);
    }
    void ResetSceneResources() override { DirectX12Renderer::ResetSceneResources(); }
    bool DrawParticles(const std::vector<ParticleRenderInstance>& instances) override
    {
      DirectX12Renderer::DrawParticles(instances);
      return true;
    }
    bool BeginUI() override { DirectX12Renderer::BeginScene(); return true; }
    void PrepareScreenUI(const glm::vec4& clearColor, bool clear) override
    {
      DirectX12Renderer::PrepareScreenUI(clearColor, clear);
    }
    bool EndUI() override { DirectX12Renderer::EndScene(); return true; }
    bool DrawQuad(const glm::mat4& transform, const glm::vec4& color) override
    {
      DirectX12Renderer::DrawQuad(transform, color);
      return true;
    }
    bool DrawText(const std::string& text, const glm::vec2& position,
                  float size, const glm::vec4& color) override
    {
      DirectX12Renderer::DrawText(text, position, size, color);
      return true;
    }
    bool BeginDebugLines() override { DirectX12Renderer::BeginDebugLines(); return true; }
    bool DrawDebugLine(const glm::vec3& start, const glm::vec3& end,
                       const glm::vec4& color) override
    {
      DirectX12Renderer::DrawDebugLine(start, end, color);
      return true;
    }
    bool EndDebugLines() override { DirectX12Renderer::EndDebugLines(); return true; }
    bool DrawPhysicsDebug() override
    {
      DirectX12Renderer::DrawPhysicsDebug();
      return true;
    }

    bool InitializeImGuiRenderer() override { return DirectX12Renderer::InitImGui(); }
    void ShutdownImGuiRenderer() override { DirectX12Renderer::ShutdownImGui(); }
    void BeginImGuiFrame() override { DirectX12Renderer::BeginImGuiFrame(); }
    void RenderImGuiDrawData() override { DirectX12Renderer::RenderImGuiDrawData(); }
    void RenderImGuiPlatformWindows() override
    {
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
    }
    [[nodiscard]] uint64_t GetEditorTextureID() const override
    {
      return DirectX12Renderer::GetEditorTextureID();
    }
  };

  std::unique_ptr<IRenderBackend> s_Backend;
  RenderDebugSettings s_DebugSettings;
  RenderStatistics s_Statistics;
}

bool RenderBackend::Select(GraphicsAPI api)
{
  switch (api)
  {
    case GraphicsAPI::DirectX12: s_Backend = std::make_unique<DirectX12RenderBackend>(); break;
    case GraphicsAPI::OpenGL: s_Backend = std::make_unique<OpenGLRenderBackend>(); break;
    default: return false;
  }
  GraphicsAPIState::Set(api);
  return true;
}

IRenderBackend& RenderBackend::Get()
{
  if (!s_Backend) throw std::runtime_error("No graphics backend has been selected");
  return *s_Backend;
}

const RenderBackendCapabilities& RenderBackend::Capabilities()
{
  return Get().GetCapabilities();
}

RenderEffectSettings RenderBackend::GetEffectSettings()
{
  RenderEffectSettings settings;
  settings.ShadowQuality = Settings::GetShadowQuality();
  settings.BloomQuality = Settings::GetBloomQuality();
  return settings;
}

RenderDebugSettings& RenderBackend::DebugSettings() { return s_DebugSettings; }
const RenderStatistics& RenderBackend::Statistics() { return s_Statistics; }
void RenderBackend::SetStatistics(const RenderStatistics& statistics) { s_Statistics = statistics; }
