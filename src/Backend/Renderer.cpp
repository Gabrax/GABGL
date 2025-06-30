#include "Renderer.h"

#include <array>
#include "BackendLogger.h"
#include "Buffer.h"
#include "LightManager.h"
#include "ModelManager.h"
#include "Renderer.h"
#include "Shader.h"
#include "Texture.h"
#include "glad/glad.h"
#include "glm/fwd.hpp"
#include <cstdint>
#include <limits>
#include <random>
#include <stb_image.h>
#include <filesystem>
#include "../engine.h"
#include "AudioManager.h"
#include "../input/UserInput.h"
#include <imgui.h>
#include <imgui_internal.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include "ImGuizmo.h"
#include <glm/gtc/type_ptr.hpp>
#include "json.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>


void MessageCallback(
	unsigned source,
	unsigned type,
	unsigned id,
	unsigned severity,
	int length,
	const char* message,
	const void* userParam)
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

struct CircleVertex
{
	glm::vec3 WorldPosition;
	glm::vec3 LocalPosition;
	glm::vec4 Color;
	float Thickness;
	float Fade;

	int EntityID;
};

struct LineVertex
{
	glm::vec3 Position;
	glm::vec4 Color;

	int EntityID;
};

struct MeshVertex
{
  glm::vec3 Position;   
  glm::vec3 Normal;     
  glm::vec4 Color;
};

struct CameraData
{
	glm::mat4 ViewProjection;
	glm::mat4 OrtoProjection;
  glm::mat4 NonRotViewProjection;
  glm::vec3 CameraPos;
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

	std::shared_ptr<VertexArray> _3DQuadVertexArray;
	std::shared_ptr<VertexBuffer> _3DQuadVertexBuffer;
	uint32_t _3DQuadIndexCount = 0;
	QuadVertex* _3DQuadVertexBufferBase = nullptr;
	QuadVertex* _3DQuadVertexBufferPtr = nullptr;
	glm::vec4 _3DQuadVertexPositions[4];

	std::shared_ptr<VertexArray> _2DQuadVertexArray;
	std::shared_ptr<VertexBuffer> _2DQuadVertexBuffer;
	uint32_t _2DQuadIndexCount = 0;
	QuadVertex* _2DQuadVertexBufferBase = nullptr;
	QuadVertex* _2DQuadVertexBufferPtr = nullptr;

	std::shared_ptr<VertexArray> CircleVertexArray;
	std::shared_ptr<VertexBuffer> CircleVertexBuffer;
	uint32_t CircleIndexCount = 0;
	CircleVertex* CircleVertexBufferBase = nullptr;
	CircleVertex* CircleVertexBufferPtr = nullptr;

	std::shared_ptr<VertexArray> LineVertexArray;
	std::shared_ptr<VertexBuffer> LineVertexBuffer;
	uint32_t LineVertexCount = 0;
	LineVertex* LineVertexBufferBase = nullptr;
	LineVertex* LineVertexBufferPtr = nullptr;
	float LineWidth = 2.0f;

  std::shared_ptr<VertexArray> CubeVertexArray;
  std::shared_ptr<VertexBuffer> CubeVertexBuffer;
  std::shared_ptr<IndexBuffer> CubeIndexBuffer;
  MeshVertex* CubeVertexBufferBase = nullptr;
  MeshVertex* CubeVertexBufferPtr = nullptr;
  uint32_t CubeIndexCount = 0;
  uint32_t CubeVertexCount = 0;

  std::shared_ptr<VertexArray> ModelVertexArray;
  std::shared_ptr<VertexBuffer> ModelVertexBuffer;
  std::shared_ptr<IndexBuffer> ModelIndexBuffer;
  Vertex* ModelVertexBufferBase = nullptr;
  Vertex* ModelVertexBufferPtr = nullptr;
  uint32_t ModelIndexCount = 0;
  uint32_t ModelVertexCount = 0;

  struct Shaders
	{
		std::shared_ptr<Shader> _3DQuadShader;
		std::shared_ptr<Shader> _2DQuadShader;
		std::shared_ptr<Shader> _CircleShader;
		std::shared_ptr<Shader> _LineShader;
		std::shared_ptr<Shader> _FramebufferShader;
    std::shared_ptr<Shader> modelShader;
    std::shared_ptr<Shader> skyboxShader;
    std::shared_ptr<Shader> ModelShader;
    std::shared_ptr<Shader> DownSampleShader;
    std::shared_ptr<Shader> UpSampleShader;
    std::shared_ptr<Shader> BloomResultShader;
    std::shared_ptr<Shader> PointShadowBufferShader;

	} _shaders;

  CameraData m_CameraBuffer;
  std::shared_ptr<UniformBuffer> m_CameraUniformBuffer;

  glm::vec3 quadPositions[4] = {
      { 0.0f, 0.0f, 0.0f },
      { 1.0f, 0.0f, 0.0f },
      { 1.0f, 1.0f, 0.0f },
      { 0.0f, 1.0f, 0.0f }
  };

  glm::vec2 texCoords[4] = {
      { 0.0f, 1.0f },
      { 1.0f, 1.0f },
      { 1.0f, 0.0f },
      { 0.0f, 0.0f }
  };

  MeshVertex cubeVertices[8] = {
      {{-0.5f, -0.5f,  0.5f}, {0, 0, 1}, {1.0f, 1.0f, 1.0f, 1.0f}},
      {{ 0.5f, -0.5f,  0.5f}, {0, 0, 1}, {1.0f, 1.0f, 1.0f, 1.0f}},
      {{ 0.5f,  0.5f,  0.5f}, {0, 0, 1}, {1.0f, 1.0f, 1.0f, 1.0f}},
      {{-0.5f,  0.5f,  0.5f}, {0, 0, 1}, {1.0f, 1.0f, 1.0f, 1.0f}},

      {{-0.5f, -0.5f, -0.5f}, {0, 0, -1}, {1.0f, 1.0f, 1.0f, 1.0f}},
      {{ 0.5f, -0.5f, -0.5f}, {0, 0, -1}, {1.0f, 1.0f, 1.0f, 1.0f}},
      {{ 0.5f,  0.5f, -0.5f}, {0, 0, -1}, {1.0f, 1.0f, 1.0f, 1.0f}},
      {{-0.5f,  0.5f, -0.5f}, {0, 0, -1}, {1.0f, 1.0f, 1.0f, 1.0f}},
  };
  
	std::array<std::shared_ptr<Texture>, MaxTextureSlots> _3DTextureSlots;
	uint32_t _3DTextureSlotIndex = 1; // 0 = white texture
	std::array<std::shared_ptr<Texture>, MaxTextureSlots> _2DTextureSlots;
	uint32_t _2DTextureSlotIndex = 1; // 0 = white texture

  std::unordered_map<std::string, std::shared_ptr<Texture>> skyboxes;

  std::shared_ptr<FrameBuffer> m_Framebuffer;
  std::shared_ptr<FrameBuffer> m_MSAAFramebuffer;
  std::shared_ptr<BloomBuffer> m_BloomFramebuffer;

