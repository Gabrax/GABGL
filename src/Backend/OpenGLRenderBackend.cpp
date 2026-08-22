#include "RenderBackendFactory.h"

#include "OpenGLRenderer.h"
#include "RenderBackend.h"
#include "Window.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <imgui.h>
#include "backends/imgui_impl_opengl3.h"

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
    void DrawScene(DeltaTime& dt, const std::function<void()>& sceneLogic,
                   bool advanceSimulation, bool, const RenderEffectSettings&) override
    {
      OpenGLRenderer::DrawScene(dt, sceneLogic, advanceSimulation);
    }

    bool UploadModel(const std::shared_ptr<Model>&) override { return false; }
    bool UploadSkybox(const std::shared_ptr<Texture>& cubemap) override
    {
      OpenGLRenderer::BakeSkyboxTextures("night", cubemap);
      return true;
    }
    void ResetSceneResources() override {}
    void RegisterModelDrawCommand(const std::string& modelName, uint32_t vertexCount,
                                  uint32_t indexCount) override
    {
      OpenGLRenderer::AddDrawCommand(modelName, vertexCount, indexCount);
    }
    void UpdateModelInstances(const std::shared_ptr<Model>& model) override
    {
      OpenGLRenderer::UpdateDrawCommandInstances(model);
    }
    void SetModelRendered(const std::shared_ptr<Model>& model, bool rendered) override
    {
      OpenGLRenderer::RebuildDrawCommandsForModel(model, rendered);
    }
    void FinalizeModelUpload() override { OpenGLRenderer::InitDrawCommandBuffer(); }
    void ResetModelDrawCommands() override { OpenGLRenderer::ResetModelDrawCommands(); }
    bool DrawParticles(const std::vector<ParticleRenderInstance>&) override { return false; }
    void PrepareScreenUI(const glm::vec4& clearColor, bool clear) override
    {
      OpenGLRenderer::PrepareScreenUI(clearColor, clear);
    }
    bool BeginUI() override { OpenGLRenderer::BeginScene(); return true; }
    bool EndUI() override { OpenGLRenderer::EndScene(); return true; }
    bool DrawQuad(const glm::mat4& transform, const glm::vec4& color) override
    {
      OpenGLRenderer::DrawQuad(transform, color);
      return true;
    }
    bool DrawText(const Font* font, const std::string& text, const glm::vec2& position,
                  float size, const glm::vec4& color) override
    {
      OpenGLRenderer::DrawText(font, text, position, size, color);
      return true;
    }
    bool BeginDebugLines() override { OpenGLRenderer::BeginScene(); return true; }
    bool DrawDebugLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color) override
    {
      OpenGLRenderer::DrawLine(start, end, color);
      return true;
    }
    bool EndDebugLines() override { OpenGLRenderer::EndScene(); return true; }
    bool DrawPhysicsDebug() override { return false; }

    bool InitializeImGuiRenderer() override { return ImGui_ImplOpenGL3_Init("#version 410"); }
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
    void OnLightsChanged(const std::vector<RenderLight>& lights) override
    {
      OpenGLRenderer::SetLights(lights);
    }
  };
}

std::unique_ptr<IRenderBackend> CreateOpenGLRenderBackend()
{
  return std::make_unique<OpenGLRenderBackend>();
}
