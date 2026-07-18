#include "Renderer.h"

#include "Logger.h"
#include "Buffer.h"
#include "Camera.h"
#include "LightManager.h"
#include "ModelManager.h"
#include "ParticleRenderer.h"
#include "Renderer.h"
#include "Shader.h"
#include "Texture.h"
#include "AudioManager.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stb_image.h>
#include <imgui.h>
#include <imgui_internal.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
//#include "ImGuizmo.h"
#include "glm/ext/scalar_constants.hpp"
#include "glm/trigonometric.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/fwd.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include "Profiler.h"
#include "SceneManager.h"
#include "Settings.h"
#include "Timer.hpp"
#include "Window.h"

void MessageCallback(unsigned source,unsigned type,unsigned id,unsigned severity,int length,const char* message,const void* userParam)
{
	switch (severity)
	{
		case GL_DEBUG_SEVERITY_HIGH:         GABGL_CRITICAL("{}",message); return;
		case GL_DEBUG_SEVERITY_MEDIUM:       GABGL_ERROR("{}",message); return;
		case GL_DEBUG_SEVERITY_LOW:          GABGL_WARN("{}",message); return;
		case GL_DEBUG_SEVERITY_NOTIFICATION: GABGL_TRACE("{}", message); return;
	}

	GABGL_ASSERT(false, "Unknown severity level!");
}

struct QuadVertex
{
	glm::vec3 Position;
	glm::vec4 Color;
	glm::vec2 TexCoord;
	float TexIndex;
	float TilingFactor;

	int EntityID;
};

struct LineVertex
{
	glm::vec3 Position;
	glm::vec4 Color;

	int EntityID;
};

struct CameraData
{
	glm::mat4 ViewProjection;
	glm::mat4 OrtoProjection;
  glm::mat4 NonRotViewProjection;
  glm::vec3 CameraPos;
};

struct DrawElementsIndirectCommand
{
  GLuint count;         // Number of indices
  GLuint instanceCount; // This is for instancing
  GLuint firstIndex;    // Offset into the index buffer
  GLint baseVertex;    // Base vertex for this draw
  GLuint baseInstance;  // You can use this to index per-object data
};

struct RendererData
{
	int m_GizmoType;
	uint64_t m_SelectedEntityID = 0;
	uint64_t m_SelectedLightID = 0;
	int selectedSceneIndex = -1;
	bool m_ViewportFocused = false, m_ViewportHovered = false;
	glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
	glm::vec2 m_ViewportBounds[2];
	bool m_BlockEvents = true;

	static constexpr uint32_t MaxQuads = 20000;
	static constexpr uint32_t MaxVertices = MaxQuads * 4;
	static constexpr uint32_t MaxIndices = MaxQuads * 6;
	static constexpr uint32_t MaxTextureSlots = 32; // TODO: RenderCaps

	std::shared_ptr<Texture> WhiteTexture;

	std::shared_ptr<VertexArray> QuadVertexArray;
	std::shared_ptr<VertexBuffer> QuadVertexBuffer;
	uint32_t QuadIndexCount = 0;
	QuadVertex* QuadVertexBufferBase = nullptr;
	QuadVertex* QuadVertexBufferPtr = nullptr;
	glm::vec4 QuadVertexPositions[4];
	static constexpr size_t quadVertexCount = 4;

	std::shared_ptr<VertexArray> LineVertexArray;
	std::shared_ptr<VertexBuffer> LineVertexBuffer;
	uint32_t LineVertexCount = 0;
	LineVertex* LineVertexBufferBase = nullptr;
	LineVertex* LineVertexBufferPtr = nullptr;
	float LineWidth = 2.0f;

  static constexpr glm::vec3 quadPositions[4] =
  {
    { 0.0f, 0.0f, 0.0f },
    { 1.0f, 0.0f, 0.0f },
    { 1.0f, 1.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f }
  };

  static constexpr glm::vec2 tex3DCoords[4] =
  {
    { 0.0f, 1.0f },
    { 1.0f, 1.0f },
    { 1.0f, 0.0f },
    { 0.0f, 0.0f }
  };

	std::array<std::shared_ptr<Texture>, MaxTextureSlots> TextureSlots;
	uint32_t TextureSlotIndex = 1; // 0 = white texture

	static constexpr glm::vec2 tex2DCoords[4] = 
  {
    { 0.0f, 0.0f },
    { 1.0f, 0.0f },
    { 1.0f, 1.0f },
    { 0.0f, 1.0f }
  };
	static constexpr float tilingFactor = 1.0f;

  struct Shaders
	{
    std::shared_ptr<Shader> QuadShader;
		std::shared_ptr<Shader> CircleShader;
		std::shared_ptr<Shader> LineShader;
    std::shared_ptr<Shader> FramebufferShader;
    std::shared_ptr<Shader> skyboxShader;
    std::shared_ptr<Shader> GeometryShader;
    std::shared_ptr<Shader> LightShader;
    std::shared_ptr<Shader> DownSampleShader;
    std::shared_ptr<Shader> UpSampleShader;
    std::shared_ptr<Shader> BloomResultShader;
    std::shared_ptr<Shader> OmniDirectShadowShader;
    std::shared_ptr<Shader> DirectShadowShader;
    std::shared_ptr<Shader> PhysicsDebugShader;

	} s_Shaders;

  CameraData m_CameraBuffer;
  std::shared_ptr<UniformBuffer> m_CameraUniformBuffer;
  std::shared_ptr<UniformBuffer> m_ResolutionUniformBuffer;

  std::unordered_map<std::string, std::shared_ptr<Texture>> skyboxes;

  std::shared_ptr<FrameBuffer> m_ResultBuffer;
  std::shared_ptr<BloomBuffer> m_BloomBuffer;
  std::shared_ptr<OmniDirectShadowBuffer> m_OmniDirectShadowBuffer;
  std::shared_ptr<DirectShadowBuffer> m_DirectShadowBuffer;
  std::shared_ptr<GeometryBuffer> m_GeometryBuffer;

  std::vector<DrawElementsIndirectCommand> m_DrawCommands;
  std::vector<DrawElementsIndirectCommand> m_CulledDrawCommands;
  std::vector<glm::mat4> m_VisibleInstanceTransforms;
  std::unordered_map<std::string, std::vector<size_t>> m_ModelDrawCommandIndices;
  uint32_t m_DrawIndexOffset = 0;
  uint32_t m_DrawVertexOffset = 0;
  uint32_t m_cmdBufer = 0;
  uint32_t m_CulledCmdBuffer = 0;
  size_t m_cmdBufferSize = 0;
  uint32_t m_VisibleInstanceCount = 0;
  uint32_t m_RenderableInstanceCount = 0;
  bool m_PhysicsDebug = false;
  uint32_t m_AppliedShadowQuality = std::numeric_limits<uint32_t>::max();
  int m_PointShadowMask = 0;
  static constexpr uint32_t MaxShadowedPointLights = 4;
  static constexpr uint32_t MaxOmniShadowLayers = 20;
  static constexpr float PointShadowRadius = 20.0f;

  enum class SceneState
	{
		Edit = 0, Play = 1

	} m_SceneState;

  bool Is3D = false;

} s_Data;

struct Frustum
{
  explicit Frustum(const glm::mat4& viewProjection)
  {
    const glm::vec4 row0(viewProjection[0][0], viewProjection[1][0], viewProjection[2][0], viewProjection[3][0]);
    const glm::vec4 row1(viewProjection[0][1], viewProjection[1][1], viewProjection[2][1], viewProjection[3][1]);
    const glm::vec4 row2(viewProjection[0][2], viewProjection[1][2], viewProjection[2][2], viewProjection[3][2]);
    const glm::vec4 row3(viewProjection[0][3], viewProjection[1][3], viewProjection[2][3], viewProjection[3][3]);
    m_Planes = {row3 + row0, row3 - row0, row3 + row1, row3 - row1, row3 + row2, row3 - row2};

    for (glm::vec4& plane : m_Planes)
    {
      const float normalLength = glm::length(glm::vec3(plane));
      if (normalLength > 0.0f)
        plane /= normalLength;
    }
  }

  bool IntersectsSphere(const glm::vec3& center, float radius) const
  {
    for (const glm::vec4& plane : m_Planes)
      if (glm::dot(glm::vec3(plane), center) + plane.w < -radius)
        return false;
    return true;
  }

private:
  std::array<glm::vec4, 6> m_Planes;
};

static bool SphereIntersectsFrustum(const glm::mat4& viewProjection, const glm::vec3& center, float radius)
{
  return Frustum(viewProjection).IntersectsSphere(center, radius);
}

static bool CubemapFaceCanContainVisibleReceiver(const glm::vec3& lightPosition,
  const glm::vec3& cameraPosition, const glm::vec3& faceDirection, float radius)
{
  const glm::vec3 toCamera = cameraPosition - lightPosition;
  const float cameraDistance = glm::length(toCamera);
  if (cameraDistance <= radius)
    return true;

  constexpr float CubemapFaceHalfDiagonal = 0.9553166f; // acos(1 / sqrt(3))
  const float receiverCone = std::asin(glm::clamp(radius / cameraDistance, 0.0f, 1.0f));
  const float maxAngle = std::min(glm::pi<float>(), CubemapFaceHalfDiagonal + receiverCone);
  return glm::dot(glm::normalize(toCamera), glm::normalize(faceDirection)) >= std::cos(maxAngle);
}