  std::vector<std::shared_ptr<PointShadowBuffer>> m_PointShadowFramebuffers;

  std::shared_ptr<PointShadowBuffer> m_PointShadowFramebuffer;

  Window* m_WindowRef = nullptr;
  Camera m_Camera;

  enum class SceneState
	{
		Edit = 0, Play = 1

	} m_SceneState;

  bool m_ShadowPass;
  glm::mat4 m_LightProj = glm::mat4(1.0f);

} s_Data;


void Renderer::LoadShaders()
{
	s_Data._shaders._3DQuadShader = Shader::Create("res/shaders/Renderer2D_Quad.glsl");
	s_Data._shaders._2DQuadShader = Shader::Create("res/shaders/Renderer2D_2DQuad.glsl");
	s_Data._shaders._CircleShader = Shader::Create("res/shaders/Renderer2D_Circle.glsl");
	s_Data._shaders._LineShader = Shader::Create("res/shaders/Renderer2D_Line.glsl");
	s_Data._shaders._FramebufferShader = Shader::Create("res/shaders/FB.glsl");
  s_Data._shaders.modelShader = Shader::Create("res/shaders/Renderer3D_static.glsl");
  s_Data._shaders.skyboxShader = Shader::Create("res/shaders/skybox.glsl");
  s_Data._shaders.ModelShader = Shader::Create("res/shaders/Renderer3D_Model.glsl");
  s_Data._shaders.DownSampleShader = Shader::Create("res/shaders/bloom_downsample.glsl");
  s_Data._shaders.UpSampleShader = Shader::Create("res/shaders/bloom_upsample.glsl");
  s_Data._shaders.BloomResultShader = Shader::Create("res/shaders/bloom_final.glsl");
  s_Data._shaders.PointShadowBufferShader = Shader::Create("res/shaders/point_shadow.glsl");
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

	s_Data._3DQuadVertexArray = VertexArray::Create();
	s_Data._3DQuadVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(QuadVertex));
	s_Data._3DQuadVertexBuffer->SetLayout({
		{ ShaderDataType::Float3, "a_Position"     },
		{ ShaderDataType::Float4, "a_Color"        },
		{ ShaderDataType::Float2, "a_TexCoord"     },
		{ ShaderDataType::Float,  "a_TexIndex"     },
		{ ShaderDataType::Float,  "a_TilingFactor" },
		{ ShaderDataType::Int,    "a_EntityID"     }
		});
	s_Data._3DQuadVertexArray->AddVertexBuffer(s_Data._3DQuadVertexBuffer);
	s_Data._3DQuadVertexBufferBase = new QuadVertex[s_Data.MaxVertices];

  s_Data._2DQuadVertexArray = VertexArray::Create();
	s_Data._2DQuadVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(QuadVertex));
	s_Data._2DQuadVertexBuffer->SetLayout({
		{ ShaderDataType::Float3, "a_Position"     },
		{ ShaderDataType::Float4, "a_Color"        },
		{ ShaderDataType::Float2, "a_TexCoord"     },
		{ ShaderDataType::Float,  "a_TexIndex"     },
		{ ShaderDataType::Float,  "a_TilingFactor" },
		{ ShaderDataType::Int,    "a_EntityID"     }
		});
	s_Data._2DQuadVertexArray->AddVertexBuffer(s_Data._2DQuadVertexBuffer);
	s_Data._2DQuadVertexBufferBase = new QuadVertex[s_Data.MaxVertices];

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

	std::shared_ptr<IndexBuffer> _3DquadIB = IndexBuffer::Create(quadIndices, s_Data.MaxIndices);
	s_Data._3DQuadVertexArray->SetIndexBuffer(_3DquadIB);
	std::shared_ptr<IndexBuffer> _2DquadIB = IndexBuffer::Create(quadIndices, s_Data.MaxIndices);
	s_Data._2DQuadVertexArray->SetIndexBuffer(_2DquadIB);
	delete[] quadIndices;

	// Circles
	s_Data.CircleVertexArray = VertexArray::Create();
	s_Data.CircleVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(CircleVertex));
	s_Data.CircleVertexBuffer->SetLayout({
		{ ShaderDataType::Float3, "a_WorldPosition" },
		{ ShaderDataType::Float3, "a_LocalPosition" },
		{ ShaderDataType::Float4, "a_Color"         },
		{ ShaderDataType::Float,  "a_Thickness"     },
		{ ShaderDataType::Float,  "a_Fade"          },
		{ ShaderDataType::Int,    "a_EntityID"      }
		/*{ ShaderDataType::Float,  "a_TilingFactor"  },
		{ ShaderDataType::Float2, "a_TexCoord"		},
		{ ShaderDataType::Float,  "a_TexIndex"		}*/
		});
	s_Data.CircleVertexArray->AddVertexBuffer(s_Data.CircleVertexBuffer);
	s_Data.CircleVertexArray->SetIndexBuffer(_3DquadIB); // Use quad IB
	s_Data.CircleVertexBufferBase = new CircleVertex[s_Data.MaxVertices];

	// Lines
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
	for (uint32_t i = 0; i < s_Data.MaxTextureSlots; i++)
		samplers[i] = i;

	s_Data._3DTextureSlots[0] = s_Data.WhiteTexture;
	s_Data._2DTextureSlots[0] = s_Data.WhiteTexture;

	s_Data._3DQuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
	s_Data._3DQuadVertexPositions[1] = { 0.5f, -0.5f, 0.0f, 1.0f };
	s_Data._3DQuadVertexPositions[2] = { 0.5f,  0.5f, 0.0f, 1.0f };
	s_Data._3DQuadVertexPositions[3] = { -0.5f,  0.5f, 0.0f, 1.0f };

  s_Data.CubeVertexBufferBase = new MeshVertex[s_Data.MaxVertices];
  s_Data.CubeVertexBuffer = VertexBuffer::Create(nullptr, s_Data.MaxVertices * sizeof(MeshVertex));
  s_Data.CubeVertexBuffer->SetLayout({
      { ShaderDataType::Float3, "a_Position" },   // 0
      { ShaderDataType::Float3, "a_Normal" },     // 1
      { ShaderDataType::Float4, "a_Color" },      // 3
  });

  std::vector<uint32_t> indices;
  indices.reserve(s_Data.MaxIndices);

  const uint32_t cubeIndicesCount = 36;

  uint32_t cubeIndices[36] = {
      0, 1, 2, 2, 3, 0,       // Front
      4, 5, 6, 6, 7, 4,       // Back
      4, 0, 3, 3, 7, 4,       // Left
      1, 5, 6, 6, 2, 1,       // Right
      3, 2, 6, 6, 7, 3,       // Top
      4, 5, 1, 1, 0, 4        // Bottom
  };

  uint32_t maxCubes = s_Data.MaxVertices / 8; // 8 vertices per cube
  for (uint32_t i = 0; i < maxCubes; i++)
  {
      for (uint32_t j = 0; j < cubeIndicesCount; j++)
      {
          indices.push_back(cubeIndices[j] + i * 8);
      }
  }

  s_Data.CubeIndexBuffer = IndexBuffer::Create(indices.data(), (uint32_t)indices.size());
  s_Data.CubeVertexArray = VertexArray::Create();
  s_Data.CubeVertexArray->AddVertexBuffer(s_Data.CubeVertexBuffer);
  s_Data.CubeVertexArray->SetIndexBuffer(s_Data.CubeIndexBuffer);
  s_Data.CubeVertexBufferPtr = s_Data.CubeVertexBufferBase;

  s_Data.m_CameraUniformBuffer = UniformBuffer::Create(sizeof(CameraData), 0);

  LoadShaders();

  FramebufferSpecification fbSpec2;
	fbSpec2.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::DEPTH24STENCIL8 };
	fbSpec2.Width = Engine::GetInstance().GetMainWindow().GetWidth();
	fbSpec2.Height = Engine::GetInstance().GetMainWindow().GetHeight();
  fbSpec2.Samples = 4;
	s_Data.m_MSAAFramebuffer = FrameBuffer::Create(fbSpec2);

  FramebufferSpecification fbSpec;
	fbSpec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::DEPTH24STENCIL8 };
	fbSpec.Width = Engine::GetInstance().GetMainWindow().GetWidth();
	fbSpec.Height = Engine::GetInstance().GetMainWindow().GetHeight();
	s_Data.m_Framebuffer = FrameBuffer::Create(fbSpec);

  s_Data.m_BloomFramebuffer = BloomBuffer::Create(s_Data._shaders.DownSampleShader, s_Data._shaders.UpSampleShader, s_Data._shaders.BloomResultShader);

  s_Data.m_PointShadowFramebuffer = PointShadowBuffer::Create(4096, 4096);

  for (int32_t i = 0; i < 1; ++i)
  {
    s_Data.m_PointShadowFramebuffers.emplace_back(PointShadowBuffer::Create(4096, 4096));
  }

  s_Data.m_SceneState = RendererData::SceneState::Play;

  s_Data.m_WindowRef = &Engine::GetInstance().GetMainWindow();

  s_Data.m_Camera = Camera(45.0f, (float)s_Data.m_WindowRef->GetWidth() / (float)s_Data.m_WindowRef->GetHeight(), 0.01f, 1000.0f);
  s_Data.m_Camera.SetViewportSize((float)s_Data.m_WindowRef->GetWidth(), (float)s_Data.m_WindowRef->GetHeight());

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	//float fontSize = 18.0f;// *2.0f;
	//io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Bold.ttf", fontSize);
	//io.FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Regular.ttf", fontSize);

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
	delete[] s_Data._3DQuadVertexBufferBase;
	delete[] s_Data._2DQuadVertexBufferBase;

  delete[] s_Data.CubeVertexBufferBase;
  s_Data.CubeVertexBufferBase = nullptr;

  s_Data.CubeVertexArray.reset();
  s_Data.CubeVertexBuffer.reset();
  s_Data.CubeIndexBuffer.reset();

  ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

