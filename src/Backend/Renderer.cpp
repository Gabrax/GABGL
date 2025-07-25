#include "Renderer.h"

#include "BackendLogger.h"
#include "Buffer.h"
#include "Camera.h"
#include "LightManager.h"
#include "ModelManager.h"
#include "Renderer.h"
#include "Shader.h"
#include "Texture.h"
#include "../engine.h"
#include "AudioManager.h"
#include <cstdint>
#include <limits>
#include <stb_image.h>
#include <imgui.h>
#include <imgui_internal.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include "ImGuizmo.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/fwd.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include "json.hpp"
#include "../input/UserInput.h"

void MessageCallback(unsigned source,unsigned type,unsigned id,unsigned severity,int length,const char* message,const void* userParam)
{
	switch (severity)
	{
		case GL_DEBUG_SEVERITY_HIGH:         GABGL_CRITICAL(message); return;
		case GL_DEBUG_SEVERITY_MEDIUM:       GABGL_ERROR(message); return;
		case GL_DEBUG_SEVERITY_LOW:          GABGL_WARN(message); return;
		case GL_DEBUG_SEVERITY_NOTIFICATION: GABGL_TRACE(message); return;
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
	int selectedSceneIndex = -1;
	bool m_ViewportFocused = false, m_ViewportHovered = false;
	glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
	glm::vec2 m_ViewportBounds[2];
	bool m_BlockEvents = true;

	static constexpr uint32_t MaxQuads = 20000;
	static constexpr uint32_t MaxVertices = MaxQuads * 4;
	static constexpr uint32_t MaxIndices = MaxQuads * 6;
	static constexpr uint32_t MaxTextureSlots = 32; // TODO: RenderCaps
  static constexpr uint32_t MaxModelVertexCount = 50000;
  static constexpr uint32_t MaxModelIndices  = 75000;

	std::shared_ptr<Texture> WhiteTexture;

	std::shared_ptr<VertexArray> QuadVertexArray;
	std::shared_ptr<VertexBuffer> QuadVertexBuffer;
	uint32_t QuadIndexCount = 0;
	QuadVertex* QuadVertexBufferBase = nullptr;
	QuadVertex* QuadVertexBufferPtr = nullptr;
	glm::vec4 QuadVertexPositions[4];

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

  static constexpr glm::vec2 texCoords[4] =
  {
    { 0.0f, 1.0f },
    { 1.0f, 1.0f },
    { 1.0f, 0.0f },
    { 0.0f, 0.0f }
  };

	std::array<std::shared_ptr<Texture>, MaxTextureSlots> TextureSlots;
	uint32_t TextureSlotIndex = 1; // 0 = white texture

  struct Shaders
	{
		std::shared_ptr<Shader> QuadShader;
		std::shared_ptr<Shader> CircleShader;
		std::shared_ptr<Shader> LineShader;
		std::shared_ptr<Shader> FramebufferShader;
    std::shared_ptr<Shader> skyboxShader;
    std::shared_ptr<Shader> DepthPrePassShader;
    std::shared_ptr<Shader> GeometryShader;
    std::shared_ptr<Shader> LightShader;
    std::shared_ptr<Shader> DownSampleShader;
    std::shared_ptr<Shader> UpSampleShader;
    std::shared_ptr<Shader> BloomResultShader;
    std::shared_ptr<Shader> OmniDirectShadowShader;
    std::shared_ptr<Shader> DirectShadowShader;

	} s_Shaders;

  CameraData m_CameraBuffer;
  std::shared_ptr<UniformBuffer> m_CameraUniformBuffer;
  std::shared_ptr<UniformBuffer> m_ResolutionUniformBuffer;

  std::unordered_map<std::string, std::shared_ptr<Texture>> skyboxes;

  std::shared_ptr<FrameBuffer> m_ResultBuffer;
  std::shared_ptr<FrameBuffer> m_LightBuffer;
  std::shared_ptr<FrameBuffer> m_SkyboxBuffer;
  std::shared_ptr<BloomBuffer> m_BloomBuffer;
  std::shared_ptr<OmniDirectShadowBuffer> m_OmniDirectShadowBuffer;
  std::shared_ptr<DirectShadowBuffer> m_DirectShadowBuffer;
  std::shared_ptr<GeometryBuffer> m_GeometryBuffer;

  std::vector<DrawElementsIndirectCommand> m_DrawCommands;
  std::unordered_map<std::string, std::vector<size_t>> m_ModelDrawCommandIndices;
  uint32_t m_DrawIndexOffset = 0;
  uint32_t m_DrawVertexOffset = 0;
  uint32_t m_cmdBufer;

  Window* m_WindowRef = nullptr;
  Camera m_Camera;

  enum class SceneState
	{
		Edit = 0, Play = 1

	} m_SceneState;

  bool Is3D = false;

} s_Data;

void Renderer::LoadShaders()
{
	s_Data.s_Shaders.QuadShader = Shader::Create("res/shaders/batch_quad.glsl");
	s_Data.s_Shaders.CircleShader = Shader::Create("res/shaders/batch_circle.glsl");
	s_Data.s_Shaders.LineShader = Shader::Create("res/shaders/batch_line.glsl");
	s_Data.s_Shaders.FramebufferShader = Shader::Create("res/shaders/finalFB.glsl");
  s_Data.s_Shaders.skyboxShader = Shader::Create("res/shaders/skybox.glsl");
  s_Data.s_Shaders.DepthPrePassShader = Shader::Create("res/shaders/geometry_z_prepass.glsl");
  s_Data.s_Shaders.GeometryShader = Shader::Create("res/shaders/geometry.glsl");
  s_Data.s_Shaders.LightShader = Shader::Create("res/shaders/light.glsl");
  s_Data.s_Shaders.DownSampleShader = Shader::Create("res/shaders/bloom_downsample.glsl");
  s_Data.s_Shaders.UpSampleShader = Shader::Create("res/shaders/bloom_upsample.glsl");
  s_Data.s_Shaders.BloomResultShader = Shader::Create("res/shaders/bloom_final.glsl");
  s_Data.s_Shaders.OmniDirectShadowShader = Shader::Create("res/shaders/omni_shadowFB.glsl");
  s_Data.s_Shaders.DirectShadowShader = Shader::Create("res/shaders/direct_shadowFB.glsl");
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

  s_Data.m_WindowRef = &Engine::GetInstance().GetMainWindow();
  glm::vec2 resolution = { s_Data.m_WindowRef->GetWidth(), s_Data.m_WindowRef->GetHeight() };

  FramebufferSpecification fbSpec;
	fbSpec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::DEPTH24STENCIL8 };
	fbSpec.Width = resolution.x;
	fbSpec.Height = resolution.y;
	s_Data.m_ResultBuffer = FrameBuffer::Create(fbSpec);

  FramebufferSpecification fbSpec2;
	fbSpec2.Attachments = { FramebufferTextureFormat::RGBA16F, FramebufferTextureFormat::RGBA16F };
	fbSpec2.Width = resolution.x;
	fbSpec2.Height = resolution.y;
	s_Data.m_LightBuffer = FrameBuffer::Create(fbSpec2);

  FramebufferSpecification fbSpec3;
	fbSpec3.Attachments = { FramebufferTextureFormat::RGBA16F, FramebufferTextureFormat::DEPTH };
	fbSpec3.Width = resolution.x;
	fbSpec3.Height = resolution.y;
	s_Data.m_SkyboxBuffer = FrameBuffer::Create(fbSpec3);

  s_Data.m_GeometryBuffer = GeometryBuffer::Create(resolution.x, resolution.y);
  s_Data.m_BloomBuffer = BloomBuffer::Create(s_Data.s_Shaders.DownSampleShader, s_Data.s_Shaders.UpSampleShader, s_Data.s_Shaders.BloomResultShader);
  s_Data.m_OmniDirectShadowBuffer = OmniDirectShadowBuffer::Create(512, 512);
  s_Data.m_DirectShadowBuffer = DirectShadowBuffer::Create(2048, 2048, 16, 8, 3);

  s_Data.m_CameraUniformBuffer = UniformBuffer::Create(sizeof(CameraData), 0);
  s_Data.m_Camera = Camera(45.0f, (float)resolution.x / (float)resolution.y, 0.01f, 2000.0f);
  s_Data.m_Camera.SetViewportSize((float)resolution.x, (float)resolution.y);

  s_Data.m_ResolutionUniformBuffer = UniformBuffer::Create(sizeof(glm::vec2), 1);
  s_Data.m_ResolutionUniformBuffer->SetData(&resolution, sizeof(glm::vec2));

  s_Data.m_SceneState = RendererData::SceneState::Play;
  s_Data.m_Camera.SetMode(CameraMode::FPS);
  s_Data.m_WindowRef->SetCursorVisible(false);

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

	GLFWwindow* window = reinterpret_cast<GLFWwindow*>(s_Data.m_WindowRef->GetWindowPtr());

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 410");
	SetLineWidth(4.0f);
  s_Data.m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
}