void Renderer::LoadShaders()
{
	Shader::Create(s_Data.s_Shaders.QuadShader, "../res/shaders/batch_quad.glsl");
	Shader::Create(s_Data.s_Shaders.CircleShader, "../res/shaders/batch_circle.glsl");
	Shader::Create(s_Data.s_Shaders.LineShader, "../res/shaders/batch_line.glsl");
	Shader::Create(s_Data.s_Shaders.FramebufferShader, "../res/shaders/finalFB.glsl");
  Shader::Create(s_Data.s_Shaders.skyboxShader, "../res/shaders/skybox.glsl");
  Shader::Create(s_Data.s_Shaders.GeometryShader, "../res/shaders/geometry.glsl");
  Shader::Create(s_Data.s_Shaders.LightShader, "../res/shaders/light.glsl");
  Shader::Create(s_Data.s_Shaders.DownSampleShader, "../res/shaders/bloom_downsample.glsl");
  Shader::Create(s_Data.s_Shaders.UpSampleShader, "../res/shaders/bloom_upsample.glsl");
  Shader::Create(s_Data.s_Shaders.BloomResultShader, "../res/shaders/bloom_final.glsl");
  Shader::Create(s_Data.s_Shaders.OmniDirectShadowShader, "../res/shaders/omni_shadowFB.glsl");
  Shader::Create(s_Data.s_Shaders.DirectShadowShader, "../res/shaders/direct_shadowFB.glsl");
  Shader::Create(s_Data.s_Shaders.PhysicsDebugShader, "../res/shaders/physics_debug.glsl");
}

void Renderer::Init()
{
#ifdef DEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(MessageCallback, nullptr);

	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
#endif

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);

  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

  s_Data.QuadVertexArray = VertexArray::Create();
	s_Data.QuadVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(QuadVertex));
	s_Data.QuadVertexBuffer->SetLayout({
		{ ShaderDataType::Float3, "a_Position"     },
		{ ShaderDataType::Float4, "a_Color"        },
		{ ShaderDataType::Float2, "a_TexCoord"     },
		{ ShaderDataType::Float,  "a_TexIndex"     },
		{ ShaderDataType::Float,  "a_TilingFactor" },
		{ ShaderDataType::Int,    "a_EntityID"     }
		});
	s_Data.QuadVertexArray->AddVertexBuffer(s_Data.QuadVertexBuffer);
	s_Data.QuadVertexBufferBase = new QuadVertex[s_Data.MaxVertices];

	uint32_t* quadIndices = new uint32_t[s_Data.MaxIndices];

	uint32_t offset = 0;
	for (uint32_t i = 0; i < s_Data.MaxIndices; i += 6)
	{
		quadIndices[i + 0] = offset + 0;
		quadIndices[i + 1] = offset + 1;
		quadIndices[i + 2] = offset + 2;

		quadIndices[i + 3] = offset + 2;
		quadIndices[i + 4] = offset + 3;
		quadIndices[i + 5] = offset + 0;

		offset += 4;
	}

	std::shared_ptr<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, s_Data.MaxIndices);
	s_Data.QuadVertexArray->SetIndexBuffer(quadIB);
	delete[] quadIndices;

	s_Data.LineVertexArray = VertexArray::Create();
	s_Data.LineVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(LineVertex));
	s_Data.LineVertexBuffer->SetLayout({
		{ ShaderDataType::Float3, "a_Position" },
		{ ShaderDataType::Float4, "a_Color"    },
		{ ShaderDataType::Int,    "a_EntityID" }
		});
	s_Data.LineVertexArray->AddVertexBuffer(s_Data.LineVertexBuffer);
	s_Data.LineVertexBufferBase = new LineVertex[s_Data.MaxVertices];

	s_Data.WhiteTexture = Texture::Create(TextureSpecification());
	uint32_t whiteTextureData = 0xffffffff;
	s_Data.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

	int32_t samplers[s_Data.MaxTextureSlots];
	for (uint32_t i = 0; i < s_Data.MaxTextureSlots; i++) samplers[i] = i;

	s_Data.TextureSlots[0] = s_Data.WhiteTexture;

	s_Data.QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
	s_Data.QuadVertexPositions[1] = { 0.5f, -0.5f, 0.0f, 1.0f };
	s_Data.QuadVertexPositions[2] = { 0.5f,  0.5f, 0.0f, 1.0f };
	s_Data.QuadVertexPositions[3] = { -0.5f,  0.5f, 0.0f, 1.0f };

  std::vector<uint32_t> indices;
  indices.reserve(s_Data.MaxIndices);

  LoadShaders();

  glm::vec2 resolution = { Window::GetWidth(), Window::GetHeight() };

  FramebufferSpecification fbSpec;
	fbSpec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::DEPTH24STENCIL8 };
	fbSpec.Width = resolution.x;
	fbSpec.Height = resolution.y;
	s_Data.m_ResultBuffer = FrameBuffer::Create(fbSpec);

  s_Data.m_GeometryBuffer = GeometryBuffer::Create(resolution.x, resolution.y);
  s_Data.m_BloomBuffer = BloomBuffer::Create(s_Data.s_Shaders.DownSampleShader, s_Data.s_Shaders.UpSampleShader, s_Data.s_Shaders.BloomResultShader);
  ParticleRenderer::Init();
  ApplyGraphicsSettings();

  s_Data.m_CameraUniformBuffer = UniformBuffer::Create(sizeof(CameraData), 0);
  Camera::Init(45.0f, (float)resolution.x / (float)resolution.y, 0.01f, 2000.0f);
  Camera::SetViewportSize((float)resolution.x, (float)resolution.y);

  s_Data.m_ResolutionUniformBuffer = UniformBuffer::Create(sizeof(glm::vec2), 1);
  s_Data.m_ResolutionUniformBuffer->SetData(&resolution, sizeof(glm::vec2));

  s_Data.m_SceneState = RendererData::SceneState::Play;
  Camera::SetMode(CameraMode::PLAYER);
  Window::SetCursorVisible(false);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	GLFWwindow* window = reinterpret_cast<GLFWwindow*>(Window::GetWindowPtr());

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 410");
	SetLineWidth(4.0f);
	//s_Data.m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;

  Profiler::Init();
}