struct CameraDirection
{
    GLenum CubemapFace;
    glm::vec3 Target;
    glm::vec3 Up;
};

CameraDirection gCameraDirections[6] =
{
    { GL_TEXTURE_CUBE_MAP_POSITIVE_X, glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3( 0.0f, -1.0f,  0.0f) },
    { GL_TEXTURE_CUBE_MAP_NEGATIVE_X, glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3( 0.0f, -1.0f,  0.0f) },
    { GL_TEXTURE_CUBE_MAP_POSITIVE_Y, glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3( 0.0f,  0.0f, -1.0f) },
    { GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3( 0.0f,  0.0f, -1.0f) },
    { GL_TEXTURE_CUBE_MAP_POSITIVE_Z, glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3( 0.0f, -1.0f,  0.0f) },
    { GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3( 0.0f, -1.0f,  0.0f) }
};

void Renderer::DrawScene(DeltaTime& dt, const std::function<void()>& geometry, const std::function<void()>& lights)
{
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_FRONT);
  glFrontFace(GL_CW);

  s_Data.m_ShadowPass = true;
  float max = std::numeric_limits<float>::max();
  SetClearColor({max,max,max,max});

  for (auto lightIndex : std::views::iota(0, LightManager::GetLightsSize()))
  {
  s_Data.m_PointShadowFramebuffer->Bind();
    s_Data._shaders.PointShadowBufferShader->Use();
    glm::vec3 lightPosition = LightManager::GetLightPosition(lightIndex);
    s_Data._shaders.PointShadowBufferShader->setVec3("gLightWorldPos",lightPosition);

    for (uint32_t face = 0; face < 6; ++face)
    {
      s_Data.m_PointShadowFramebuffer->BindForWriting(gCameraDirections[face].CubemapFace);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glm::mat4 view = glm::lookAt(lightPosition,lightPosition + gCameraDirections[face].Target,gCameraDirections[face].Up);
      s_Data.m_LightProj = s_Data.m_PointShadowFramebuffer->GetLightProj() * view;

      geometry();
    }
  s_Data.m_PointShadowFramebuffer->UnBind();
  }
  s_Data.m_ShadowPass = false;

  s_Data.m_MSAAFramebuffer->Bind();
  SetClearColor(glm::vec4(0.0f));
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  s_Data._shaders.ModelShader->Use();
  s_Data.m_PointShadowFramebuffer->BindForReading(GL_TEXTURE1);
  s_Data._shaders.ModelShader->setInt("u_ShadowMap",1);
  BeginScene(s_Data.m_Camera);
  geometry();
  EndScene();

  s_Data.m_MSAAFramebuffer->UnBind();
  s_Data.m_MSAAFramebuffer->BlitColor(s_Data.m_Framebuffer);

  s_Data.m_BloomFramebuffer->BlitDepthFrom(s_Data.m_MSAAFramebuffer);
  s_Data.m_BloomFramebuffer->Bind();
  glClear(GL_COLOR_BUFFER_BIT);

  BeginScene(s_Data.m_Camera);
  lights();
  EndScene();

  s_Data.m_BloomFramebuffer->RenderBloomTexture(0.005f);
  s_Data.m_BloomFramebuffer->UnBind();
  s_Data.m_BloomFramebuffer->CompositeBloomOver(s_Data.m_Framebuffer);
  s_Data.m_BloomFramebuffer->BlitDepthTo(s_Data.m_Framebuffer);

  s_Data.m_Framebuffer->Bind();
  s_Data.m_Framebuffer->ClearAttachment(1, -1);
  s_Data.m_Framebuffer->UnBind();

  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  SetClearColor(glm::vec4(0.0f));
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  s_Data.m_Camera.OnUpdate(dt);
  ModelManager::UpdateAnimations(dt);
  PhysX::Simulate(dt);
  AudioManager::UpdateAllMusic();

  if (Input::IsKeyPressed(Key::E)) s_Data.m_SceneState = RendererData::SceneState::Edit;
  if (Input::IsKeyPressed(Key::Q)) s_Data.m_SceneState = RendererData::SceneState::Play;

  uint32_t finalTexture = s_Data.m_Framebuffer->GetColorAttachmentRendererID();

  switch (s_Data.m_SceneState)
  {
   case RendererData::SceneState::Edit:
   {
     s_Data.m_WindowRef->SetCursorVisible(true);
     Renderer::DrawEditorFrameBuffer(finalTexture);
     break;
   }

   case RendererData::SceneState::Play:
   {
     s_Data.m_WindowRef->SetCursorVisible(false);
     Renderer::DrawFramebuffer(finalTexture);
     break;
   }
  }
}