void Renderer::Shutdown()
{
	delete[] s_Data.QuadVertexBufferBase;

  ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void Renderer::DrawScene(DeltaTime& dt, const std::function<void()>& geometry, const std::function<void()>& lights)
{
  glDisable(GL_DITHER);
  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_FRONT);
  glFrontFace(GL_CW);

  if(!LightManager::DirectLightEmpty())
  {
    GABGL_PROFILE_SCOPE("DIRECT SHADOW PASS");

    s_Data.m_DirectShadowBuffer->Bind();
    float max = std::numeric_limits<float>::max();
  	glClearColor(max, max, max, max);
    glClear(GL_DEPTH_BUFFER_BIT);

    s_Data.s_Shaders.DirectShadowShader->Bind();
    s_Data.m_DirectShadowBuffer->UpdateShadowView(LightManager::GetDirectLightRotation());
    s_Data.s_Shaders.DirectShadowShader->SetMat4("u_LightSpaceMatrix", s_Data.m_DirectShadowBuffer->GetShadowViewProj());
    s_Data.s_Shaders.DirectShadowShader->SetBool("isInstanced", false);

    glBindVertexArray(ModelManager::GetModelsVAO());
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER,s_Data.m_cmdBufer);
    glMultiDrawElementsIndirect(GL_TRIANGLES,GL_UNSIGNED_INT,NULL,s_Data.m_DrawCommands.size(),0);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER,0);
    glBindVertexArray(0);

    s_Data.s_Shaders.DirectShadowShader->UnBind();
    s_Data.m_DirectShadowBuffer->UnBind();
  }
  if(!LightManager::PointLightEmpty())
  {
    GABGL_PROFILE_SCOPE("OMNI SHADOW PASS");

    s_Data.m_OmniDirectShadowBuffer->Bind();
    float max = std::numeric_limits<float>::max();
  	glClearColor(max, max, max, max);

    s_Data.s_Shaders.OmniDirectShadowShader->Bind();
    uint32_t lightIndex = 0;
    for (const auto& light : LightManager::GetPointLightPositions())
    {
      s_Data.s_Shaders.OmniDirectShadowShader->SetVec3("gLightWorldPos",light);

      const auto& directions = s_Data.m_OmniDirectShadowBuffer->GetFaceDirections();
      for (auto face = 0; face < directions.size(); ++face)
      {
        s_Data.m_OmniDirectShadowBuffer->BindCubemapFaceForWriting(lightIndex, face);
        glClear(GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(light,light + directions[face].Target,directions[face].Up);
        s_Data.s_Shaders.OmniDirectShadowShader->SetMat4("u_LightViewProjection", s_Data.m_OmniDirectShadowBuffer->GetShadowProj() * view);

        glBindVertexArray(ModelManager::GetModelsVAO());
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER,s_Data.m_cmdBufer);
        glMultiDrawElementsIndirect(GL_TRIANGLES,GL_UNSIGNED_INT,NULL,s_Data.m_DrawCommands.size(),0);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER,0);
        glBindVertexArray(0);
      }
      lightIndex++;
    }
    s_Data.s_Shaders.OmniDirectShadowShader->UnBind();
    s_Data.m_OmniDirectShadowBuffer->UnBind();
  }
  {
    GABGL_PROFILE_SCOPE("DEPTH PRE PASS");

    s_Data.m_GeometryBuffer->Bind(); 

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); 
    glDepthMask(GL_TRUE);                                
    glDepthFunc(GL_LESS);                                
    glClear(GL_DEPTH_BUFFER_BIT);                        

    s_Data.s_Shaders.DepthPrePassShader->Bind();
    s_Data.s_Shaders.DepthPrePassShader->SetBool("isInstanced", false);

    BeginScene(s_Data.m_Camera);
    glBindVertexArray(ModelManager::GetModelsVAO());
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, s_Data.m_cmdBufer);
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, s_Data.m_DrawCommands.size(), 0);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    glBindVertexArray(0);
    EndScene();

    s_Data.s_Shaders.DepthPrePassShader->UnBind();
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); 
  }
  {
    GABGL_PROFILE_SCOPE("GEOMETRY PASS");

    s_Data.m_GeometryBuffer->Bind();

    glDepthFunc(GL_EQUAL);       
    glDepthMask(GL_FALSE);       
  	glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT); 

    s_Data.s_Shaders.GeometryShader->Bind();
    s_Data.s_Shaders.GeometryShader->SetBool("isInstanced", false);

    BeginScene(s_Data.m_Camera);
    glBindVertexArray(ModelManager::GetModelsVAO());
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, s_Data.m_cmdBufer);
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, NULL, s_Data.m_DrawCommands.size(), 0);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    glBindVertexArray(0);
    EndScene();
    s_Data.s_Shaders.GeometryShader->UnBind();

    glDepthFunc(GL_LESS);   
    glDepthMask(GL_TRUE);   

    s_Data.m_GeometryBuffer->UnBind();
    s_Data.m_GeometryBuffer->BlitDepthTo(s_Data.m_SkyboxBuffer);
  }
  {
    GABGL_PROFILE_SCOPE("LIGHT PASS");

    s_Data.m_LightBuffer->Bind(); 
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
    s_Data.s_Shaders.LightShader->SetMat4("u_DirectShadowViewProj", s_Data.m_DirectShadowBuffer->GetShadowViewProj());

    DrawFullscreenQuad();

    s_Data.s_Shaders.LightShader->UnBind();
    s_Data.m_LightBuffer->UnBind(); 
  }
  {
    GABGL_PROFILE_SCOPE("BLOOM PASS");

    s_Data.m_BloomBuffer->BlitColorFrom(s_Data.m_LightBuffer,0);
    s_Data.m_BloomBuffer->BlitColorFrom(s_Data.m_LightBuffer,1);
    s_Data.m_BloomBuffer->Bind();
    s_Data.m_BloomBuffer->RenderBloomTexture(0.005f);
    s_Data.m_BloomBuffer->CompositeBloomOver();
    s_Data.m_BloomBuffer->UnBind();
    s_Data.m_BloomBuffer->BlitColorTo(s_Data.m_SkyboxBuffer);
  }
  {
    GABGL_PROFILE_SCOPE("SKYBOX PASS");

    s_Data.m_SkyboxBuffer->Bind();

    Renderer::DrawSkybox("night");

    s_Data.m_SkyboxBuffer->UnBind();
    s_Data.m_SkyboxBuffer->BlitColor(s_Data.m_ResultBuffer);
  }
  {
    GABGL_PROFILE_SCOPE("MISC UPDATE PASS");

    s_Data.m_Camera.OnUpdate(dt);
    ModelManager::UpdateTransforms(dt);
    PhysX::Simulate(dt);
    AudioManager::UpdateAllMusic();
  }
  {
    GABGL_PROFILE_SCOPE("SCENE RESULT PASS");

    s_Data.m_ResultBuffer->Bind();
    s_Data.m_ResultBuffer->ClearAttachment(1, -1);
    s_Data.m_ResultBuffer->UnBind();

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
    glViewport(0, 0, s_Data.m_WindowRef->GetWidth(), s_Data.m_WindowRef->GetHeight());
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    BeginScene(s_Data.m_Camera);
    Renderer::DrawText(FontManager::GetFont("dpcomic"), "FPS: " + std::to_string(dt.GetFPS()), glm::vec2(100.0f, 50.0f), 0.5f, glm::vec4(1.0f));
    EndScene();
  }

  geometry();
}