void Renderer::Shutdown()
{
	ParticleRenderer::Shutdown();
	delete[] s_Data.QuadVertexBufferBase;

  ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void Renderer::DrawScene(DeltaTime& dt, const std::function<void()>& scene_logic)
{
  GABGL_RESOLVE_GPU_QUERIES();
  ApplyGraphicsSettings();

  const bool shadowsEnabled = Settings::GetShadowQuality() != GraphicsQuality::Off;

  glDisable(GL_DITHER);
  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_FRONT);
  glFrontFace(GL_CW);

  scene_logic();

  {
    GABGL_PROFILE_SCOPE("MISC UPDATE PASS");

    Camera::OnUpdate(dt);
    AudioManager::SetListenerLocation(Camera::GetPosition());
    AudioManager::SetListenerOrientation(Camera::GetForwardDirection(), Camera::GetUpDirection());
    ModelManager::UpdateTransforms(dt);
    PhysX::Simulate(dt);
    AudioManager::UpdateAllMusic();
  }
  ModelManager::BindAllInstanceTransforms();
  if(shadowsEnabled && !LightManager::DirectLightEmpty())
  {
    GABGL_PROFILE_SCOPE("DIRECT SHADOW PASS");

    s_Data.m_DirectShadowBuffer->Bind();
    float max = std::numeric_limits<float>::max();
  	glClearColor(max, max, max, max);
    glClear(GL_DEPTH_BUFFER_BIT);

    s_Data.s_Shaders.DirectShadowShader->Bind();
    s_Data.m_DirectShadowBuffer->UpdateShadowView(LightManager::GetDirectLightRotation(), Camera::GetPosition());
    s_Data.s_Shaders.DirectShadowShader->SetMat4("u_LightSpaceMatrix", s_Data.m_DirectShadowBuffer->GetShadowViewProj());
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);
    glBindVertexArray(ModelManager::GetModelsVAO());
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER,s_Data.m_cmdBufer);
    glMultiDrawElementsIndirect(GL_TRIANGLES,GL_UNSIGNED_INT,NULL,s_Data.m_DrawCommands.size(),0);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER,0);
    glBindVertexArray(0);
    glDisable(GL_POLYGON_OFFSET_FILL);

    s_Data.s_Shaders.DirectShadowShader->UnBind();
    s_Data.m_DirectShadowBuffer->UnBind();
  }
  if(shadowsEnabled && !LightManager::PointLightEmpty())
  {
    GABGL_PROFILE_SCOPE("OMNI SHADOW PASS");

    struct ShadowCandidate
    {
      float distanceSquared;
      uint32_t lightIndex;
    };

    const auto& pointLights = LightManager::GetPointLightPositions();
    std::vector<ShadowCandidate> candidates;
    candidates.reserve(std::min(pointLights.size(), static_cast<size_t>(RendererData::MaxOmniShadowLayers)));

    const glm::vec3 cameraPosition = Camera::GetPosition();
    const glm::mat4 viewProjection = Camera::GetViewProjection();
    const size_t supportedLightCount = std::min(pointLights.size(), static_cast<size_t>(RendererData::MaxOmniShadowLayers));
    for (size_t i = 0; i < supportedLightCount; ++i)
    {
      if (!SphereIntersectsFrustum(viewProjection, pointLights[i], RendererData::PointShadowRadius))
        continue;

      const glm::vec3 toLight = pointLights[i] - cameraPosition;
      candidates.push_back({glm::dot(toLight, toLight), static_cast<uint32_t>(i)});
    }

    std::ranges::sort(candidates, [](const ShadowCandidate& lhs, const ShadowCandidate& rhs)
    {
	    return lhs.distanceSquared < rhs.distanceSquared;
    });
    if (candidates.size() > RendererData::MaxShadowedPointLights)
      candidates.resize(RendererData::MaxShadowedPointLights);

    s_Data.m_PointShadowMask = 0;
    s_Data.m_OmniDirectShadowBuffer->Bind();
    s_Data.s_Shaders.OmniDirectShadowShader->Bind();
    glBindVertexArray(ModelManager::GetModelsVAO());
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, s_Data.m_cmdBufer);

    const auto& directions = s_Data.m_OmniDirectShadowBuffer->GetFaceDirections();
    for (const ShadowCandidate& candidate : candidates)
    {
      const uint32_t lightIndex = candidate.lightIndex;
      const glm::vec3& light = pointLights[lightIndex];
      s_Data.m_PointShadowMask |= 1 << lightIndex;
      s_Data.s_Shaders.OmniDirectShadowShader->SetVec3("gLightWorldPos",light);

      for (size_t face = 0; face < directions.size(); ++face)
      {
        if (!CubemapFaceCanContainVisibleReceiver(light, cameraPosition, directions[face].Target,
            RendererData::PointShadowRadius))
          continue;

        s_Data.m_OmniDirectShadowBuffer->BindCubemapFaceForWriting(lightIndex, face);
        glClearColor(20.0f, 20.0f, 20.0f, 20.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(light,light + directions[face].Target,directions[face].Up);
        s_Data.s_Shaders.OmniDirectShadowShader->SetMat4("u_LightViewProjection", s_Data.m_OmniDirectShadowBuffer->GetShadowProj() * view);

        glMultiDrawElementsIndirect(GL_TRIANGLES,GL_UNSIGNED_INT,NULL,s_Data.m_DrawCommands.size(),0);
      }
    }
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER,0);
    glBindVertexArray(0);
    s_Data.s_Shaders.OmniDirectShadowShader->UnBind();
    s_Data.m_OmniDirectShadowBuffer->UnBind();
  }
  else
  {
    s_Data.m_PointShadowMask = 0;
  }

  UpdateModelFrustumCulling();
  {
    GABGL_PROFILE_SCOPE("GEOMETRY PASS");

    s_Data.m_GeometryBuffer->Bind();


    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
  	glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    s_Data.s_Shaders.GeometryShader->Bind();
    BeginScene();
    glBindVertexArray(ModelManager::GetModelsVAO());
    ModelManager::BindVisibleInstanceTransforms();
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, s_Data.m_CulledCmdBuffer);
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, NULL, s_Data.m_CulledDrawCommands.size(), 0);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    glBindVertexArray(0);
    EndScene();
    s_Data.s_Shaders.GeometryShader->UnBind();

    s_Data.m_GeometryBuffer->UnBind();
    s_Data.m_GeometryBuffer->BlitDepthTo(s_Data.m_ResultBuffer);
  }
  {
    GABGL_PROFILE_SCOPE("LIGHT PASS");


    s_Data.m_BloomBuffer->Bind();
	glDisable(GL_DEPTH_TEST);
  	glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    s_Data.m_GeometryBuffer->BindPositionTextureForReading(GL_TEXTURE1); 
    s_Data.m_GeometryBuffer->BindNormalTextureForReading(GL_TEXTURE2); 
    s_Data.m_GeometryBuffer->BindAlbedoTextureForReading(GL_TEXTURE3); 
    s_Data.m_DirectShadowBuffer->BindShadowTextureForReading(GL_TEXTURE4);
    s_Data.m_DirectShadowBuffer->BindOffsetTextureForReading(GL_TEXTURE5);
    s_Data.m_OmniDirectShadowBuffer->BindShadowTextureForReading(GL_TEXTURE6);

    s_Data.s_Shaders.LightShader->Bind();
    s_Data.s_Shaders.LightShader->SetInt("gPosition", 1);
    s_Data.s_Shaders.LightShader->SetInt("gNormal", 2);
    s_Data.s_Shaders.LightShader->SetInt("gAlbedoSpec", 3);
    s_Data.s_Shaders.LightShader->SetInt("u_DirectShadow", 4);
    s_Data.s_Shaders.LightShader->SetInt("u_OffsetTexture", 5);
    s_Data.s_Shaders.LightShader->SetInt("u_OmniShadow", 6);
    s_Data.s_Shaders.LightShader->SetInt("u_ShadowsEnabled", shadowsEnabled ? 1 : 0);
    s_Data.s_Shaders.LightShader->SetInt("u_PointShadowMask", s_Data.m_PointShadowMask);
    s_Data.s_Shaders.LightShader->SetMat4("u_DirectShadowViewProj", s_Data.m_DirectShadowBuffer->GetShadowViewProj());

    DrawFullscreenQuad();

    s_Data.s_Shaders.LightShader->UnBind();
    s_Data.m_BloomBuffer->UnBind();
  }
  {
    GABGL_PROFILE_SCOPE("BLOOM PASS");

    const GraphicsQuality bloomQuality = Settings::GetBloomQuality();
    if (bloomQuality != GraphicsQuality::Off)
    {
      const uint32_t bloomMipCount = bloomQuality == GraphicsQuality::Low ? 3u
        : bloomQuality == GraphicsQuality::Medium ? 5u
        : 6u;
      s_Data.m_BloomBuffer->RenderBloomTexture(0.005f, bloomMipCount);
    }

    s_Data.m_ResultBuffer->ClearAttachment(1, -1);
    s_Data.m_BloomBuffer->CompositeTo(s_Data.m_ResultBuffer, bloomQuality != GraphicsQuality::Off);
  }
  {
    GABGL_PROFILE_SCOPE("FORWARD PASS");

    s_Data.m_ResultBuffer->Bind();
    s_Data.m_ResultBuffer->SetDrawBuffer(0);

    Renderer::DrawSkybox("night");
    Renderer::DrawPhysicsDebug();
    ParticleRenderer::UpdateAndRender(dt);

    s_Data.m_ResultBuffer->SetDrawBuffers();
    s_Data.m_ResultBuffer->UnBind();
  }
  {
    GABGL_PROFILE_SCOPE("SCENE RESULT PASS");

    uint32_t finalTexture = s_Data.m_ResultBuffer->GetColorAttachmentRendererID();

    switch (s_Data.m_SceneState)
    {
     case RendererData::SceneState::Edit:
     {
       Renderer::DrawEditorFrameBuffer(finalTexture);
       break;
     }
     case RendererData::SceneState::Play:
     {
       Renderer::DrawFramebuffer(finalTexture);
       break;
     }
    }
  }
  {
    GABGL_PROFILE_SCOPE("UI PASS");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, Window::GetWidth(), Window::GetHeight());
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    BeginScene();
    Renderer::DrawText(FontManager::GetFont("dpcomic"), "FPS: " + std::to_string(dt.GetFPS()), glm::vec2(100.0f, 50.0f), 0.5f, glm::vec4(1.0f));
    EndScene();
  }
}

void Renderer::DrawLoadingScreen()
{
  BeginScene();
  Renderer::DrawText(FontManager::GetFont("dpcomic"),"LOADING", glm::vec2(Window::GetWidth() / 2,Window::GetHeight() / 2), 1.0f, glm::vec4(1.0f));
  EndScene();
}

void Renderer::SwitchRenderState()
{
  if (s_Data.m_SceneState == RendererData::SceneState::Edit)
  {
    Camera::SetMode(CameraMode::PLAYER);
    Window::SetCursorVisible(false);
    s_Data.m_SceneState = RendererData::SceneState::Play;
  }
  else if (s_Data.m_SceneState == RendererData::SceneState::Play)
  {
    Camera::SetMode(CameraMode::ORBITAL);
    Window::SetCursorVisible(true);
    s_Data.m_SceneState = RendererData::SceneState::Edit;
  }
}

void Renderer::SetFullscreen(const std::string& sound, bool windowed)
{
  Window::SetFullscreen(windowed);

  uint32_t width = (uint32_t)Window::GetWidth(); 
  uint32_t height = (uint32_t)Window::GetHeight();

  glm::vec2 newResolution = { width, height };
  s_Data.m_ResolutionUniformBuffer->SetData(&newResolution, sizeof(glm::vec2));

  s_Data.m_GeometryBuffer->Resize(width, height);
  s_Data.m_BloomBuffer->Resize(width, height);
  s_Data.m_ResultBuffer->Resize(width, height);
  Camera::SetViewportSize(width, height);
  AudioManager::PlaySound(sound);
}

void Renderer::ApplyDisplaySettings()
{
  Window::SetWindowMode(
    Settings::GetWindowMode(),
    Settings::GetWindowWidth(),
    Settings::GetWindowHeight());
  Window::SetVSync(Settings::GetVSync());

  const uint32_t width = Window::GetWidth();
  const uint32_t height = Window::GetHeight();
  const glm::vec2 resolution = {width, height};
  s_Data.m_ResolutionUniformBuffer->SetData(&resolution, sizeof(glm::vec2));
  s_Data.m_GeometryBuffer->Resize(width, height);
  s_Data.m_ResultBuffer->Resize(width, height);
  s_Data.m_BloomBuffer->Resize(width, height);
  Camera::SetViewportSize(width, height);
}