void Renderer::DrawLoadingScreen()
{
  BeginScene(s_Data.m_Camera);
  Renderer::Draw2DText(FontManager::GetFont("dpcomic"),"LOADING", glm::vec2(s_Data.m_WindowRef->GetWidth() / 2,s_Data.m_WindowRef->GetHeight() / 2), 1.0f, glm::vec4(1.0f));
  EndScene();
}

void Renderer::SetFullscreen(const std::string& sound, bool windowed)
{
  s_Data.m_WindowRef->SetFullscreen(windowed);

  uint32_t width = (uint32_t)s_Data.m_WindowRef->GetWidth(); 
  uint32_t height = (uint32_t)s_Data.m_WindowRef->GetHeight(); 

  s_Data.m_MSAAFramebuffer->Resize(width, height);
  s_Data.m_BloomFramebuffer->Resize(width,height);
  s_Data.m_Framebuffer->Resize(width, height);
  s_Data.m_Camera.SetViewportSize(width, height);
  AudioManager::PlaySound(sound);
}

void Renderer::StartBatch()
{
	s_Data._3DQuadIndexCount = 0;
	s_Data._3DQuadVertexBufferPtr = s_Data._3DQuadVertexBufferBase;
	s_Data._3DTextureSlotIndex = 1;

	s_Data._2DQuadIndexCount = 0;
	s_Data._2DQuadVertexBufferPtr = s_Data._2DQuadVertexBufferBase;
	s_Data._2DTextureSlotIndex = 1;

	s_Data.CircleIndexCount = 0;
	s_Data.CircleVertexBufferPtr = s_Data.CircleVertexBufferBase;

	s_Data.LineVertexCount = 0;
	s_Data.LineVertexBufferPtr = s_Data.LineVertexBufferBase;

  s_Data.CubeVertexCount = 0;
  s_Data.CubeIndexCount = 0;
  s_Data.CubeVertexBufferPtr = s_Data.CubeVertexBufferBase;
}

void Renderer::BeginScene(const Camera& camera, const glm::mat4& transform)
{
	s_Data.m_CameraBuffer.ViewProjection = camera.GetProjection() * glm::inverse(transform);
	s_Data.m_CameraBuffer.OrtoProjection = camera.GetOrtoProjection();
  s_Data.m_CameraBuffer.NonRotViewProjection = camera.GetNonRotationViewProjection();
  s_Data.m_CameraBuffer.CameraPos = camera.GetPosition();
	s_Data.m_CameraUniformBuffer->SetData(&s_Data.m_CameraBuffer, sizeof(CameraData));

	StartBatch();
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

void Renderer::Flush()
{
	if (s_Data._3DQuadIndexCount)
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data._3DQuadVertexBufferPtr - (uint8_t*)s_Data._3DQuadVertexBufferBase);
		s_Data._3DQuadVertexBuffer->SetData(s_Data._3DQuadVertexBufferBase, dataSize);

		for (uint32_t i = 0; i < s_Data._3DTextureSlotIndex; i++)
			s_Data._3DTextureSlots[i]->Bind(i);

		s_Data._shaders._3DQuadShader->Use();
		Renderer::DrawIndexed(s_Data._3DQuadVertexArray, s_Data._3DQuadIndexCount);
	}
	if (s_Data._2DQuadIndexCount)
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data._2DQuadVertexBufferPtr - (uint8_t*)s_Data._2DQuadVertexBufferBase);
		s_Data._2DQuadVertexBuffer->SetData(s_Data._2DQuadVertexBufferBase, dataSize);

		for (uint32_t i = 0; i < s_Data._2DTextureSlotIndex; i++)
			s_Data._2DTextureSlots[i]->Bind(i);

		s_Data._shaders._2DQuadShader->Use();
		Renderer::DrawIndexed(s_Data._2DQuadVertexArray, s_Data._2DQuadIndexCount);
	}
	if (s_Data.CircleIndexCount)
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.CircleVertexBufferPtr - (uint8_t*)s_Data.CircleVertexBufferBase);
		s_Data.CircleVertexBuffer->SetData(s_Data.CircleVertexBufferBase, dataSize);

		s_Data._shaders._CircleShader->Use();
		Renderer::DrawIndexed(s_Data.CircleVertexArray, s_Data.CircleIndexCount);
	}
	if (s_Data.LineVertexCount)
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.LineVertexBufferPtr - (uint8_t*)s_Data.LineVertexBufferBase);
		s_Data.LineVertexBuffer->SetData(s_Data.LineVertexBufferBase, dataSize);

		s_Data._shaders._LineShader->Use();
		Renderer::SetLineWidth(s_Data.LineWidth);
		Renderer::DrawLines(s_Data.LineVertexArray, s_Data.LineVertexCount);
	}
  if (s_Data.CubeIndexCount) 
  {
    uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.CubeVertexBufferPtr - (uint8_t*)s_Data.CubeVertexBufferBase);
    s_Data.CubeVertexBuffer->SetData(s_Data.CubeVertexBufferBase, dataSize);

    s_Data._shaders.modelShader->Use();
    s_Data.CubeVertexArray->Bind();

    Renderer::DrawIndexed(s_Data.CubeVertexArray, s_Data.CubeIndexCount);
  }
}

void Renderer::EndScene()
{
	Flush();
}

void Renderer::NextBatch()
{
	Flush();
	StartBatch();
}

void Renderer::Draw3DQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
{
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
		* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

	Draw3DQuad(transform, color);
}

void Renderer::Draw3DQuad(const glm::mat4& transform, const glm::vec4& color, int entityID)
{
	constexpr size_t quadVertexCount = 4;
	const float textureIndex = 0.0f; // White Texture
	constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
	const float tilingFactor = 1.0f;

	if (s_Data._3DQuadIndexCount >= RendererData::MaxIndices)
		NextBatch();

	for (size_t i = 0; i < quadVertexCount; i++)
	{
		s_Data._3DQuadVertexBufferPtr->Position = transform * s_Data._3DQuadVertexPositions[i];
		s_Data._3DQuadVertexBufferPtr->Color = color;
		s_Data._3DQuadVertexBufferPtr->TexCoord = textureCoords[i];
		s_Data._3DQuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data._3DQuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data._3DQuadVertexBufferPtr->EntityID = entityID;
		s_Data._3DQuadVertexBufferPtr++;
	}

	s_Data._3DQuadIndexCount += 6;
}

void Renderer::Draw3DQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const std::shared_ptr<Texture>& texture, float tilingFactor, const glm::vec4& tintColor)
{
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
		* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
		* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

	Draw3DQuad(transform, texture, tilingFactor, tintColor);
}