void Renderer::DrawLoadingScreen()
{
  BeginScene(s_Data.m_Camera);
  Renderer::DrawText(FontManager::GetFont("dpcomic"),"LOADING", glm::vec2(s_Data.m_WindowRef->GetWidth() / 2,s_Data.m_WindowRef->GetHeight() / 2), 1.0f, glm::vec4(1.0f));
  EndScene();
}

void Renderer::SwitchRenderState()
{
  if (s_Data.m_SceneState == RendererData::SceneState::Edit)
  {
    s_Data.m_Camera.SetMode(CameraMode::FPS);
    s_Data.m_WindowRef->SetCursorVisible(false);
    s_Data.m_SceneState = RendererData::SceneState::Play;
  }
  else if (s_Data.m_SceneState == RendererData::SceneState::Play)
  {
    s_Data.m_Camera.SetMode(CameraMode::ORBITAL);
    s_Data.m_WindowRef->SetCursorVisible(true);
    s_Data.m_SceneState = RendererData::SceneState::Edit;
  }
}

void Renderer::SetFullscreen(const std::string& sound, bool windowed)
{
  s_Data.m_WindowRef->SetFullscreen(windowed);

  uint32_t width = (uint32_t)s_Data.m_WindowRef->GetWidth(); 
  uint32_t height = (uint32_t)s_Data.m_WindowRef->GetHeight();

  glm::vec2 newResolution = { width, height };
  s_Data.m_ResolutionUniformBuffer->SetData(&newResolution, sizeof(glm::vec2));

  s_Data.m_GeometryBuffer->Resize(width, height);
  s_Data.m_LightBuffer->Resize(width, height);
  /*s_Data.m_BloomFramebuffer->Resize(width,height);*/
  s_Data.m_SkyboxBuffer->Resize(width,height);
  s_Data.m_ResultBuffer->Resize(width, height);
  s_Data.m_Camera.SetViewportSize(width, height);
  AudioManager::PlaySound(sound);
}