void Renderer::ApplyGraphicsSettings()
{
  const GraphicsQuality quality = Settings::GetShadowQuality();
  const uint32_t qualityValue = static_cast<uint32_t>(quality);
  if (qualityValue == s_Data.m_AppliedShadowQuality &&
      s_Data.m_DirectShadowBuffer && s_Data.m_OmniDirectShadowBuffer)
    return;

  uint32_t directResolution = 512;
  uint32_t omniResolution = 256;
  float filterSize = 2.0f;
  float randomRadius = 1.5f;

  if (quality == GraphicsQuality::Medium)
  {
    directResolution = 2048;
    omniResolution = 512;
    filterSize = 4.0f;
    randomRadius = 2.0f;
  }
  else if (quality == GraphicsQuality::High)
  {
    directResolution = 4096;
    omniResolution = 1024;
    filterSize = 8.0f;
    randomRadius = 3.0f;
  }
  else if (quality == GraphicsQuality::Low)
  {
    directResolution = 1024;
  }

  s_Data.m_DirectShadowBuffer = DirectShadowBuffer::Create(
    directResolution, directResolution, 16.0f, filterSize, randomRadius);
  s_Data.m_OmniDirectShadowBuffer = OmniDirectShadowBuffer::Create(omniResolution, omniResolution);
  s_Data.m_AppliedShadowQuality = qualityValue;
}

void Renderer::DrawPausedFrame()
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, Window::GetWidth(), Window::GetHeight());
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glClear(GL_COLOR_BUFFER_BIT);
  DrawFramebuffer(s_Data.m_ResultBuffer->GetColorAttachmentRendererID());
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::DrawPhysicsDebug()
{
  if (!s_Data.m_PhysicsDebug || !s_Data.s_Shaders.PhysicsDebugShader)
    return;

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glDepthMask(GL_FALSE);
  glDisable(GL_CULL_FACE);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glLineWidth(2.0f);

  s_Data.s_Shaders.PhysicsDebugShader->Bind();
  s_Data.s_Shaders.PhysicsDebugShader->SetVec4("u_Color", glm::vec4(0.15f, 1.0f, 0.35f, 1.0f));
  ModelManager::BindAllInstanceTransforms();
  glBindVertexArray(ModelManager::GetModelsVAO());

  for (const auto& [modelName, commandIndices] : s_Data.m_ModelDrawCommandIndices)
  {
    const auto model = ModelManager::GetModel(modelName);
    if (!model || model->GetPhysXMeshType() != MeshType::CONVEXMESH)
      continue;

    const GLsizei instanceCount = static_cast<GLsizei>(model->m_InstanceTransforms.size());
    for (const size_t commandIndex : commandIndices)
    {
      const auto& command = s_Data.m_DrawCommands[commandIndex];
      glDrawElementsInstancedBaseVertexBaseInstance(
        GL_TRIANGLES,
        static_cast<GLsizei>(command.count),
        GL_UNSIGNED_INT,
        reinterpret_cast<const void*>(static_cast<uintptr_t>(command.firstIndex) * sizeof(GLuint)),
        instanceCount,
        command.baseVertex,
        model->m_InstanceBase);
    }
  }

  glBindVertexArray(0);
  s_Data.s_Shaders.PhysicsDebugShader->UnBind();

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_CULL_FACE);
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LESS);
  glLineWidth(s_Data.LineWidth);
}

void Renderer::BeginScene()
{
	s_Data.m_CameraBuffer.ViewProjection = Camera::GetViewProjection();
	s_Data.m_CameraBuffer.OrtoProjection = Camera::GetOrtoProjection();
  s_Data.m_CameraBuffer.NonRotViewProjection = Camera::GetNonRotationViewProjection();
  s_Data.m_CameraBuffer.CameraPos = Camera::GetPosition();
	s_Data.m_CameraUniformBuffer->SetData(&s_Data.m_CameraBuffer, sizeof(CameraData));

	StartBatch();
}

void Renderer::EndScene()
{
	Flush();
}

void Renderer::Flush()
{
	if (s_Data.QuadIndexCount)
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.QuadVertexBufferPtr - (uint8_t*)s_Data.QuadVertexBufferBase);
		s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, dataSize);

		for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
			s_Data.TextureSlots[i]->Bind(i);

		s_Data.s_Shaders.QuadShader->Bind();
    s_Data.s_Shaders.QuadShader->SetBool("u_Is3D", s_Data.Is3D);
		Renderer::DrawIndexed(s_Data.QuadVertexArray, s_Data.QuadIndexCount);
	}
	if (s_Data.LineVertexCount)
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.LineVertexBufferPtr - (uint8_t*)s_Data.LineVertexBufferBase);
		s_Data.LineVertexBuffer->SetData(s_Data.LineVertexBufferBase, dataSize);

		s_Data.s_Shaders.LineShader->Bind();
		Renderer::SetLineWidth(s_Data.LineWidth);
		Renderer::DrawLines(s_Data.LineVertexArray, s_Data.LineVertexCount);
	}
}

void Renderer::StartBatch()
{
	s_Data.QuadIndexCount = 0;
	s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;
	s_Data.TextureSlotIndex = 1;

	s_Data.LineVertexCount = 0;
	s_Data.LineVertexBufferPtr = s_Data.LineVertexBufferBase;
}

void Renderer::NextBatch()
{
	Flush();
	StartBatch();
}

void Renderer::DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, int entityID)
{
	s_Data.LineVertexBufferPtr->Position = p0;
	s_Data.LineVertexBufferPtr->Color = color;
	s_Data.LineVertexBufferPtr->EntityID = entityID;
	s_Data.LineVertexBufferPtr++;

	s_Data.LineVertexBufferPtr->Position = p1;
	s_Data.LineVertexBufferPtr->Color = color;
	s_Data.LineVertexBufferPtr->EntityID = entityID;
	s_Data.LineVertexBufferPtr++;

	s_Data.LineVertexCount += 2;
}

void Renderer::DrawQuad(const glm::vec3& position, const glm::vec3& size, const glm::vec3& rotation, const glm::vec4& color)
{
  glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
    * glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1, 0, 0))
    * glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0, 1, 0))
    * glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0, 0, 1))
    * glm::scale(glm::mat4(1.0f), size);

	DrawQuad(transform, color);
}

void Renderer::DrawQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
{
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f))
		* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
		* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

	DrawQuad(transform, color);
}

void Renderer::DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID)
{
	const float textureIndex = 0.0f; // White Texture

	if (s_Data.QuadIndexCount >= RendererData::MaxIndices) NextBatch();

  glm::vec3 position = glm::vec3(transform[3]);

  float sizeX = glm::length(glm::vec3(transform[0]));
  float sizeY = glm::length(glm::vec3(transform[1]));

  glm::vec3 cameraRight = Camera::GetRightDirection();
  glm::vec3 cameraUp = Camera::GetUpDirection();

	for (size_t i = 0; i < s_Data.quadVertexCount; i++)
	{
    if(s_Data.Is3D)
    {
      glm::vec3 worldPos =
          position +
          cameraRight * (s_Data.QuadVertexPositions[i].x * sizeX) +
          cameraUp    * (s_Data.QuadVertexPositions[i].y * sizeY);

      s_Data.QuadVertexBufferPtr->Position = worldPos;
    }
    else s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];

    s_Data.QuadVertexBufferPtr->Color = color;
    s_Data.QuadVertexBufferPtr->TexCoord = s_Data.tex3DCoords[i];
    s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
    s_Data.QuadVertexBufferPtr->TilingFactor = s_Data.tilingFactor;
    s_Data.QuadVertexBufferPtr->EntityID = entityID;
    s_Data.QuadVertexBufferPtr++;
	}

	s_Data.QuadIndexCount += 6;
}

void Renderer::DrawQuad(const glm::vec3& position, const glm::vec3& size, const glm::vec3& rotation, const std::shared_ptr<Texture>& texture, const glm::vec4& tintColor)
{
  glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
    * glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1, 0, 0))
    * glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0, 1, 0))
    * glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0, 0, 1))
    * glm::scale(glm::mat4(1.0f), size);

	DrawQuad(transform, texture, tintColor, 1.0f);
}

void Renderer::DrawQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const std::shared_ptr<Texture>& texture, const glm::vec4& tintColor)
{
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f))
		* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
		* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

	DrawQuad(transform, texture, tintColor, 1.0f);
}

void Renderer::DrawQuad(const glm::mat4& transform, const std::shared_ptr<Texture>& texture, const glm::vec4& tintColor, float tilingFactor, int entityID)
{
	if (s_Data.QuadIndexCount >= RendererData::MaxIndices) NextBatch();

	float textureIndex = 0.0f;
	for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
	{
		if (*s_Data.TextureSlots[i] == *texture)
		{
			textureIndex = (float)i;
			break;
		}
	}

	if (textureIndex == 0.0f)
	{
		if (s_Data.TextureSlotIndex >= RendererData::MaxTextureSlots) NextBatch();

		textureIndex = (float)s_Data.TextureSlotIndex;
		s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
		s_Data.TextureSlotIndex++;
	}

	for (size_t i = 0; i < s_Data.quadVertexCount; i++)
	{
		s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
		s_Data.QuadVertexBufferPtr->Color = tintColor;
		s_Data.QuadVertexBufferPtr->TexCoord = s_Data.tex2DCoords[i];
		s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data.QuadVertexBufferPtr->TilingFactor = s_Data.tilingFactor;
		s_Data.QuadVertexBufferPtr->EntityID = entityID;
		s_Data.QuadVertexBufferPtr++;
	}

	s_Data.QuadIndexCount += 6;
}