void Renderer::Draw3DQuad(const glm::mat4& transform, const std::shared_ptr<Texture>& texture, float tilingFactor, const glm::vec4& tintColor, int entityID)
{
	constexpr size_t quadVertexCount = 4;
	constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

	if (s_Data._3DQuadIndexCount >= RendererData::MaxIndices)
		NextBatch();

	float textureIndex = 0.0f;
	for (uint32_t i = 1; i < s_Data._3DTextureSlotIndex; i++)
	{
		if (*s_Data._3DTextureSlots[i] == *texture)
		{
			textureIndex = (float)i;
			break;
		}
	}

	if (textureIndex == 0.0f)
	{
		if (s_Data._3DTextureSlotIndex >= RendererData::MaxTextureSlots)
			NextBatch();

		textureIndex = (float)s_Data._3DTextureSlotIndex;
		s_Data._3DTextureSlots[s_Data._3DTextureSlotIndex] = texture;
		s_Data._3DTextureSlotIndex++;
	}

	for (size_t i = 0; i < quadVertexCount; i++)
	{
		s_Data._3DQuadVertexBufferPtr->Position = transform * s_Data._3DQuadVertexPositions[i];
		s_Data._3DQuadVertexBufferPtr->Color = tintColor;
		s_Data._3DQuadVertexBufferPtr->TexCoord = textureCoords[i];
		s_Data._3DQuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data._3DQuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data._3DQuadVertexBufferPtr->EntityID = entityID;
		s_Data._3DQuadVertexBufferPtr++;
	}

	s_Data._3DQuadIndexCount += 6;
}

void Renderer::Draw3DCircle(const glm::mat4& transform, const glm::vec4& color, float thickness /*= 1.0f*/, float fade /*= 0.005f*/, int entityID /*= -1*/)
{
	// TODO: implement for circles
	// if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
	// 	NextBatch();

	for (size_t i = 0; i < 4; i++)
	{
		s_Data.CircleVertexBufferPtr->WorldPosition = transform * s_Data._3DQuadVertexPositions[i];
		s_Data.CircleVertexBufferPtr->LocalPosition = s_Data._3DQuadVertexPositions[i] * 2.0f;
		s_Data.CircleVertexBufferPtr->Color = color;
		s_Data.CircleVertexBufferPtr->Thickness = thickness;
		s_Data.CircleVertexBufferPtr->Fade = fade;
		s_Data.CircleVertexBufferPtr->EntityID = entityID;
		s_Data.CircleVertexBufferPtr++;
	}

	s_Data.CircleIndexCount += 6;
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

void Renderer::Draw3DRect(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, int entityID)
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

void Renderer::Draw3DRect(const glm::mat4& transform, const glm::vec4& color, int entityID)
{
	glm::vec3 lineVertices[4];
	for (size_t i = 0; i < 4; i++) lineVertices[i] = transform * s_Data._3DQuadVertexPositions[i];

	DrawLine(lineVertices[0], lineVertices[1], color, entityID);
	DrawLine(lineVertices[1], lineVertices[2], color, entityID);
	DrawLine(lineVertices[2], lineVertices[3], color, entityID);
	DrawLine(lineVertices[3], lineVertices[0], color, entityID);
}

void Renderer::Draw2DQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
{
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f))
		* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
		* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

	Draw2DQuad(transform, color);
}

void Renderer::Draw2DQuad(const glm::mat4& transform, const glm::vec4& color, int entityID)
{
	constexpr size_t quadVertexCount = 4;
	const float textureIndex = 0.0f; // White Texture
	constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
	const float tilingFactor = 1.0f;

	if (s_Data._2DQuadIndexCount >= RendererData::MaxIndices) NextBatch();

	for (size_t i = 0; i < quadVertexCount; i++)
	{
		s_Data._2DQuadVertexBufferPtr->Position = transform * s_Data._3DQuadVertexPositions[i];
		s_Data._2DQuadVertexBufferPtr->Color = color;
		s_Data._2DQuadVertexBufferPtr->TexCoord = textureCoords[i];
		s_Data._2DQuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data._2DQuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data._2DQuadVertexBufferPtr->EntityID = entityID;
		s_Data._2DQuadVertexBufferPtr++;
	}

	s_Data._2DQuadIndexCount += 6;
}

void Renderer::Draw2DQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const std::shared_ptr<Texture>& texture, float tilingFactor, const glm::vec4& tintColor)
{
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f))
		* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
		* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

	Draw2DQuad(transform, texture, tilingFactor, tintColor);
}

void Renderer::Draw2DQuad(const glm::mat4& transform, const std::shared_ptr<Texture>& texture, float tilingFactor, const glm::vec4& tintColor, int entityID)
{
	constexpr size_t quadVertexCount = 4;
	constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

	if (s_Data._2DQuadIndexCount >= RendererData::MaxIndices) NextBatch();

	float textureIndex = 0.0f;
	for (uint32_t i = 1; i < s_Data._2DTextureSlotIndex; i++)
	{
		if (*s_Data._2DTextureSlots[i] == *texture)
		{
			textureIndex = (float)i;
			break;
		}
	}

	if (textureIndex == 0.0f)
	{
		if (s_Data._2DTextureSlotIndex >= RendererData::MaxTextureSlots) NextBatch();

		textureIndex = (float)s_Data._2DTextureSlotIndex;
		s_Data._2DTextureSlots[s_Data._2DTextureSlotIndex] = texture;
		s_Data._2DTextureSlotIndex++;
	}

	for (size_t i = 0; i < quadVertexCount; i++)
	{
		s_Data._2DQuadVertexBufferPtr->Position = transform * s_Data._3DQuadVertexPositions[i];
		s_Data._2DQuadVertexBufferPtr->Color = tintColor;
		s_Data._2DQuadVertexBufferPtr->TexCoord = textureCoords[i];
		s_Data._2DQuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data._2DQuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data._2DQuadVertexBufferPtr->EntityID = entityID;
		s_Data._2DQuadVertexBufferPtr++;
	}

	s_Data._2DQuadIndexCount += 6;
}

void Renderer::Draw2DCircle(const glm::mat4& transform, const glm::vec4& color, float thickness /*= 1.0f*/, float fade /*= 0.005f*/, int entityID /*= -1*/)
{
	// TODO: implement for circles
	// if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
	// 	NextBatch();

	for (size_t i = 0; i < 4; i++)
	{
		s_Data.CircleVertexBufferPtr->WorldPosition = transform * s_Data._3DQuadVertexPositions[i];
		s_Data.CircleVertexBufferPtr->LocalPosition = s_Data._3DQuadVertexPositions[i] * 2.0f;
		s_Data.CircleVertexBufferPtr->Color = color;
		s_Data.CircleVertexBufferPtr->Thickness = thickness;
		s_Data.CircleVertexBufferPtr->Fade = fade;
		s_Data.CircleVertexBufferPtr->EntityID = entityID;
		s_Data.CircleVertexBufferPtr++;
	}

	s_Data.CircleIndexCount += 6;
}