void Renderer::BeginScene(const Camera& camera)
{
	s_Data.m_CameraBuffer.ViewProjection = camera.GetViewProjection();
	s_Data.m_CameraBuffer.OrtoProjection = camera.GetOrtoProjection();
  s_Data.m_CameraBuffer.NonRotViewProjection = camera.GetNonRotationViewProjection();
  s_Data.m_CameraBuffer.CameraPos = camera.GetPosition();
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

void Renderer::DrawQuad(glm::vec3& position, const glm::vec3& size, const glm::vec3& rotation, const glm::vec4& color)
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
	constexpr size_t quadVertexCount = 4;
	const float textureIndex = 0.0f; // White Texture
	constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
	const float tilingFactor = 1.0f;

	if (s_Data.QuadIndexCount >= RendererData::MaxIndices) NextBatch();

	for (size_t i = 0; i < quadVertexCount; i++)
	{
		s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
		s_Data.QuadVertexBufferPtr->Color = color;
		s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
		s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
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
	constexpr size_t quadVertexCount = 4;
	constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

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

	for (size_t i = 0; i < quadVertexCount; i++)
	{
		s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
		s_Data.QuadVertexBufferPtr->Color = tintColor;
		s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
		s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
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
		glm::scale(glm::mat4(1.0f), glm::vec3(xy, 1.0f)),
		texture,tintColor);

	// BACK (-Z)
	DrawQuad(
		glm::translate(glm::mat4(1.0f), position + glm::vec3(0.0f, 0.0f, -halfZ)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), { 0, 1, 0 }) *
		glm::scale(glm::mat4(1.0f), glm::vec3(xy, 1.0f)),
		texture,tintColor);

	// LEFT (-X)
	DrawQuad(
		glm::translate(glm::mat4(1.0f), position + glm::vec3(-halfX, 0.0f, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), { 0, 1, 0 }) *
		glm::scale(glm::mat4(1.0f), glm::vec3(yz, 1.0f)),
		texture,tintColor);

	// RIGHT (+X)
	DrawQuad(
		glm::translate(glm::mat4(1.0f), position + glm::vec3(+halfX, 0.0f, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), { 0, 1, 0 }) *
		glm::scale(glm::mat4(1.0f), glm::vec3(yz, 1.0f)),
		texture,tintColor);

	// TOP (+Y)
	DrawQuad(
		glm::translate(glm::mat4(1.0f), position + glm::vec3(0.0f, +halfY, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), { 1, 0, 0 }) *
		glm::scale(glm::mat4(1.0f), glm::vec3(xz, 1.0f)),
		texture,tintColor);

	// BOTTOM (-Y)
	DrawQuad(
		glm::translate(glm::mat4(1.0f), position + glm::vec3(0.0f, -halfY, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), { 1, 0, 0 }) *
		glm::scale(glm::mat4(1.0f), glm::vec3(xz, 1.0f)),
		texture,tintColor);

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
		glm::scale(glm::mat4(1.0f), glm::vec3(xy, 1.0f)),
		color);

	// BACK (-Z)
	DrawQuad(
		glm::translate(glm::mat4(1.0f), position + glm::vec3(0.0f, 0.0f, -halfZ)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), { 0, 1, 0 }) *
		glm::scale(glm::mat4(1.0f), glm::vec3(xy, 1.0f)),
		color);

	// LEFT (-X)
	DrawQuad(
		glm::translate(glm::mat4(1.0f), position + glm::vec3(-halfX, 0.0f, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), { 0, 1, 0 }) *
		glm::scale(glm::mat4(1.0f), glm::vec3(yz, 1.0f)),
		color);

	// RIGHT (+X)
	DrawQuad(
		glm::translate(glm::mat4(1.0f), position + glm::vec3(+halfX, 0.0f, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), { 0, 1, 0 }) *
		glm::scale(glm::mat4(1.0f), glm::vec3(yz, 1.0f)),
		color);

	// TOP (+Y)
	DrawQuad(
		glm::translate(glm::mat4(1.0f), position + glm::vec3(0.0f, +halfY, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), { 1, 0, 0 }) *
		glm::scale(glm::mat4(1.0f), glm::vec3(xz, 1.0f)),
		color);

	// BOTTOM (-Y)
	DrawQuad(
		glm::translate(glm::mat4(1.0f), position + glm::vec3(0.0f, -halfY, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), { 1, 0, 0 }) *
		glm::scale(glm::mat4(1.0f), glm::vec3(xz, 1.0f)),
		color);
  // Restore the previous front face winding
  glFrontFace(prevFrontFace);
  glDisable(GL_CULL_FACE);

  Renderer::Set3D(false);
}