void Renderer::DrawQuadContour(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, int entityID)
{
	glm::vec3 p0 = glm::vec3(position.x - size.x * 0.5f, position.y - size.y * 0.5f, position.z);
	glm::vec3 p1 = glm::vec3(position.x + size.x * 0.5f, position.y - size.y * 0.5f, position.z);
	glm::vec3 p2 = glm::vec3(position.x + size.x * 0.5f, position.y + size.y * 0.5f, position.z);
	glm::vec3 p3 = glm::vec3(position.x - size.x * 0.5f, position.y + size.y * 0.5f, position.z);

	DrawLine(p0, p1, color, entityID);
	DrawLine(p1, p2, color, entityID);
	DrawLine(p2, p3, color, entityID);
	DrawLine(p3, p0, color, entityID);
}

void Renderer::DrawQuadContour(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color, int entityID)
{
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(position,0.0f))
		* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
		* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

	DrawQuadContour(transform, color);
}

void Renderer::DrawQuadContour(const glm::mat4& transform, const glm::vec4& color, int entityID)
{
	glm::vec3 lineVertices[4];
	for (size_t i = 0; i < 4; i++) lineVertices[i] = transform * s_Data.QuadVertexPositions[i];

	DrawLine(lineVertices[0], lineVertices[1], color, entityID);
	DrawLine(lineVertices[1], lineVertices[2], color, entityID);
	DrawLine(lineVertices[2], lineVertices[3], color, entityID);
	DrawLine(lineVertices[3], lineVertices[0], color, entityID);
}

void Renderer::DrawCube(const glm::vec3& position, const glm::vec3& size, const std::shared_ptr<Texture>& texture, const glm::vec4& tintColor, int entityID)
{
  Renderer::Set3D(true);

  GLint prevFrontFace;
  glGetIntegerv(GL_FRONT_FACE, &prevFrontFace);

  glEnable(GL_CULL_FACE);
  glCullFace(GL_FRONT);
  glFrontFace(GL_CCW); // Only cubes use counter-clockwise winding

  glm::vec2 xy = { size.x, size.y };
	glm::vec2 yz = { size.z, size.y };
	glm::vec2 xz = { size.x, size.z };

	float halfX = size.x / 2.0f;
	float halfY = size.y / 2.0f;
	float halfZ = size.z / 2.0f;

	// FRONT (+Z)
	DrawQuad(
		glm::translate(glm::mat4(1.0f), position + glm::vec3(0.0f, 0.0f, +halfZ)) *
		glm::scale(glm::mat4(1.0f), glm::vec3(xy, 1.0f)),texture,tintColor);

	// BACK (-Z)
	DrawQuad(
		glm::translate(glm::mat4(1.0f), position + glm::vec3(0.0f, 0.0f, -halfZ)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), { 0, 1, 0 }) *
		glm::scale(glm::mat4(1.0f), glm::vec3(xy, 1.0f)),texture,tintColor);

	// LEFT (-X)
	DrawQuad(
		glm::translate(glm::mat4(1.0f), position + glm::vec3(-halfX, 0.0f, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), { 0, 1, 0 }) *
		glm::scale(glm::mat4(1.0f), glm::vec3(yz, 1.0f)),texture,tintColor);

	// RIGHT (+X)
	DrawQuad(
		glm::translate(glm::mat4(1.0f), position + glm::vec3(+halfX, 0.0f, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), { 0, 1, 0 }) *
		glm::scale(glm::mat4(1.0f), glm::vec3(yz, 1.0f)),texture,tintColor);

	// TOP (+Y)
	DrawQuad(
		glm::translate(glm::mat4(1.0f), position + glm::vec3(0.0f, +halfY, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), { 1, 0, 0 }) *
		glm::scale(glm::mat4(1.0f), glm::vec3(xz, 1.0f)),texture,tintColor);

	// BOTTOM (-Y)
	DrawQuad(
		glm::translate(glm::mat4(1.0f), position + glm::vec3(0.0f, -halfY, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), { 1, 0, 0 }) *
		glm::scale(glm::mat4(1.0f), glm::vec3(xz, 1.0f)),texture,tintColor);

  glFrontFace(prevFrontFace);
  glDisable(GL_CULL_FACE);

  Renderer::Set3D(false);
}

void Renderer::DrawCube(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color, int entityID)
{
  Renderer::Set3D(true);

  GLint prevFrontFace;
  glGetIntegerv(GL_FRONT_FACE, &prevFrontFace);

  glEnable(GL_CULL_FACE);
  glCullFace(GL_FRONT);
  glFrontFace(GL_CCW); // Only cubes use counter-clockwise winding

  glm::vec2 xy = { size.x, size.y };
	glm::vec2 yz = { size.z, size.y };
	glm::vec2 xz = { size.x, size.z };

	float halfX = size.x / 2.0f;
	float halfY = size.y / 2.0f;
	float halfZ = size.z / 2.0f;

	// FRONT (+Z)
	DrawQuad(
		glm::translate(glm::mat4(1.0f), position + glm::vec3(0.0f, 0.0f, +halfZ)) *
		glm::scale(glm::mat4(1.0f), glm::vec3(xy, 1.0f)), color);

	// BACK (-Z)
	DrawQuad(
		glm::translate(glm::mat4(1.0f), position + glm::vec3(0.0f, 0.0f, -halfZ)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), { 0, 1, 0 }) *
		glm::scale(glm::mat4(1.0f), glm::vec3(xy, 1.0f)), color);

	// LEFT (-X)
	DrawQuad(
		glm::translate(glm::mat4(1.0f), position + glm::vec3(-halfX, 0.0f, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), { 0, 1, 0 }) *
		glm::scale(glm::mat4(1.0f), glm::vec3(yz, 1.0f)), color);

	// RIGHT (+X)
	DrawQuad(
		glm::translate(glm::mat4(1.0f), position + glm::vec3(+halfX, 0.0f, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), { 0, 1, 0 }) *
		glm::scale(glm::mat4(1.0f), glm::vec3(yz, 1.0f)), color);

	// TOP (+Y)
	DrawQuad(
		glm::translate(glm::mat4(1.0f), position + glm::vec3(0.0f, +halfY, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), { 1, 0, 0 }) *
		glm::scale(glm::mat4(1.0f), glm::vec3(xz, 1.0f)), color);

	// BOTTOM (-Y)
	DrawQuad(
		glm::translate(glm::mat4(1.0f), position + glm::vec3(0.0f, -halfY, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), { 1, 0, 0 }) *
		glm::scale(glm::mat4(1.0f), glm::vec3(xz, 1.0f)), color);

  glFrontFace(prevFrontFace);
  glDisable(GL_CULL_FACE);

  Renderer::Set3D(false);
}

void Renderer::DrawCubeContour(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color, int entityID)
{
  glm::vec3 half = size * 0.5f;

  // 8 cube vertices relative to center position
  glm::vec3 v0 = position + glm::vec3(-half.x, -half.y, -half.z); // left bottom back
  glm::vec3 v1 = position + glm::vec3( half.x, -half.y, -half.z); // right bottom back
  glm::vec3 v2 = position + glm::vec3( half.x,  half.y, -half.z); // right top back
  glm::vec3 v3 = position + glm::vec3(-half.x,  half.y, -half.z); // left top back

  glm::vec3 v4 = position + glm::vec3(-half.x, -half.y,  half.z); // left bottom front
  glm::vec3 v5 = position + glm::vec3( half.x, -half.y,  half.z); // right bottom front
  glm::vec3 v6 = position + glm::vec3( half.x,  half.y,  half.z); // right top front
  glm::vec3 v7 = position + glm::vec3(-half.x,  half.y,  half.z); // left top front

  // Bottom square
  DrawLine(v0, v1, color, entityID);
  DrawLine(v1, v2, color, entityID);
  DrawLine(v2, v3, color, entityID);
  DrawLine(v3, v0, color, entityID);

  // Top square
  DrawLine(v4, v5, color, entityID);
  DrawLine(v5, v6, color, entityID);
  DrawLine(v6, v7, color, entityID);
  DrawLine(v7, v4, color, entityID);

  // Vertical edges
  DrawLine(v0, v4, color, entityID);
  DrawLine(v1, v5, color, entityID);
  DrawLine(v2, v6, color, entityID);
  DrawLine(v3, v7, color, entityID);
}