void Renderer::Draw2DRect(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color, int entityID)
{
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(position,0.0f))
		* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
		* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

	Draw2DRect(transform, color);
}

void Renderer::Draw2DRect(const glm::mat4& transform, const glm::vec4& color, int entityID)
{
	glm::vec3 lineVertices[4];
	for (size_t i = 0; i < 4; i++)
		lineVertices[i] = transform * s_Data._3DQuadVertexPositions[i];

	DrawLine(lineVertices[0], lineVertices[1], color, entityID);
	DrawLine(lineVertices[1], lineVertices[2], color, entityID);
	DrawLine(lineVertices[2], lineVertices[3], color, entityID);
	DrawLine(lineVertices[3], lineVertices[0], color, entityID);
}

void Renderer::DrawCube(const Transform& transform, int entityID)
{
    // Backup the current front face winding
    GLint prevFrontFace;
    glGetIntegerv(GL_FRONT_FACE, &prevFrontFace);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glFrontFace(GL_CCW); // Only cubes use counter-clockwise winding

    if (s_Data.CubeIndexCount >= RendererData::MaxIndices)
        NextBatch(); // Flush batch if full

    glm::mat4 model = transform.GetTransform();
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

    for (int i = 0; i < 8; i++)
    {
        MeshVertex vertex = s_Data.cubeVertices[i];

        glm::vec4 transformedPos = model * glm::vec4(vertex.Position, 1.0f);
        vertex.Position = glm::vec3(transformedPos);
        vertex.Normal = glm::normalize(normalMatrix * vertex.Normal);

        *s_Data.CubeVertexBufferPtr = vertex;
        s_Data.CubeVertexBufferPtr++;
    }

    s_Data.CubeVertexCount += 8;
    s_Data.CubeIndexCount += 36;

    // Restore the previous front face winding
    glFrontFace(prevFrontFace);
    glDisable(GL_CULL_FACE);
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

void Renderer::DrawFramebuffer(uint32_t textureID)
{
  s_Data._shaders._FramebufferShader->Use();
  s_Data._shaders._FramebufferShader->setInt("u_Texture", 0);

  static uint32_t quadVAO = 0, quadVBO = 0;
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

      glGenVertexArrays(1, &quadVAO);
      glGenBuffers(1, &quadVBO);
      glBindVertexArray(quadVAO);
      glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
      glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
  }

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textureID);

  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
}


void Renderer::DrawModel(const std::shared_ptr<Model>& model, const glm::vec3& position, const glm::vec3& size, const glm::vec3& rotation)
{
  if(model == nullptr)
  {
    GABGL_ERROR("Model doesnt exist");
    return;
  }

  glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);

  transform = glm::rotate(transform, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
  transform = glm::rotate(transform, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
  transform = glm::rotate(transform, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));

  transform = glm::scale(transform, size);

  DrawModel(model, transform);
}

void Renderer::DrawModel(const std::shared_ptr<Model>& model, const std::shared_ptr<Model>& convex, const glm::vec3& position, const glm::vec3& size, const glm::vec3& rotation)
{
  if(model == nullptr || convex == nullptr)
  {
    GABGL_ERROR("Model doesnt exist");
    return;
  }

  glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);

  transform = glm::rotate(transform, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
  transform = glm::rotate(transform, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
  transform = glm::rotate(transform, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));

  transform = glm::scale(transform, size);

  DrawModel(model, convex, transform);
}

void Renderer::DrawModel(const std::shared_ptr<Model>& model, const glm::mat4& transform, int entityID)
{
  if(model == nullptr)
  {
    GABGL_ERROR("Model doesnt exist");
    return;
  }

  if(s_Data.m_ShadowPass)
  {
    s_Data._shaders.PointShadowBufferShader->Use();

    glm::mat4 modelMat = glm::mat4(1.0f); 
    if (model->GetPhysXMeshType() == MeshType::TRIANGLEMESH)  modelMat = PhysX::PxMat44ToGlmMat4(model->GetStaticActor()->getGlobalPose());
    else if (model->GetPhysXMeshType() == MeshType::CONTROLLER) modelMat = model->GetControllerTransform().GetTransform();
    else if (model->GetPhysXMeshType() == MeshType::NONE) modelMat = transform;

    s_Data._shaders.PointShadowBufferShader->setMat4("gWVP", s_Data.m_LightProj * modelMat);
    s_Data._shaders.PointShadowBufferShader->setMat4("gWorld", modelMat);

    for (auto& mesh : model->GetMeshes())
    {
      glBindVertexArray(mesh.VAO);
      glDrawElements(GL_TRIANGLES, static_cast<GLuint>(mesh.m_Indices.size()), GL_UNSIGNED_INT, 0);
      glBindVertexArray(0);
    }
  }
  else
  {
    s_Data._shaders.ModelShader->Use();
    if (model->GetPhysXMeshType() == MeshType::TRIANGLEMESH) s_Data._shaders.ModelShader->setMat4("model", PhysX::PxMat44ToGlmMat4(model->GetStaticActor()->getGlobalPose()));
    else if (model->GetPhysXMeshType() == MeshType::CONTROLLER) s_Data._shaders.ModelShader->setMat4("model", model->GetControllerTransform().GetTransform());
    else if (model->GetPhysXMeshType() == MeshType::NONE) s_Data._shaders.ModelShader->setMat4("model", transform);
    s_Data._shaders.ModelShader->setBool("isAnimated", model->IsAnimated());
    s_Data._shaders.ModelShader->setBool("isInstanced", false);
    if(model->IsAnimated())
    {
      auto& transforms = model->GetFinalBoneMatrices();
      for (size_t i = 0; i < transforms.size(); ++i) {
          s_Data._shaders.ModelShader->setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);
      }
    }

    for (auto& mesh : model->GetMeshes())
    {
      std::unordered_map<std::string, GLuint> textureCounters =
      {
        {"texture_diffuse", 1},
        {"texture_specular", 1},
        {"texture_normal", 1},
        {"texture_height", 1}
      };

      auto& textures = mesh.m_Textures;
      for (GLuint i = 0; i < textures.size(); i++)
      {
        glActiveTexture(GL_TEXTURE0 + i);

        std::string number = std::to_string(textureCounters[textures[i]->GetType()]++);
        s_Data._shaders.ModelShader->setInt(textures[i]->GetType() + number, i);
        glBindTexture(GL_TEXTURE_2D, textures[i]->GetRendererID());
      }
      
      glBindVertexArray(mesh.VAO);
      glDrawElements(GL_TRIANGLES, static_cast<GLuint>(mesh.m_Indices.size()), GL_UNSIGNED_INT, 0);
      glBindVertexArray(0);
      glActiveTexture(GL_TEXTURE0);
    }
  }
}