void Renderer::DrawCubeContour(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color, int entityID)
{
  // Half size for easier calculations
  glm::vec3 half = size * 0.5f;

  // Define 8 cube vertices relative to center position
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
        // positions        // texture Coords
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
        // positions
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
    GABGL_ERROR("Skybox texture not found: " + name);
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
        s_Data.QuadVertexBufferPtr->TexCoord = s_Data.texCoords[i];
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
  auto& meshes = model->GetMeshes();

  const auto& indices = s_Data.m_ModelDrawCommandIndices[model->m_Name];

  size_t vertexOffset = 0; 
  size_t indexOffset = 0;

  for (size_t i = 0; i < meshes.size(); ++i)
  {
    const auto& mesh = meshes[i];
    size_t cmdIndex = indices[i];

    DrawElementsIndirectCommand& cmd = s_Data.m_DrawCommands[cmdIndex];

    cmd.count = static_cast<GLuint>(mesh.m_Indices.size());
    cmd.instanceCount = render ? 1 : 0;
    cmd.firstIndex = static_cast<GLuint>(indexOffset);
    cmd.baseVertex = static_cast<GLint>(vertexOffset);
    cmd.baseInstance = 0;

    vertexOffset += mesh.m_Vertices.size();
    indexOffset += mesh.m_Indices.size();
  }

  glNamedBufferSubData(s_Data.m_cmdBufer, 0, s_Data.m_DrawCommands.size() * sizeof(DrawElementsIndirectCommand), s_Data.m_DrawCommands.data());
}

void Renderer::InitDrawCommandBuffer()
{
  glCreateBuffers(1, &s_Data.m_cmdBufer);
  glNamedBufferStorage(s_Data.m_cmdBufer,sizeof(s_Data.m_DrawCommands[0]) * s_Data.m_DrawCommands.size(),(const void*)s_Data.m_DrawCommands.data(), GL_DYNAMIC_STORAGE_BIT);
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
	if (s_Data.Is3D != is3D)
	{
		Flush();             
		s_Data.Is3D = is3D;  
	}
}

void Renderer::DrawEditorFrameBuffer(uint32_t framebufferTexture)
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
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


	ImGui::End();

	ImGui::Begin("Components", nullptr, ImGuiWindowFlags_NoCollapse);

	if (ImGui::Button("Reload Shaders")) LoadShaders();

  for (const auto& result : s_ProfileResults)
  {
      ImGui::Text("%s: %.3f ms", result.Name, result.Time);
  }
  s_ProfileResults.clear();

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
	io.DisplaySize = ImVec2((float)s_Data.m_WindowRef->GetWidth(), (float)s_Data.m_WindowRef->GetHeight());

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