void Renderer::DrawFullscreenQuad()
{
  static GLuint quadVAO = 0, quadVBO = 0;
  if (quadVAO == 0)
  {
    float quadVertices[] = {
        // positions        // tex Coords
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };

    glCreateVertexArrays(1, &quadVAO);
    glCreateBuffers(1, &quadVBO);

    glNamedBufferData(quadVBO, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glVertexArrayVertexBuffer(quadVAO, 0, quadVBO, 0, 5 * sizeof(float));

    glEnableVertexArrayAttrib(quadVAO, 0);
    glVertexArrayAttribFormat(quadVAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(quadVAO, 0, 0);

    glEnableVertexArrayAttrib(quadVAO, 1);
    glVertexArrayAttribFormat(quadVAO, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glVertexArrayAttribBinding(quadVAO, 1, 0);
  }

  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindVertexArray(0);
}

void Renderer::DrawFramebuffer(uint32_t textureID)
{
  static GLuint quadVAO = 0, quadVBO = 0;
  if (quadVAO == 0)
  {
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glCreateVertexArrays(1, &quadVAO);

    glCreateBuffers(1, &quadVBO);
    glNamedBufferData(quadVBO, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glVertexArrayVertexBuffer(quadVAO, 0, quadVBO, 0, 4 * sizeof(float));

    glEnableVertexArrayAttrib(quadVAO, 0);
    glVertexArrayAttribFormat(quadVAO, 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(quadVAO, 0, 0);

    glEnableVertexArrayAttrib(quadVAO, 1);
    glVertexArrayAttribFormat(quadVAO, 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
    glVertexArrayAttribBinding(quadVAO, 1, 0);
  }

  s_Data.s_Shaders.FramebufferShader->Bind();
  s_Data.s_Shaders.FramebufferShader->SetInt("u_Texture", 0);

  glBindTextureUnit(0, textureID);

  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);
  s_Data.s_Shaders.FramebufferShader->UnBind();
}

void Renderer::BakeSkyboxTextures(const std::string& name, const std::shared_ptr<Texture>& cubemap)
{
  Timer timer;

  auto channels = cubemap->GetChannels();
  auto width = cubemap->GetWidth();
  auto height = cubemap->GetHeight();
  auto pixels = cubemap->GetPixels();

  GLuint rendererID = 0;
  glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &rendererID);
  GABGL_ASSERT(rendererID != 0, "Failed to create cube map texture!");

  cubemap->SetRendererID(rendererID);

  GLenum internalFormat = (channels == 4) ? GL_RGBA8 : GL_RGB8;
  GLenum dataFormat     = (channels == 4) ? GL_RGBA  : GL_RGB;

  glTextureStorage2D(rendererID, 1, internalFormat, width, height);

  for (int i = 0; i < 6; ++i)
  {
      glTextureSubImage3D(
          rendererID,
          0,                // mip level
          0, 0, i,          // x, y, z offset — z=i for cube face
          width, height, 1, // width, height, depth (1 face)
          dataFormat,
          GL_UNSIGNED_BYTE,
          pixels[i]
      );

      stbi_image_free(pixels[i]);
      pixels[i] = nullptr;
  }

  glTextureParameteri(rendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTextureParameteri(rendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTextureParameteri(rendererID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTextureParameteri(rendererID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTextureParameteri(rendererID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  glGenerateTextureMipmap(rendererID);

  s_Data.skyboxes[name] = std::move(cubemap);

  GABGL_WARN("Skybox uploading took {0} ms", timer.ElapsedMillis());
}

void Renderer::DrawSkybox(const std::string& name)
{
  static uint32_t SkyboxVAO = 0, SkyboxVBO = 0;
  if (SkyboxVAO == 0)
  {
    float skyboxVertices[] =
    {
        -1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f
    };

    glCreateVertexArrays(1, &SkyboxVAO);
    glCreateBuffers(1, &SkyboxVBO);
    glNamedBufferData(SkyboxVBO, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

    glVertexArrayVertexBuffer(SkyboxVAO, 0, SkyboxVBO, 0, 3 * sizeof(float));
    glEnableVertexArrayAttrib(SkyboxVAO, 0);
    glVertexArrayAttribFormat(SkyboxVAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(SkyboxVAO, 0, 0);
  }

  glDepthFunc(GL_LEQUAL);
  glDepthMask(GL_FALSE);    
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

  s_Data.s_Shaders.skyboxShader->Bind();

  auto it = s_Data.skyboxes.find(name);
  if (it != s_Data.skyboxes.end())
  {
    glBindTextureUnit(0, it->second->GetRendererID());  
  }
  else
  {
    GABGL_ERROR("Skybox texture not found: {}",name);
    return;
  }

  glBindVertexArray(SkyboxVAO);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);

  glDepthFunc(GL_LESS);
  glDepthMask(GL_TRUE);
}

void Renderer::DrawText(const Font* font, const std::string& text, const glm::vec3& position, const glm::vec3& rotation, float size, const glm::vec4& color)
{
  Renderer::Set3D(true);
  DrawText(font, text, position, rotation, size, color, -1);
  Renderer::Set3D(false);
}

void Renderer::DrawText(const Font* font, const std::string& text, const glm::vec2& position, float size, const glm::vec4& color)
{
  DrawText(font, text, glm::vec3(position, 0.0f), glm::vec3(0.0f), size, color, -1);
}

void Renderer::DrawText(const Font* font, const std::string& text, const glm::vec3& position, const glm::vec3& rotation, float size, const glm::vec4& color, int entityID)
{
  if (!font || font->m_Characters.empty() || text.empty())
  {
      GABGL_ERROR("Font is nullptr, empty, or text is empty");
      return;
  }

  float textWidth = 0.0f;
  float maxBearingY = 0.0f;
  float maxBelowBaseline = 0.0f;

  // First pass: calculate dimensions
  for (char c : text)
  {
    auto it = font->m_Characters.find(c);
    if (it == font->m_Characters.end()) continue;

    const auto& ch = it->second;
    textWidth += (ch.Advance >> 6) * size;

    float bearingY = ch.Bearing.y * size;
    float belowBaseline = (ch.Size.y - ch.Bearing.y) * size;

    maxBearingY = std::max(maxBearingY, bearingY);
    maxBelowBaseline = std::max(maxBelowBaseline, belowBaseline);
  }

  float totalHeight = maxBearingY + maxBelowBaseline;
  float originX = -textWidth * 0.5f;
  float originY = -totalHeight * 0.5f;

  glm::vec3 cursor = glm::vec3(originX, originY, 0.0f);

  // Precompute global transform
  glm::mat4 baseTransform = glm::translate(glm::mat4(1.0f), position) *
                            glm::rotate(glm::mat4(1.0f), rotation.x, {1, 0, 0}) *
                            glm::rotate(glm::mat4(1.0f), rotation.y, {0, 1, 0}) *
                            glm::rotate(glm::mat4(1.0f), rotation.z, {0, 0, 1});

  std::unordered_map<uint32_t, float> textureSlotCache;

  for (char c : text)
  {
    auto it = font->m_Characters.find(c);
    if (it == font->m_Characters.end()) continue;

    const Character& ch = it->second;

    if (s_Data.QuadIndexCount >= RendererData::MaxIndices)
        NextBatch();

    float xpos = cursor.x + ch.Bearing.x * size;
    float ypos = cursor.y + (maxBearingY - ch.Bearing.y * size);
    float w = ch.Size.x * size;
    float h = ch.Size.y * size;

    glm::mat4 localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(xpos, ypos, 0.0f)) *
                               glm::scale(glm::mat4(1.0f), glm::vec3(w, h, 1.0f));

    glm::mat4 transform = baseTransform * localTransform;

    float textureIndex = 0.0f;
    auto found = textureSlotCache.find(ch.TextureID);
    if (found != textureSlotCache.end()) {
        textureIndex = found->second;
    } else {
        if (s_Data.TextureSlotIndex >= RendererData::MaxTextureSlots)
            NextBatch();

        textureIndex = static_cast<float>(s_Data.TextureSlotIndex);
        s_Data.TextureSlots[s_Data.TextureSlotIndex++] = Texture::WrapExisting(ch.TextureID);
        textureSlotCache[ch.TextureID] = textureIndex;
    }

    for (int i = 0; i < 4; i++) {
        s_Data.QuadVertexBufferPtr->Position = transform * glm::vec4(s_Data.quadPositions[i], 1.0f);
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = s_Data.tex3DCoords[i];
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = 1.0f;
        s_Data.QuadVertexBufferPtr->EntityID = entityID;
        s_Data.QuadVertexBufferPtr++;
    }

    s_Data.QuadIndexCount += 6;
    cursor.x += (ch.Advance >> 6) * size;
  }
}

void Renderer::AddDrawCommand(const std::string& modelName, uint32_t verticesSize, uint32_t indicesSize)
{
  DrawElementsIndirectCommand cmd =
  {
    .count = static_cast<GLuint>(indicesSize),
    .instanceCount = 1,
    .firstIndex = static_cast<GLuint>(s_Data.m_DrawIndexOffset),
    .baseVertex = static_cast<GLint>(s_Data.m_DrawVertexOffset),
    .baseInstance = 0, 
  };

  s_Data.m_ModelDrawCommandIndices[modelName].push_back(s_Data.m_DrawCommands.size()); // store index
  s_Data.m_DrawCommands.push_back(cmd);

  s_Data.m_DrawIndexOffset += indicesSize;
  s_Data.m_DrawVertexOffset += verticesSize;
}

void Renderer::RebuildDrawCommandsForModel(const std::shared_ptr<Model>& model, bool render)
{
  model->m_IsRendered = render;
  UpdateDrawCommandInstances(model);
}

void Renderer::UpdateModelFrustumCulling()
{
  if (s_Data.m_DrawCommands.empty() || s_Data.m_CulledCmdBuffer == 0)
    return;

  const Frustum frustum(Camera::GetViewProjection());
  auto& visibleTransforms = s_Data.m_VisibleInstanceTransforms;
  visibleTransforms.clear();
  s_Data.m_CulledDrawCommands = s_Data.m_DrawCommands;
  s_Data.m_VisibleInstanceCount = 0;
  s_Data.m_RenderableInstanceCount = 0;

  for (const std::string& modelName : ModelManager::GetModelNames())
  {
    const auto model = ModelManager::GetModel(modelName);
    const auto commandIndices = s_Data.m_ModelDrawCommandIndices.find(modelName);
    if (!model || commandIndices == s_Data.m_ModelDrawCommandIndices.end())
      continue;

    const GLuint visibleBase = static_cast<GLuint>(visibleTransforms.size());
    GLuint visibleCount = 0;
    if (model->m_IsRendered)
    {
      s_Data.m_RenderableInstanceCount += static_cast<uint32_t>(model->m_InstanceTransforms.size());
      const float localRadius = std::max(model->GetBoundsRadius(), 0.001f);
      for (const glm::mat4& transform : model->m_InstanceTransforms)
      {
        const glm::vec3 worldCenter = glm::vec3(transform * glm::vec4(model->GetBoundsCenter(), 1.0f));
        const float maxScale = std::max({
          glm::length(glm::vec3(transform[0])),
          glm::length(glm::vec3(transform[1])),
          glm::length(glm::vec3(transform[2]))
        });
        const float worldRadius = localRadius * maxScale;
        if (!frustum.IntersectsSphere(worldCenter, worldRadius))
          continue;

        visibleTransforms.push_back(transform);
        ++visibleCount;
      }
    }

    s_Data.m_VisibleInstanceCount += visibleCount;
    for (const size_t commandIndex : commandIndices->second)
    {
      auto& command = s_Data.m_CulledDrawCommands[commandIndex];
      command.instanceCount = visibleCount;
      command.baseInstance = visibleBase;
    }
  }

  ModelManager::UploadVisibleInstanceTransforms(visibleTransforms);
  glNamedBufferSubData(s_Data.m_CulledCmdBuffer, 0,
    static_cast<GLsizeiptr>(s_Data.m_CulledDrawCommands.size() * sizeof(DrawElementsIndirectCommand)),
    s_Data.m_CulledDrawCommands.data());
}

void Renderer::UpdateDrawCommandInstances(const std::shared_ptr<Model>& model)
{
  const auto commandIndices = s_Data.m_ModelDrawCommandIndices.find(model->m_Name);
  if (commandIndices == s_Data.m_ModelDrawCommandIndices.end())
    return;

  const GLuint instanceCount = model->m_IsRendered
    ? static_cast<GLuint>(model->m_InstanceTransforms.size())
    : 0;

  for (const size_t commandIndex : commandIndices->second)
  {
    auto& command = s_Data.m_DrawCommands[commandIndex];
    command.instanceCount = instanceCount;
    command.baseInstance = model->m_InstanceBase;
  }

  const size_t requiredSize = s_Data.m_DrawCommands.size() * sizeof(DrawElementsIndirectCommand);
  if (s_Data.m_cmdBufer != 0 && requiredSize <= s_Data.m_cmdBufferSize)
  {
    glNamedBufferSubData(
      s_Data.m_cmdBufer,
      0,
      requiredSize,
      s_Data.m_DrawCommands.data());
  }
}

void Renderer::InitDrawCommandBuffer()
{
  if (s_Data.m_DrawCommands.empty())
    return;

  if (s_Data.m_cmdBufer != 0)
    glDeleteBuffers(1, &s_Data.m_cmdBufer);
  if (s_Data.m_CulledCmdBuffer != 0)
    glDeleteBuffers(1, &s_Data.m_CulledCmdBuffer);

  s_Data.m_cmdBufferSize = s_Data.m_DrawCommands.size() * sizeof(DrawElementsIndirectCommand);
  s_Data.m_CulledDrawCommands = s_Data.m_DrawCommands;
  glCreateBuffers(1, &s_Data.m_cmdBufer);
  glNamedBufferStorage(s_Data.m_cmdBufer, s_Data.m_cmdBufferSize, s_Data.m_DrawCommands.data(), GL_DYNAMIC_STORAGE_BIT);
  glCreateBuffers(1, &s_Data.m_CulledCmdBuffer);
  glNamedBufferStorage(s_Data.m_CulledCmdBuffer, s_Data.m_cmdBufferSize,
    s_Data.m_CulledDrawCommands.data(), GL_DYNAMIC_STORAGE_BIT);
}

void Renderer::ResetModelDrawCommands()
{
  if (s_Data.m_cmdBufer != 0)
    glDeleteBuffers(1, &s_Data.m_cmdBufer);
  if (s_Data.m_CulledCmdBuffer != 0)
    glDeleteBuffers(1, &s_Data.m_CulledCmdBuffer);

  s_Data.m_cmdBufer = 0;
  s_Data.m_CulledCmdBuffer = 0;
  s_Data.m_cmdBufferSize = 0;
  s_Data.m_DrawCommands.clear();
  s_Data.m_CulledDrawCommands.clear();
  s_Data.m_VisibleInstanceTransforms.clear();
  s_Data.m_ModelDrawCommandIndices.clear();
  s_Data.m_DrawIndexOffset = 0;
  s_Data.m_DrawVertexOffset = 0;
  s_Data.m_VisibleInstanceCount = 0;
  s_Data.m_RenderableInstanceCount = 0;
}

void Renderer::DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount)
{
	vertexArray->Bind();
	uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
	glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
}

void Renderer::DrawLines(const std::shared_ptr<VertexArray>& vertexArray, uint32_t vertexCount)
{
	vertexArray->Bind();
	glDrawArrays(GL_LINES, 0, vertexCount);
}

void Renderer::SetLineWidth(float width)
{
	glLineWidth(width);
}

float Renderer::GetLineWidth()
{
	return s_Data.LineWidth;
}

uint32_t Renderer::GetActiveWidgetID()
{
	return GImGui->ActiveId;
}

void Renderer::BlockEvents(bool block)
{ 
  s_Data.m_BlockEvents = block;
}

void Renderer::Set3D(bool is3D)
{
  if (s_Data.Is3D == is3D)
    return;

  Flush();
  StartBatch();
  s_Data.Is3D = is3D;
}

void Renderer::DrawEditorFrameBuffer(uint32_t framebufferTexture)
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	//ImGuizmo::BeginFrame();
	// Note: Switch this to true to enable dockspace
	static bool dockspaceOpen = true;
	static bool opt_fullscreen_persistant = true;
	bool opt_fullscreen = opt_fullscreen_persistant;
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_NoTabBar;

	// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
	// because it would be confusing to have two docking targets within each others.
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
	if (opt_fullscreen)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	}

	// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
	if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
		window_flags |= ImGuiWindowFlags_NoBackground;

	// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
	// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
	// all active windows docked into it will lose their parent and become undocked.
	// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
	// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
	ImGui::PopStyleVar();
	if (opt_fullscreen)
		ImGui::PopStyleVar(2);

	// DockSpace
	ImGuiIO& io = ImGui::GetIO();
	ImGuiStyle& style = ImGui::GetStyle();
	float minWinSizeX = style.WindowMinSize.x;
	style.WindowMinSize.x = 370.0f;
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	}

	style.WindowMinSize.x = minWinSizeX;

	ImGui::Begin("Scene Hierarchy", nullptr, ImGuiWindowFlags_NoCollapse);

	SceneManager::SyncEditorEntityTransforms();
	const std::string activeSceneName = SceneManager::GetActiveSceneName();
	const auto sceneNames = SceneManager::GetAvailableSceneNames();
	if (ImGui::BeginCombo("Scene", activeSceneName.empty() ? "<none>" : activeSceneName.c_str()))
	{
		for (const auto& sceneName : sceneNames)
		{
			const bool selected = sceneName == activeSceneName;
			if (ImGui::Selectable(sceneName.c_str(), selected) && !selected)
			{
				s_Data.m_SelectedEntityID = 0;
				s_Data.m_SelectedLightID = 0;
				SceneManager::LoadScene(sceneName);
			}
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	static const char* saveStatus = nullptr;
	if (ImGui::Button("Save Scene"))
		saveStatus = SceneManager::SaveActiveScene() ? "Scene saved" : "Save failed";
	ImGui::SameLine();
	if (ImGui::Button("Reload Scene") && !activeSceneName.empty())
	{
		s_Data.m_SelectedEntityID = 0;
		s_Data.m_SelectedLightID = 0;
		SceneManager::LoadScene(activeSceneName);
	}
	if (saveStatus)
		ImGui::TextUnformatted(saveStatus);

	ImGui::Separator();
	uint64_t duplicateRequest = 0;
	for (const auto& entity : SceneManager::GetEntities())
	{
		ImGui::PushID(static_cast<int>(entity.id));
		const bool selected = entity.id == s_Data.m_SelectedEntityID;
		if (ImGui::Selectable(entity.name.c_str(), selected))
		{
			s_Data.m_SelectedEntityID = entity.id;
			s_Data.m_SelectedLightID = 0;
		}

		if (ImGui::BeginPopupContextItem("EntityContext"))
		{
			if (entity.type != "controller" && ImGui::MenuItem("Duplicate / Instance"))
				duplicateRequest = entity.id;
			if (entity.type == "controller")
				ImGui::TextDisabled("Controllers cannot be instanced");
			ImGui::EndPopup();
		}
		ImGui::PopID();
	}

	if (duplicateRequest != 0)
	{
		const uint64_t duplicateID = SceneManager::DuplicateEntity(duplicateRequest);
		if (duplicateID != 0)
		{
			s_Data.m_SelectedEntityID = duplicateID;
			s_Data.m_SelectedLightID = 0;
		}
	}

	ImGui::SeparatorText("Lights");
	static int newLightType = static_cast<int>(LightType::POINT);
	const char* lightTypes[] = { "Directional", "Point", "Spot" };
	ImGui::SetNextItemWidth(150.0f);
	ImGui::Combo("##NewLightType", &newLightType, lightTypes, IM_ARRAYSIZE(lightTypes));
	ImGui::SameLine();
	const bool directLightAlreadyExists = newLightType == static_cast<int>(LightType::DIRECT) &&
		std::any_of(SceneManager::GetLights().begin(), SceneManager::GetLights().end(),
			[](const SceneLight& light) { return light.type == LightType::DIRECT; });
	ImGui::BeginDisabled(directLightAlreadyExists);
	if (ImGui::Button("Add Light"))
	{
		const uint64_t lightID = SceneManager::AddLight(static_cast<LightType>(newLightType));
		if (lightID != 0)
		{
			s_Data.m_SelectedEntityID = 0;
			s_Data.m_SelectedLightID = lightID;
		}
	}
	ImGui::EndDisabled();
	if (directLightAlreadyExists && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		ImGui::SetTooltip("Only one directional light is allowed per scene");

	uint64_t removeLightRequest = 0;
	for (const auto& light : SceneManager::GetLights())
	{
		ImGui::PushID("Light");
		ImGui::PushID(static_cast<int>(light.id));
		const bool selected = light.id == s_Data.m_SelectedLightID;
		if (ImGui::Selectable(light.name.c_str(), selected))
		{
			s_Data.m_SelectedEntityID = 0;
			s_Data.m_SelectedLightID = light.id;
		}
		if (ImGui::BeginPopupContextItem("LightContext"))
		{
			if (ImGui::MenuItem("Remove"))
				removeLightRequest = light.id;
			ImGui::EndPopup();
		}
		ImGui::PopID();
		ImGui::PopID();
	}
	if (removeLightRequest != 0 && SceneManager::RemoveLight(removeLightRequest))
	{
		if (s_Data.m_SelectedLightID == removeLightRequest)
			s_Data.m_SelectedLightID = 0;
	}


	ImGui::End();

	ImGui::Begin("Components", nullptr, ImGuiWindowFlags_NoCollapse);

	if (ImGui::Button("Reload Shaders")) LoadShaders();
	ImGui::SameLine();
	ImGui::Checkbox("Physics Debug", &s_Data.m_PhysicsDebug);
	ImGui::TextDisabled("Frustum culling: %u / %u model instances visible",
		s_Data.m_VisibleInstanceCount, s_Data.m_RenderableInstanceCount);

	if (SceneEntity* entity = SceneManager::FindEntity(s_Data.m_SelectedEntityID))
	{
		ImGui::Separator();
		ImGui::Text("Entity: %s", entity->name.c_str());
		ImGui::Text("Model: %s", entity->model.c_str());
		ImGui::Text("Instance: %u", entity->instanceIndex);

		glm::vec3 position = entity->transform.GetPosition();
		glm::vec3 rotation = entity->transform.GetRotation();
		glm::vec3 scale = entity->transform.GetScale();
		bool transformChanged = false;
		transformChanged |= ImGui::DragFloat3("Position", glm::value_ptr(position), 0.1f);
		transformChanged |= ImGui::DragFloat3("Rotation", glm::value_ptr(rotation), 0.5f);
		transformChanged |= ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.05f);

		if (transformChanged)
			SceneManager::UpdateEntityTransform(entity->id, Transform(position, rotation, scale));

		if (entity->type != "controller" && ImGui::Button("Duplicate / Instance"))
		{
			const uint64_t duplicateID = SceneManager::DuplicateEntity(entity->id);
			if (duplicateID != 0)
				s_Data.m_SelectedEntityID = duplicateID;
		}
	}
	else if (SceneLight* light = SceneManager::FindLight(s_Data.m_SelectedLightID))
	{
		ImGui::Separator();
		const char* typeName = light->type == LightType::DIRECT ? "Directional"
			: light->type == LightType::SPOT ? "Spot" : "Point";
		ImGui::Text("Light: %s", light->name.c_str());
		ImGui::Text("Type: %s", typeName);

		glm::vec3 color = light->color;
		glm::vec3 position = light->position;
		glm::vec3 rotation = light->rotation;
		bool lightChanged = false;
		lightChanged |= ImGui::ColorEdit3("Color", glm::value_ptr(color), ImGuiColorEditFlags_Float);
		if (light->type != LightType::DIRECT)
			lightChanged |= ImGui::DragFloat3("Position", glm::value_ptr(position), 0.1f);
		if (light->type != LightType::POINT)
			lightChanged |= ImGui::DragFloat3("Direction", glm::value_ptr(rotation), 0.05f);

		if (lightChanged)
			SceneManager::UpdateLight(light->id, light->name, color, position, rotation);

		if (ImGui::Button("Remove Light"))
		{
			const uint64_t removedID = light->id;
			if (SceneManager::RemoveLight(removedID))
				s_Data.m_SelectedLightID = 0;
		}
	}

  for (const auto& result : Profiler::GetResults())
  {
    bool gpuBound = result.GPUTime > result.CPUTime;

    ImVec4 color = gpuBound
        ? ImVec4(1, 0.4f, 0.4f, 1)  // red
        : ImVec4(0.4f, 1, 0.4f, 1); // green

    ImGui::TextColored(color,
        "%s: CPU %.3f ms | GPU %.3f ms",
        result.Name,
        result.CPUTime,
        result.GPUTime);
  }

	ImGui::End();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
	ImGui::Begin("Viewport", nullptr, NULL);
	auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
	auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
	auto viewportOffset = ImGui::GetWindowPos();
	s_Data.m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
	s_Data.m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

	s_Data.m_ViewportFocused = ImGui::IsWindowFocused();
	s_Data.m_ViewportHovered = ImGui::IsWindowHovered();

	BlockEvents(!s_Data.m_ViewportHovered);
	ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
	s_Data.m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

	uint64_t textureID = static_cast<uint64_t>(framebufferTexture);
	ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ s_Data.m_ViewportSize.x, s_Data.m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

	ImGui::End();
	ImGui::PopStyleVar();

	ImGui::End();
	io.DisplaySize = ImVec2((float)Window::GetWidth(), (float)Window::GetHeight());

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}
}

bool Renderer::DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale)
{
  // From glm::decompose in matrix_decompose.inl

  using namespace glm;
  using T = float;

  mat4 LocalMatrix(transform);

  // Normalize the matrix.
  if (epsilonEqual(LocalMatrix[3][3], static_cast<float>(0), epsilon<T>()))
    return false;

  // First, isolate perspective.  This is the messiest.
  if (
    epsilonNotEqual(LocalMatrix[0][3], static_cast<T>(0), epsilon<T>()) ||
    epsilonNotEqual(LocalMatrix[1][3], static_cast<T>(0), epsilon<T>()) ||
    epsilonNotEqual(LocalMatrix[2][3], static_cast<T>(0), epsilon<T>()))
  {
    // Clear the perspective partition
    LocalMatrix[0][3] = LocalMatrix[1][3] = LocalMatrix[2][3] = static_cast<T>(0);
    LocalMatrix[3][3] = static_cast<T>(1);
  }

  // Next take care of translation (easy).
  translation = vec3(LocalMatrix[3]);
  LocalMatrix[3] = vec4(0, 0, 0, LocalMatrix[3].w);

  vec3 Row[3], Pdum3;

  // Now get scale and shear.
  for (length_t i = 0; i < 3; ++i)
    for (length_t j = 0; j < 3; ++j)
      Row[i][j] = LocalMatrix[i][j];

  // Compute X scale factor and normalize first row.
  scale.x = length(Row[0]);
  Row[0] = detail::scale(Row[0], static_cast<T>(1));
  scale.y = length(Row[1]);
  Row[1] = detail::scale(Row[1], static_cast<T>(1));
  scale.z = length(Row[2]);
  Row[2] = detail::scale(Row[2], static_cast<T>(1));

  // At this point, the matrix (in rows[]) is orthonormal.
  // Check for a coordinate system flip.  If the determinant
  // is -1, then negate the matrix and the scaling factors.
#if 0
  Pdum3 = cross(Row[1], Row[2]); // v3Cross(row[1], row[2], Pdum3);
  if (dot(Row[0], Pdum3) < 0)
  {
    for (length_t i = 0; i < 3; i++)
    {
      scale[i] *= static_cast<T>(-1);
      Row[i] *= static_cast<T>(-1);
    }
  }
#endif

  rotation.y = asin(-Row[0][2]);
  if (cos(rotation.y) != 0) {
    rotation.x = atan2(Row[1][2], Row[2][2]);
    rotation.z = atan2(Row[0][1], Row[0][0]);
  }
  else {
    rotation.x = atan2(-Row[2][0], Row[1][1]);
    rotation.z = 0;
  }


  return true;
}