void Renderer::DrawModel(const std::shared_ptr<Model>& model, const std::shared_ptr<Model>& convex, const glm::mat4& transform, int entityID)
{
  if(model == nullptr || convex == nullptr)
  {
    GABGL_ERROR("Model doesnt exist");
    return;
  }

  if(s_Data.m_ShadowPass)
  {
    s_Data._shaders.PointShadowBufferShader->Use();
    glm::mat4 modelMat = PhysX::PxMat44ToGlmMat4(convex->GetDynamicActor()->getGlobalPose()); 

    s_Data._shaders.PointShadowBufferShader->setMat4("gWVP", s_Data.m_LightProj * modelMat);
    s_Data._shaders.PointShadowBufferShader->setMat4("gWorld", modelMat);

    for (auto& mesh : model->GetMeshes())
    {
      glBindVertexArray(mesh.VAO);
      glDrawElements(GL_TRIANGLES, static_cast<GLuint>(mesh.m_Indices.size()), GL_UNSIGNED_INT, 0);
      glBindVertexArray(0);
    }
  }
  else
  {
    s_Data._shaders.ModelShader->Use();
    s_Data._shaders.ModelShader->setMat4("model", PhysX::PxMat44ToGlmMat4(convex->GetDynamicActor()->getGlobalPose()));
    s_Data._shaders.ModelShader->setBool("isAnimated", model->IsAnimated());
    s_Data._shaders.ModelShader->setBool("isInstanced", false);
    if(model->IsAnimated())
    {
      auto& transforms = model->GetFinalBoneMatrices();
      for (size_t i = 0; i < transforms.size(); ++i) {
          s_Data._shaders.ModelShader->setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);
      }
    }

    for (auto& mesh : model->GetMeshes())
    {
      std::unordered_map<std::string, GLuint> textureCounters =
      {
        {"texture_diffuse", 1},
        {"texture_specular", 1},
        {"texture_normal", 1},
        {"texture_height", 1}
      };

      auto& textures = mesh.m_Textures;
      for (GLuint i = 0; i < textures.size(); i++)
      {
        glActiveTexture(GL_TEXTURE0 + i);

        std::string number = std::to_string(textureCounters[textures[i]->GetType()]++);
        s_Data._shaders.ModelShader->setInt(textures[i]->GetType() + number, i);
        glBindTexture(GL_TEXTURE_2D, textures[i]->GetRendererID());
      }
      
      glBindVertexArray(mesh.VAO);
      glDrawElements(GL_TRIANGLES, static_cast<GLuint>(mesh.m_Indices.size()), GL_UNSIGNED_INT, 0);
      glBindVertexArray(0);
      glActiveTexture(GL_TEXTURE0);
    }

    /*for (auto& mesh : convex->GetMeshes())*/
    /*{*/
    /*  glBindVertexArray(mesh.VAO);*/
    /*  glDrawElements(GL_LINES, static_cast<GLuint>(mesh.m_Indices.size()), GL_UNSIGNED_INT, 0);*/
    /*  glBindVertexArray(0);*/
    /*}*/
  }
}

void Renderer::DrawModelInstanced(const std::shared_ptr<Model>& model, const std::vector<Transform>& instances, int entityID)
{
  s_Data._shaders.ModelShader->Use();
  s_Data._shaders.ModelShader->setBool("isAnimated", model->IsAnimated());
  s_Data._shaders.ModelShader->setBool("isInstanced", true);

  if (model->IsAnimated())
  {
      auto& transforms = model->GetFinalBoneMatrices();
      for (size_t i = 0; i < transforms.size(); ++i)
      {
          s_Data._shaders.ModelShader->setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);
      }
  }

  for (auto& mesh : model->GetMeshes())
  {
      ModelManager::BakeModelInstancedBuffers(mesh, instances); // Upload instance data

      // Bind all textures
      std::unordered_map<std::string, GLuint> textureCounters = {
          {"texture_diffuse", 1},
          {"texture_specular", 1},
          {"texture_normal", 1},
          {"texture_height", 1}
      };

      auto& textures = mesh.m_Textures;
      for (GLuint i = 0; i < textures.size(); i++) {
          glActiveTexture(GL_TEXTURE0 + i);
          std::string number = std::to_string(textureCounters[textures[i]->GetType()]++);
          s_Data._shaders.ModelShader->setInt(textures[i]->GetType() + number, i);
          glBindTexture(GL_TEXTURE_2D, textures[i]->GetRendererID());
      }

      glBindVertexArray(mesh.VAO);
      glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLuint>(mesh.m_Indices.size()), GL_UNSIGNED_INT, 0, static_cast<GLsizei>(instances.size()));
      glBindVertexArray(0);
      glActiveTexture(GL_TEXTURE0);
  }
}

void Renderer::BakeSkyboxTextures(const std::string& name, const std::shared_ptr<Texture>& cubemap)
{
  Timer timer;

  auto channels = cubemap->GetChannels();
  auto width = cubemap->GetWidth();
  auto height = cubemap->GetHeight();
  auto pixels = cubemap->GetPixels();

  uint32_t rendererID = 0;
  glGenTextures(1, &rendererID);
  GABGL_ASSERT(rendererID != 0, "Failed to generate texture ID!");

  cubemap->SetRendererID(rendererID); // ✅ assign generated ID to the Texture

  glBindTexture(GL_TEXTURE_CUBE_MAP, rendererID);

  GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

  for (int i = 0; i < 6; ++i)
  {
      glTexImage2D(
          GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
          0,
          format,
          width,
          height,
          0,
          format,
          GL_UNSIGNED_BYTE,
          pixels[i]
      );

      stbi_image_free(pixels[i]); // ✅ free after upload
      pixels[i] = nullptr;
  }

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

  s_Data.skyboxes[name] = std::move(cubemap);
  GABGL_WARN("Skybox uploading took {0} ms", timer.ElapsedMillis());
}

void Renderer::DrawSkybox(const std::string& name)
{
  static uint32_t SkyboxVAO = 0, SkyboxVBO = 0;
  if (SkyboxVAO == 0)
  {

    float skyboxVertices[] = {
        // positions          
       -1.0f,  1.0f, -1.0f,
       -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
       -1.0f,  1.0f, -1.0f,

       -1.0f, -1.0f,  1.0f,
       -1.0f, -1.0f, -1.0f,
       -1.0f,  1.0f, -1.0f,
       -1.0f,  1.0f, -1.0f,
       -1.0f,  1.0f,  1.0f,
       -1.0f, -1.0f,  1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

       -1.0f, -1.0f,  1.0f,
       -1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f, -1.0f,  1.0f,
       -1.0f, -1.0f,  1.0f,

       -1.0f,  1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
       -1.0f,  1.0f,  1.0f,
       -1.0f,  1.0f, -1.0f,

       -1.0f, -1.0f, -1.0f,
       -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
       -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f,  1.0f
    };

    glGenVertexArrays(1, &SkyboxVAO);
    glGenBuffers(1, &SkyboxVBO);

    glBindVertexArray(SkyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, SkyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

  }

  glDepthFunc(GL_LEQUAL); 

  s_Data._shaders.skyboxShader->Use();

  glActiveTexture(GL_TEXTURE0);
  auto it = s_Data.skyboxes.find(name);
  if (it != s_Data.skyboxes.end())
  {
      glBindTexture(GL_TEXTURE_CUBE_MAP, it->second->GetRendererID());
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
}

void Renderer::Draw3DText(const Font* font, const std::string& text, const glm::vec2& position, float size, const glm::vec4& color, int entityID)
{
  if (font->m_Characters.empty() || font == nullptr)
  {
    GABGL_ERROR("Font is nullptr");
    return;
  }

  // --- Precompute total width and height for centering ---
  float textWidth = 0.0f;
  float maxBearingY = 0.0f;
  float maxBelowBaseline = 0.0f;

  for (char c : text)
  {
      auto it = font->m_Characters.find(c);
      if (it == font->m_Characters.end())
          continue;

      const auto& ch = it->second;
      textWidth += (ch.Advance >> 6) * size;

      float bearingY = ch.Bearing.y * size;
      float belowBaseline = (ch.Size.y - ch.Bearing.y) * size;

      if (bearingY > maxBearingY) maxBearingY = bearingY;
      if (belowBaseline > maxBelowBaseline) maxBelowBaseline = belowBaseline;
  }

  float totalHeight = maxBearingY + maxBelowBaseline;

  // Center the starting position
  float x = position.x - textWidth * 0.5f;
  float y = position.y - totalHeight * 0.5f;

  // --- Render characters ---
  for (char c : text)
  {
      auto it = font->m_Characters.find(c);
      if (it == font->m_Characters.end())
          continue;

      const Character& ch = it->second;

      if (s_Data._3DQuadIndexCount >= RendererData::MaxIndices)
          NextBatch();

      float xpos = x + ch.Bearing.x * size;
      float ypos = y + (maxBearingY - ch.Bearing.y * size);  // Adjust Y using max bearing

      float w = ch.Size.x * size;
      float h = ch.Size.y * size;

      glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(xpos, ypos, 0.0f)) *
                            glm::scale(glm::mat4(1.0f), glm::vec3(w, h, 1.0f));

      float textureIndex = 0.0f;
      for (uint32_t i = 1; i < s_Data._3DTextureSlotIndex; i++) {
          if (s_Data._3DTextureSlots[i]->GetRendererID() == ch.TextureID) {
              textureIndex = (float)i;
              break;
          }
      }

      if (textureIndex == 0.0f) {
          if (s_Data._3DTextureSlotIndex >= RendererData::MaxTextureSlots) NextBatch();

          textureIndex = (float)s_Data._3DTextureSlotIndex;
          s_Data._3DTextureSlots[s_Data._3DTextureSlotIndex++] = Texture::WrapExisting(ch.TextureID);
      }

      for (int i = 0; i < 4; i++) {
          s_Data._3DQuadVertexBufferPtr->Position = transform * glm::vec4(s_Data.quadPositions[i], 1.0f);
          s_Data._3DQuadVertexBufferPtr->Color = color;
          s_Data._3DQuadVertexBufferPtr->TexCoord = s_Data.texCoords[i];
          s_Data._3DQuadVertexBufferPtr->TexIndex = textureIndex;
          s_Data._3DQuadVertexBufferPtr->TilingFactor = 1.0f;
          s_Data._3DQuadVertexBufferPtr->EntityID = entityID;
          s_Data._3DQuadVertexBufferPtr++;
      }

      s_Data._3DQuadIndexCount += 6;

      x += (ch.Advance >> 6) * size;
  }
}

void Renderer::Draw2DText(const Font* font, const std::string& text, const glm::vec2& position, float size, const glm::vec4& color, int entityID)
{
	if (font->m_Characters.empty() || font == nullptr)
	{
		GABGL_ERROR("Font is nullptr");
		return;
	}

  // --- Precompute total width and height for centering ---
  float textWidth = 0.0f;
  float maxBearingY = 0.0f;
  float maxBelowBaseline = 0.0f;

  for (char c : text)
  {
    auto it = font->m_Characters.find(c);
    if (it == font->m_Characters.end())
        continue;

    const auto& ch = it->second;
    textWidth += (ch.Advance >> 6) * size;

    float bearingY = ch.Bearing.y * size;
    float belowBaseline = (ch.Size.y - ch.Bearing.y) * size;

    if (bearingY > maxBearingY) maxBearingY = bearingY;
    if (belowBaseline > maxBelowBaseline) maxBelowBaseline = belowBaseline;
  }

  float totalHeight = maxBearingY + maxBelowBaseline;

  // Center the starting position
  float x = position.x - textWidth * 0.5f;
  float y = position.y - totalHeight * 0.5f;

  // --- Render characters ---
  for (char c : text)
  {
    auto it = font->m_Characters.find(c);
    if (it == font->m_Characters.end())
        continue;

    const Character& ch = it->second;

    if (s_Data._2DQuadIndexCount >= RendererData::MaxIndices)
        NextBatch();

    float xpos = x + ch.Bearing.x * size;
    float ypos = y + (maxBearingY - ch.Bearing.y * size);  // Adjust Y using max bearing

    float w = ch.Size.x * size;
    float h = ch.Size.y * size;

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(xpos, ypos, 0.0f)) *
                          glm::scale(glm::mat4(1.0f), glm::vec3(w, h, 1.0f));

    float textureIndex = 0.0f;
    for (uint32_t i = 1; i < s_Data._2DTextureSlotIndex; i++) {
        if (s_Data._2DTextureSlots[i]->GetRendererID() == ch.TextureID) {
            textureIndex = (float)i;
            break;
        }
    }

    if (textureIndex == 0.0f) {
        if (s_Data._2DTextureSlotIndex >= RendererData::MaxTextureSlots) NextBatch();

        textureIndex = (float)s_Data._2DTextureSlotIndex;
        s_Data._2DTextureSlots[s_Data._2DTextureSlotIndex++] = Texture::WrapExisting(ch.TextureID);
    }

    for (int i = 0; i < 4; i++) {
        s_Data._2DQuadVertexBufferPtr->Position = transform * glm::vec4(s_Data.quadPositions[i], 1.0f);
        s_Data._2DQuadVertexBufferPtr->Color = color;
        s_Data._2DQuadVertexBufferPtr->TexCoord = s_Data.texCoords[i];
        s_Data._2DQuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data._2DQuadVertexBufferPtr->TilingFactor = 1.0f;
        s_Data._2DQuadVertexBufferPtr->EntityID = entityID;
        s_Data._2DQuadVertexBufferPtr++;
    }

    s_Data._2DQuadIndexCount += 6;

    x += (ch.Advance >> 6) * size;
  }
}

void Renderer::OnWindowResize(uint32_t width, uint32_t height)
{
	SetViewport(0, 0, width, height);
}

void Renderer::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	glViewport(x, y, width, height);
}

void Renderer::SetClearColor(const glm::vec4& color)
{
	glClearColor(color.r, color.g, color.b, color.a);
}

void Renderer::ClearBuffers()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::ClearColorBuffers()
{
	glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::ClearDepthBuffers()
{
	glClear(GL_DEPTH_BUFFER_BIT);
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
	static ImGuiDockNodeFlags dockspace_flags = ImGuiWindowFlags_NoCollapse | ImGuiDockNodeFlags_NoTabBar;

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

