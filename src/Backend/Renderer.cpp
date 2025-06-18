#include "Renderer.h"

#include <array>
#include "BackendLogger.h"
#include "Buffer.h"
#include "Renderer.h"
#include "Shader.h"
#include "Texture.h"
#include "glad/glad.h"
#include "glm/fwd.hpp"
#include <cstdint>
#include <stb_image.h>
#include <filesystem>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "../engine.h"
#include "Audio.h"
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

  int EntityID;        
};

struct Character
{
  uint32_t TextureID;
  glm::ivec2 Size;
  glm::ivec2 Bearing;
  uint32_t Advance;
};

struct CameraData
{
	glm::mat4 ViewProjection;
  glm::mat4 NonRotViewProjection;
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

  struct Shaders2D
	{
		std::shared_ptr<Shader> _3DQuadShader;
		std::shared_ptr<Shader> _2DQuadShader;
		std::shared_ptr<Shader> _CircleShader;
		std::shared_ptr<Shader> _LineShader;
		std::shared_ptr<Shader> _FramebufferShader;
	} _shaders2D;

  struct Shaders3D
  {
      std::shared_ptr<Shader> modelShader;
      std::shared_ptr<Shader> skyboxShader;
      std::shared_ptr<Shader> ModelShader;
  } _shaders3D;

	Renderer::Statistics _2DStats;
  Renderer::Statistics _3DStats;

  CameraData _3DCameraBuffer;
  std::shared_ptr<UniformBuffer> _3DCameraUniformBuffer;

  CameraData _2DCameraBuffer;
  std::shared_ptr<UniformBuffer> _2DCameraUniformBuffer;

  std::shared_ptr<StorageBuffer> LightPosStorageBuffer;
  std::shared_ptr<StorageBuffer> LightQuantityStorageBuffer;
  std::shared_ptr<StorageBuffer> LightColorStorageBuffer;
  std::shared_ptr<StorageBuffer> LightTypeStorageBuffer;

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
      {{-0.5f, -0.5f,  0.5f}, {0, 0, 1}, {1.0f, 1.0f, 1.0f, 1.0f}, 0},
      {{ 0.5f, -0.5f,  0.5f}, {0, 0, 1}, {1.0f, 1.0f, 1.0f, 1.0f}, 0},
      {{ 0.5f,  0.5f,  0.5f}, {0, 0, 1}, {1.0f, 1.0f, 1.0f, 1.0f}, 0},
      {{-0.5f,  0.5f,  0.5f}, {0, 0, 1}, {1.0f, 1.0f, 1.0f, 1.0f}, 0},

      {{-0.5f, -0.5f, -0.5f}, {0, 0, -1}, {1.0f, 1.0f, 1.0f, 1.0f}, 0},
      {{ 0.5f, -0.5f, -0.5f}, {0, 0, -1}, {1.0f, 1.0f, 1.0f, 1.0f}, 0},
      {{ 0.5f,  0.5f, -0.5f}, {0, 0, -1}, {1.0f, 1.0f, 1.0f, 1.0f}, 0},
      {{-0.5f,  0.5f, -0.5f}, {0, 0, -1}, {1.0f, 1.0f, 1.0f, 1.0f}, 0},
  };
  
	std::array<std::shared_ptr<Texture>, MaxTextureSlots> _3DTextureSlots;
	uint32_t _3DTextureSlotIndex = 1; // 0 = white texture
	std::array<std::shared_ptr<Texture>, MaxTextureSlots> _2DTextureSlots;
	uint32_t _2DTextureSlotIndex = 1; // 0 = white texture

  std::unordered_map<std::string, std::shared_ptr<Texture>> skyboxes;
  std::unordered_map<std::string, std::shared_ptr<Model>> models;
  std::unordered_map<char, Character> Characters;

  std::shared_ptr<FrameBuffer> m_Framebuffer;

  Window* m_WindowRef = nullptr;
  Camera m_Camera;

  enum class SceneState
	{
		Edit = 0, Play = 1
	} m_SceneState;

  FT_Library ft;

} s_RendererData;

void Renderer::LoadShaders()
{
	s_RendererData._shaders2D._3DQuadShader = Shader::Create("res/shaders/Renderer2D_Quad.glsl");
	s_RendererData._shaders2D._2DQuadShader = Shader::Create("res/shaders/Renderer2D_2DQuad.glsl");
	s_RendererData._shaders2D._CircleShader = Shader::Create("res/shaders/Renderer2D_Circle.glsl");
	s_RendererData._shaders2D._LineShader = Shader::Create("res/shaders/Renderer2D_Line.glsl");
	s_RendererData._shaders2D._FramebufferShader = Shader::Create("res/shaders/FB.glsl");
  s_RendererData._shaders3D.modelShader = Shader::Create("res/shaders/Renderer3D_static.glsl");
  s_RendererData._shaders3D.skyboxShader = Shader::Create("res/shaders/skybox.glsl");
  s_RendererData._shaders3D.ModelShader = Shader::Create("res/shaders/Renderer3D_Model.glsl");
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

	s_RendererData._3DQuadVertexArray = VertexArray::Create();
	s_RendererData._3DQuadVertexBuffer = VertexBuffer::Create(s_RendererData.MaxVertices * sizeof(QuadVertex));
	s_RendererData._3DQuadVertexBuffer->SetLayout({
		{ ShaderDataType::Float3, "a_Position"     },
		{ ShaderDataType::Float4, "a_Color"        },
		{ ShaderDataType::Float2, "a_TexCoord"     },
		{ ShaderDataType::Float,  "a_TexIndex"     },
		{ ShaderDataType::Float,  "a_TilingFactor" },
		{ ShaderDataType::Int,    "a_EntityID"     }
		});
	s_RendererData._3DQuadVertexArray->AddVertexBuffer(s_RendererData._3DQuadVertexBuffer);
	s_RendererData._3DQuadVertexBufferBase = new QuadVertex[s_RendererData.MaxVertices];

  s_RendererData._2DQuadVertexArray = VertexArray::Create();
	s_RendererData._2DQuadVertexBuffer = VertexBuffer::Create(s_RendererData.MaxVertices * sizeof(QuadVertex));
	s_RendererData._2DQuadVertexBuffer->SetLayout({
		{ ShaderDataType::Float3, "a_Position"     },
		{ ShaderDataType::Float4, "a_Color"        },
		{ ShaderDataType::Float2, "a_TexCoord"     },
		{ ShaderDataType::Float,  "a_TexIndex"     },
		{ ShaderDataType::Float,  "a_TilingFactor" },
		{ ShaderDataType::Int,    "a_EntityID"     }
		});
	s_RendererData._2DQuadVertexArray->AddVertexBuffer(s_RendererData._2DQuadVertexBuffer);
	s_RendererData._2DQuadVertexBufferBase = new QuadVertex[s_RendererData.MaxVertices];

	uint32_t* quadIndices = new uint32_t[s_RendererData.MaxIndices];

	uint32_t offset = 0;
	for (uint32_t i = 0; i < s_RendererData.MaxIndices; i += 6)
	{
		quadIndices[i + 0] = offset + 0;
		quadIndices[i + 1] = offset + 1;
		quadIndices[i + 2] = offset + 2;

		quadIndices[i + 3] = offset + 2;
		quadIndices[i + 4] = offset + 3;
		quadIndices[i + 5] = offset + 0;

		offset += 4;
	}

	std::shared_ptr<IndexBuffer> _3DquadIB = IndexBuffer::Create(quadIndices, s_RendererData.MaxIndices);
	s_RendererData._3DQuadVertexArray->SetIndexBuffer(_3DquadIB);
	std::shared_ptr<IndexBuffer> _2DquadIB = IndexBuffer::Create(quadIndices, s_RendererData.MaxIndices);
	s_RendererData._2DQuadVertexArray->SetIndexBuffer(_2DquadIB);
	delete[] quadIndices;

	// Circles
	s_RendererData.CircleVertexArray = VertexArray::Create();
	s_RendererData.CircleVertexBuffer = VertexBuffer::Create(s_RendererData.MaxVertices * sizeof(CircleVertex));
	s_RendererData.CircleVertexBuffer->SetLayout({
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
	s_RendererData.CircleVertexArray->AddVertexBuffer(s_RendererData.CircleVertexBuffer);
	s_RendererData.CircleVertexArray->SetIndexBuffer(_3DquadIB); // Use quad IB
	s_RendererData.CircleVertexBufferBase = new CircleVertex[s_RendererData.MaxVertices];

	// Lines
	s_RendererData.LineVertexArray = VertexArray::Create();
	s_RendererData.LineVertexBuffer = VertexBuffer::Create(s_RendererData.MaxVertices * sizeof(LineVertex));
	s_RendererData.LineVertexBuffer->SetLayout({
		{ ShaderDataType::Float3, "a_Position" },
		{ ShaderDataType::Float4, "a_Color"    },
		{ ShaderDataType::Int,    "a_EntityID" }
		});
	s_RendererData.LineVertexArray->AddVertexBuffer(s_RendererData.LineVertexBuffer);
	s_RendererData.LineVertexBufferBase = new LineVertex[s_RendererData.MaxVertices];

	s_RendererData.WhiteTexture = Texture::Create(TextureSpecification());
	uint32_t whiteTextureData = 0xffffffff;
	s_RendererData.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

	int32_t samplers[s_RendererData.MaxTextureSlots];
	for (uint32_t i = 0; i < s_RendererData.MaxTextureSlots; i++)
		samplers[i] = i;

	s_RendererData._3DTextureSlots[0] = s_RendererData.WhiteTexture;
	s_RendererData._2DTextureSlots[0] = s_RendererData.WhiteTexture;

	s_RendererData._3DQuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
	s_RendererData._3DQuadVertexPositions[1] = { 0.5f, -0.5f, 0.0f, 1.0f };
	s_RendererData._3DQuadVertexPositions[2] = { 0.5f,  0.5f, 0.0f, 1.0f };
	s_RendererData._3DQuadVertexPositions[3] = { -0.5f,  0.5f, 0.0f, 1.0f };

  s_RendererData.CubeVertexBufferBase = new MeshVertex[s_RendererData.MaxVertices];
  s_RendererData.CubeVertexBuffer = VertexBuffer::Create(nullptr, s_RendererData.MaxVertices * sizeof(MeshVertex));
  s_RendererData.CubeVertexBuffer->SetLayout({
      { ShaderDataType::Float3, "a_Position" },   // 0
      { ShaderDataType::Float3, "a_Normal" },     // 1
      { ShaderDataType::Float4, "a_Color" },      // 3
      { ShaderDataType::Int,    "a_EntityID" }    // 5
  });

  std::vector<uint32_t> indices;
  indices.reserve(s_RendererData.MaxIndices);

  const uint32_t cubeIndicesCount = 36;

  uint32_t cubeIndices[36] = {
      0, 1, 2, 2, 3, 0,       // Front
      4, 5, 6, 6, 7, 4,       // Back
      4, 0, 3, 3, 7, 4,       // Left
      1, 5, 6, 6, 2, 1,       // Right
      3, 2, 6, 6, 7, 3,       // Top
      4, 5, 1, 1, 0, 4        // Bottom
  };

  uint32_t maxCubes = s_RendererData.MaxVertices / 8; // 8 vertices per cube
  for (uint32_t i = 0; i < maxCubes; i++)
  {
      for (uint32_t j = 0; j < cubeIndicesCount; j++)
      {
          indices.push_back(cubeIndices[j] + i * 8);
      }
  }

  s_RendererData.CubeIndexBuffer = IndexBuffer::Create(indices.data(), (uint32_t)indices.size());
  s_RendererData.CubeVertexArray = VertexArray::Create();
  s_RendererData.CubeVertexArray->AddVertexBuffer(s_RendererData.CubeVertexBuffer);
  s_RendererData.CubeVertexArray->SetIndexBuffer(s_RendererData.CubeIndexBuffer);
  s_RendererData.CubeVertexBufferPtr = s_RendererData.CubeVertexBufferBase;

  s_RendererData._3DCameraUniformBuffer = UniformBuffer::Create(sizeof(CameraData), 0);
  s_RendererData._2DCameraUniformBuffer = UniformBuffer::Create(sizeof(CameraData), 1);
  s_RendererData.LightPosStorageBuffer = StorageBuffer::Create(sizeof(glm::vec3) * 10, 0);
  s_RendererData.LightQuantityStorageBuffer= StorageBuffer::Create(sizeof(uint32_t) * 10, 1);
  s_RendererData.LightColorStorageBuffer= StorageBuffer::Create(sizeof(glm::vec4) * 10, 2);
  s_RendererData.LightTypeStorageBuffer= StorageBuffer::Create(sizeof(uint32_t) * 10, 3);

  FramebufferSpecification fbSpec;
	fbSpec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
	fbSpec.Width = Engine::GetInstance().GetMainWindow().GetWidth();
	fbSpec.Height = Engine::GetInstance().GetMainWindow().GetHeight();
	s_RendererData.m_Framebuffer = FrameBuffer::Create(fbSpec);

  LoadShaders();

  GABGL_ASSERT(!FT_Init_FreeType(&s_RendererData.ft), "Could not init FreeType");

  LoadFont("res/fonts/dpcomic.ttf");

  s_RendererData.m_SceneState = RendererData::SceneState::Edit;

  s_RendererData.m_WindowRef = &Engine::GetInstance().GetMainWindow();

  s_RendererData.m_Camera = Camera(45.0f, (float)s_RendererData.m_WindowRef->GetWidth() / (float)s_RendererData.m_WindowRef->GetHeight(), 0.01f, 1000.0f);
  s_RendererData.m_Camera.SetViewportSize((float)s_RendererData.m_WindowRef->GetWidth(), (float)s_RendererData.m_WindowRef->GetHeight());

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

	GLFWwindow* window = reinterpret_cast<GLFWwindow*>(s_RendererData.m_WindowRef->GetWindowPtr());

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 410");
	SetLineWidth(4.0f);
  s_RendererData.m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
}

void Renderer::Shutdown()
{
	delete[] s_RendererData._3DQuadVertexBufferBase;
	delete[] s_RendererData._2DQuadVertexBufferBase;

  delete[] s_RendererData.CubeVertexBufferBase;
  s_RendererData.CubeVertexBufferBase = nullptr;

  s_RendererData.CubeVertexArray.reset();
  s_RendererData.CubeVertexBuffer.reset();
  s_RendererData.CubeIndexBuffer.reset();

  ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void Renderer::RenderScene(DeltaTime& dt, const std::function<void()>& pre)
{
	ResetStats();
	s_RendererData.m_Framebuffer->Bind();
	s_RendererData.m_Framebuffer->ClearAttachment(1, -1);
  SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
	Clear();
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_FRONT);
  glFrontFace(GL_CW);

	BeginScene(s_RendererData.m_Camera);

	 pre();

	EndScene();

  glDisable(GL_CULL_FACE);
	s_RendererData.m_Framebuffer->Unbind();
  SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
	Clear();
  glDisable(GL_DEPTH_TEST);


  s_RendererData.m_Camera.OnUpdate(dt);
  PhysX::Simulate(dt);
  AudioSystem::UpdateAllMusic();

  if(Input::IsKeyPressed(Key::E))
  {
    s_RendererData.m_SceneState = RendererData::SceneState::Edit;
  }
  if(Input::IsKeyPressed(Key::Q))
  {
    s_RendererData.m_SceneState = RendererData::SceneState::Play;
  }

	switch (s_RendererData.m_SceneState)
	{
	   case RendererData::SceneState::Edit:
		{
	     s_RendererData.m_WindowRef->SetCursorVisible(true);
       Renderer::DrawEditorFrameBuffer(s_RendererData.m_Framebuffer->GetColorAttachmentRendererID());
			break;
		}
	   case RendererData::SceneState::Play:
		{
	     s_RendererData.m_WindowRef->SetCursorVisible(false);
	     Renderer::DrawFramebuffer(s_RendererData.m_Framebuffer->GetColorAttachmentRendererID());
			break;
		}
	}
}

void Renderer::SetFullscreen(const std::string& sound, bool windowed)
{
  s_RendererData.m_WindowRef->SetFullscreen(windowed);

  uint32_t width = (uint32_t)s_RendererData.m_WindowRef->GetWidth(); 
  uint32_t height = (uint32_t)s_RendererData.m_WindowRef->GetHeight(); 

  s_RendererData.m_Framebuffer->Resize(width, height);
  s_RendererData.m_Camera.SetViewportSize(width, height);
  AudioSystem::PlaySound(sound);
}

void Renderer::StartBatch()
{
	s_RendererData._3DQuadIndexCount = 0;
	s_RendererData._3DQuadVertexBufferPtr = s_RendererData._3DQuadVertexBufferBase;
	s_RendererData._3DTextureSlotIndex = 1;

	s_RendererData._2DQuadIndexCount = 0;
	s_RendererData._2DQuadVertexBufferPtr = s_RendererData._2DQuadVertexBufferBase;
	s_RendererData._2DTextureSlotIndex = 1;

	s_RendererData.CircleIndexCount = 0;
	s_RendererData.CircleVertexBufferPtr = s_RendererData.CircleVertexBufferBase;

	s_RendererData.LineVertexCount = 0;
	s_RendererData.LineVertexBufferPtr = s_RendererData.LineVertexBufferBase;

  s_RendererData.CubeVertexCount = 0;
  s_RendererData.CubeIndexCount = 0;
  s_RendererData.CubeVertexBufferPtr = s_RendererData.CubeVertexBufferBase;
}

void Renderer::BeginScene(const Camera& camera, const glm::mat4& transform)
{
	s_RendererData._3DCameraBuffer.ViewProjection = camera.GetProjection() * glm::inverse(transform);
  s_RendererData._3DCameraBuffer.NonRotViewProjection = camera.GetNonRotationViewProjection();
	s_RendererData._3DCameraUniformBuffer->SetData(&s_RendererData._3DCameraBuffer, sizeof(CameraData));

	s_RendererData._2DCameraBuffer.ViewProjection = camera.GetOrtoProjection() * glm::inverse(transform);
  s_RendererData._2DCameraUniformBuffer->SetData(&s_RendererData._2DCameraBuffer, sizeof(CameraData));

	StartBatch();
}

void Renderer::BeginScene(const Camera& camera)
{
	s_RendererData._3DCameraBuffer.ViewProjection = camera.GetViewProjection();
  s_RendererData._3DCameraBuffer.NonRotViewProjection = camera.GetNonRotationViewProjection();
	s_RendererData._3DCameraUniformBuffer->SetData(&s_RendererData._3DCameraBuffer, sizeof(CameraData));

	s_RendererData._2DCameraBuffer.ViewProjection = camera.GetOrtoProjection();
  s_RendererData._2DCameraUniformBuffer->SetData(&s_RendererData._2DCameraBuffer, sizeof(CameraData));

	StartBatch();
}

void Renderer::Flush()
{
	if (s_RendererData._3DQuadIndexCount)
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_RendererData._3DQuadVertexBufferPtr - (uint8_t*)s_RendererData._3DQuadVertexBufferBase);
		s_RendererData._3DQuadVertexBuffer->SetData(s_RendererData._3DQuadVertexBufferBase, dataSize);

		for (uint32_t i = 0; i < s_RendererData._3DTextureSlotIndex; i++)
			s_RendererData._3DTextureSlots[i]->Bind(i);

		s_RendererData._shaders2D._3DQuadShader->Use();
		Renderer::DrawIndexed(s_RendererData._3DQuadVertexArray, s_RendererData._3DQuadIndexCount);
		s_RendererData._2DStats.DrawCalls++;
	}
	if (s_RendererData._2DQuadIndexCount)
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_RendererData._2DQuadVertexBufferPtr - (uint8_t*)s_RendererData._2DQuadVertexBufferBase);
		s_RendererData._2DQuadVertexBuffer->SetData(s_RendererData._2DQuadVertexBufferBase, dataSize);

		for (uint32_t i = 0; i < s_RendererData._2DTextureSlotIndex; i++)
			s_RendererData._2DTextureSlots[i]->Bind(i);

		s_RendererData._shaders2D._2DQuadShader->Use();
		Renderer::DrawIndexed(s_RendererData._2DQuadVertexArray, s_RendererData._2DQuadIndexCount);
		s_RendererData._2DStats.DrawCalls++;
	}
	if (s_RendererData.CircleIndexCount)
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_RendererData.CircleVertexBufferPtr - (uint8_t*)s_RendererData.CircleVertexBufferBase);
		s_RendererData.CircleVertexBuffer->SetData(s_RendererData.CircleVertexBufferBase, dataSize);

		s_RendererData._shaders2D._CircleShader->Use();
		Renderer::DrawIndexed(s_RendererData.CircleVertexArray, s_RendererData.CircleIndexCount);
		s_RendererData._2DStats.DrawCalls++;
	}
	if (s_RendererData.LineVertexCount)
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_RendererData.LineVertexBufferPtr - (uint8_t*)s_RendererData.LineVertexBufferBase);
		s_RendererData.LineVertexBuffer->SetData(s_RendererData.LineVertexBufferBase, dataSize);

		s_RendererData._shaders2D._LineShader->Use();
		Renderer::SetLineWidth(s_RendererData.LineWidth);
		Renderer::DrawLines(s_RendererData.LineVertexArray, s_RendererData.LineVertexCount);
		s_RendererData._2DStats.DrawCalls++;
	}
  if (s_RendererData.CubeIndexCount) 
  {
    uint32_t dataSize = (uint32_t)((uint8_t*)s_RendererData.CubeVertexBufferPtr - (uint8_t*)s_RendererData.CubeVertexBufferBase);
    s_RendererData.CubeVertexBuffer->SetData(s_RendererData.CubeVertexBufferBase, dataSize);

    s_RendererData._shaders3D.modelShader->Use();
    s_RendererData.CubeVertexArray->Bind();

    Renderer::DrawIndexed(s_RendererData.CubeVertexArray, s_RendererData.CubeIndexCount);

    s_RendererData._3DStats.DrawCalls++;
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

	if (s_RendererData._3DQuadIndexCount >= RendererData::MaxIndices)
		NextBatch();

	for (size_t i = 0; i < quadVertexCount; i++)
	{
		s_RendererData._3DQuadVertexBufferPtr->Position = transform * s_RendererData._3DQuadVertexPositions[i];
		s_RendererData._3DQuadVertexBufferPtr->Color = color;
		s_RendererData._3DQuadVertexBufferPtr->TexCoord = textureCoords[i];
		s_RendererData._3DQuadVertexBufferPtr->TexIndex = textureIndex;
		s_RendererData._3DQuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_RendererData._3DQuadVertexBufferPtr->EntityID = entityID;
		s_RendererData._3DQuadVertexBufferPtr++;
	}

	s_RendererData._3DQuadIndexCount += 6;

	s_RendererData._2DStats.QuadCount++;
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

	if (s_RendererData._3DQuadIndexCount >= RendererData::MaxIndices)
		NextBatch();

	float textureIndex = 0.0f;
	for (uint32_t i = 1; i < s_RendererData._3DTextureSlotIndex; i++)
	{
		if (*s_RendererData._3DTextureSlots[i] == *texture)
		{
			textureIndex = (float)i;
			break;
		}
	}

	if (textureIndex == 0.0f)
	{
		if (s_RendererData._3DTextureSlotIndex >= RendererData::MaxTextureSlots)
			NextBatch();

		textureIndex = (float)s_RendererData._3DTextureSlotIndex;
		s_RendererData._3DTextureSlots[s_RendererData._3DTextureSlotIndex] = texture;
		s_RendererData._3DTextureSlotIndex++;
	}

	for (size_t i = 0; i < quadVertexCount; i++)
	{
		s_RendererData._3DQuadVertexBufferPtr->Position = transform * s_RendererData._3DQuadVertexPositions[i];
		s_RendererData._3DQuadVertexBufferPtr->Color = tintColor;
		s_RendererData._3DQuadVertexBufferPtr->TexCoord = textureCoords[i];
		s_RendererData._3DQuadVertexBufferPtr->TexIndex = textureIndex;
		s_RendererData._3DQuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_RendererData._3DQuadVertexBufferPtr->EntityID = entityID;
		s_RendererData._3DQuadVertexBufferPtr++;
	}

	s_RendererData._3DQuadIndexCount += 6;

	s_RendererData._2DStats.QuadCount++;
}

void Renderer::Draw3DCircle(const glm::mat4& transform, const glm::vec4& color, float thickness /*= 1.0f*/, float fade /*= 0.005f*/, int entityID /*= -1*/)
{
	// TODO: implement for circles
	// if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
	// 	NextBatch();

	for (size_t i = 0; i < 4; i++)
	{
		s_RendererData.CircleVertexBufferPtr->WorldPosition = transform * s_RendererData._3DQuadVertexPositions[i];
		s_RendererData.CircleVertexBufferPtr->LocalPosition = s_RendererData._3DQuadVertexPositions[i] * 2.0f;
		s_RendererData.CircleVertexBufferPtr->Color = color;
		s_RendererData.CircleVertexBufferPtr->Thickness = thickness;
		s_RendererData.CircleVertexBufferPtr->Fade = fade;
		s_RendererData.CircleVertexBufferPtr->EntityID = entityID;
		s_RendererData.CircleVertexBufferPtr++;
	}

	s_RendererData.CircleIndexCount += 6;

	s_RendererData._2DStats.QuadCount++;
}

void Renderer::DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, int entityID)
{
	s_RendererData.LineVertexBufferPtr->Position = p0;
	s_RendererData.LineVertexBufferPtr->Color = color;
	s_RendererData.LineVertexBufferPtr->EntityID = entityID;
	s_RendererData.LineVertexBufferPtr++;

	s_RendererData.LineVertexBufferPtr->Position = p1;
	s_RendererData.LineVertexBufferPtr->Color = color;
	s_RendererData.LineVertexBufferPtr->EntityID = entityID;
	s_RendererData.LineVertexBufferPtr++;

	s_RendererData.LineVertexCount += 2;
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
	for (size_t i = 0; i < 4; i++) lineVertices[i] = transform * s_RendererData._3DQuadVertexPositions[i];

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

	if (s_RendererData._2DQuadIndexCount >= RendererData::MaxIndices) NextBatch();

	for (size_t i = 0; i < quadVertexCount; i++)
	{
		s_RendererData._2DQuadVertexBufferPtr->Position = transform * s_RendererData._3DQuadVertexPositions[i];
		s_RendererData._2DQuadVertexBufferPtr->Color = color;
		s_RendererData._2DQuadVertexBufferPtr->TexCoord = textureCoords[i];
		s_RendererData._2DQuadVertexBufferPtr->TexIndex = textureIndex;
		s_RendererData._2DQuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_RendererData._2DQuadVertexBufferPtr->EntityID = entityID;
		s_RendererData._2DQuadVertexBufferPtr++;
	}

	s_RendererData._2DQuadIndexCount += 6;

	s_RendererData._2DStats.QuadCount++;
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

	if (s_RendererData._2DQuadIndexCount >= RendererData::MaxIndices) NextBatch();

	float textureIndex = 0.0f;
	for (uint32_t i = 1; i < s_RendererData._2DTextureSlotIndex; i++)
	{
		if (*s_RendererData._2DTextureSlots[i] == *texture)
		{
			textureIndex = (float)i;
			break;
		}
	}

	if (textureIndex == 0.0f)
	{
		if (s_RendererData._2DTextureSlotIndex >= RendererData::MaxTextureSlots) NextBatch();

		textureIndex = (float)s_RendererData._2DTextureSlotIndex;
		s_RendererData._2DTextureSlots[s_RendererData._2DTextureSlotIndex] = texture;
		s_RendererData._2DTextureSlotIndex++;
	}

	for (size_t i = 0; i < quadVertexCount; i++)
	{
		s_RendererData._2DQuadVertexBufferPtr->Position = transform * s_RendererData._3DQuadVertexPositions[i];
		s_RendererData._2DQuadVertexBufferPtr->Color = tintColor;
		s_RendererData._2DQuadVertexBufferPtr->TexCoord = textureCoords[i];
		s_RendererData._2DQuadVertexBufferPtr->TexIndex = textureIndex;
		s_RendererData._2DQuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_RendererData._2DQuadVertexBufferPtr->EntityID = entityID;
		s_RendererData._2DQuadVertexBufferPtr++;
	}

	s_RendererData._2DQuadIndexCount += 6;

	s_RendererData._2DStats.QuadCount++;
}

void Renderer::Draw2DCircle(const glm::mat4& transform, const glm::vec4& color, float thickness /*= 1.0f*/, float fade /*= 0.005f*/, int entityID /*= -1*/)
{
	// TODO: implement for circles
	// if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
	// 	NextBatch();

	for (size_t i = 0; i < 4; i++)
	{
		s_RendererData.CircleVertexBufferPtr->WorldPosition = transform * s_RendererData._3DQuadVertexPositions[i];
		s_RendererData.CircleVertexBufferPtr->LocalPosition = s_RendererData._3DQuadVertexPositions[i] * 2.0f;
		s_RendererData.CircleVertexBufferPtr->Color = color;
		s_RendererData.CircleVertexBufferPtr->Thickness = thickness;
		s_RendererData.CircleVertexBufferPtr->Fade = fade;
		s_RendererData.CircleVertexBufferPtr->EntityID = entityID;
		s_RendererData.CircleVertexBufferPtr++;
	}

	s_RendererData.CircleIndexCount += 6;

	s_RendererData._2DStats.QuadCount++;
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
		lineVertices[i] = transform * s_RendererData._3DQuadVertexPositions[i];

	DrawLine(lineVertices[0], lineVertices[1], color, entityID);
	DrawLine(lineVertices[1], lineVertices[2], color, entityID);
	DrawLine(lineVertices[2], lineVertices[3], color, entityID);
	DrawLine(lineVertices[3], lineVertices[0], color, entityID);
}

void Renderer::DrawCube(const Transform& transform, int entityID)
{
  if (s_RendererData.CubeIndexCount >= RendererData::MaxIndices) NextBatch(); // Flush batch if full

  glm::mat4 model = transform.GetTransform();
  glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

  for (int i = 0; i < 8; i++)
  {
      MeshVertex vertex = s_RendererData.cubeVertices[i];

      // Transform position
      glm::vec4 transformedPos = model * glm::vec4(vertex.Position, 1.0f);
      vertex.Position = glm::vec3(transformedPos);
      vertex.Normal = glm::normalize(normalMatrix * vertex.Normal);
      vertex.Color = glm::vec4(2.0f);
      vertex.EntityID = entityID;

      *s_RendererData.CubeVertexBufferPtr = vertex;
      s_RendererData.CubeVertexBufferPtr++;
  }

  s_RendererData.CubeVertexCount += 8;
  s_RendererData.CubeIndexCount += 36; // 6 faces * 2 triangles * 3 indices = 36 indices per cube
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
  s_RendererData._shaders2D._FramebufferShader->Use();
  s_RendererData._shaders2D._FramebufferShader->setInt("u_Texture", 0);

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

void Renderer::BakeModelBuffers(const std::string& name)
{
  Timer timer;

  auto it = s_RendererData.models.find(name);
  if (it != s_RendererData.models.end())
  {
    for(auto& mesh : it->second->GetMeshes())
    {
      glGenVertexArrays(1, &mesh.VAO);
      glGenBuffers(1, &mesh.VBO);
      glGenBuffers(1, &mesh.EBO);

      glBindVertexArray(mesh.VAO);

      glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
      glBufferData(GL_ARRAY_BUFFER, mesh.m_Vertices.size() * sizeof(Vertex), &mesh.m_Vertices[0], GL_STATIC_DRAW);

      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.m_Indices.size() * sizeof(unsigned int), &mesh.m_Indices[0], GL_STATIC_DRAW);

      struct Attribute {
          GLint size;
          GLenum type;
          GLboolean normalized;
          size_t offset;
      };

      std::array<Attribute, 7> attributes = { {
          {3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Position)},
          {3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Normal)},
          {2, GL_FLOAT, GL_FALSE, offsetof(Vertex, TexCoords)},
          {3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Tangent)},
          {3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Bitangent)},
          {4, GL_INT, GL_FALSE, offsetof(Vertex, m_BoneIDs)},
          {4, GL_FLOAT, GL_FALSE, offsetof(Vertex, m_Weights)}
      } };

      for (size_t i = 0; i < attributes.size(); ++i) {
          glEnableVertexAttribArray(static_cast<GLuint>(i));
          if (attributes[i].type == GL_INT) {
              glVertexAttribIPointer(static_cast<GLuint>(i), attributes[i].size, attributes[i].type, sizeof(Vertex), (void*)attributes[i].offset);
          } else {
              glVertexAttribPointer(static_cast<GLuint>(i), attributes[i].size, attributes[i].type, attributes[i].normalized, sizeof(Vertex), (void*)attributes[i].offset);
          }
      }

      glBindVertexArray(0);

      for(auto& tex : mesh.m_Textures) tex->ClearRawData();
    }
  }
  GABGL_WARN("Model: {0} buffers baking took {1} ms", name, timer.ElapsedMillis());
}

void Renderer::BakeModelInstancedBuffers(Mesh& mesh, const std::vector<Transform>& transforms)
{
    if (transforms.empty()) return;

    // Convert transforms to matrices
    std::vector<glm::mat4> instanceMatrices;
    instanceMatrices.reserve(transforms.size());
    for (const auto& t : transforms)
        instanceMatrices.push_back(t.GetTransform());

    if (mesh.instanceVBO == 0)
        glGenBuffers(1, &mesh.instanceVBO);

    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceMatrices.size() * sizeof(glm::mat4), instanceMatrices.data(), GL_STREAM_DRAW);

    if (!mesh.instanceAttribsConfigured)
    {
        std::size_t vec4Size = sizeof(glm::vec4);
        for (int i = 0; i < 4; ++i)
        {
            GLuint loc = 7 + i;
            glEnableVertexAttribArray(loc);
            glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * vec4Size));
            glVertexAttribDivisor(loc, 1);
        }

        mesh.instanceAttribsConfigured = true;
    }

    glBindVertexArray(0);
}

void Renderer::BakeModelTextures(const std::string& path, const std::shared_ptr<Model>& model)
{
  Timer timer;

  for(auto& mesh : model->GetMeshes())
  {
    for (auto& texture : mesh.m_Textures)
    {
      GLuint id;
      glGenTextures(1, &id);
      texture->SetRendererID(id);
      glBindTexture(GL_TEXTURE_2D,id);

      if(texture->IsUnCompressed())
      {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->GetEmbeddedTexture()->mWidth, texture->GetEmbeddedTexture()->mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->GetEmbeddedTexture()->pcData);
        glGenerateMipmap(GL_TEXTURE_2D);
      }
      else
      {
        glTexImage2D(GL_TEXTURE_2D, 0, texture->GetDataFormat(), texture->GetWidth(), texture->GetHeight(), 0, texture->GetDataFormat(), GL_UNSIGNED_BYTE, texture->GetRawData());
        glGenerateMipmap(GL_TEXTURE_2D);
      }

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
  }
  std::string name = std::filesystem::path(path).stem().string();
  s_RendererData.models[name] = std::move(model);
  GABGL_WARN("Model: {0} textures baking took {1} ms", name, timer.ElapsedMillis());
}

void Renderer::DrawModel(DeltaTime& dt,const std::string& name, const glm::vec3& position, const glm::vec3& size, float rotation)
{
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
		* glm::scale(glm::mat4(1.0f), { size.x, size.y, size.z });

	DrawModel(dt,name,transform);
}

void Renderer::DrawModel(DeltaTime& dt, const std::string& name, const glm::mat4& transform, int entityID)
{
  auto it = s_RendererData.models.find(name);
  if (it != s_RendererData.models.end())
  {
    s_RendererData._shaders3D.ModelShader->Use();
    s_RendererData._shaders3D.ModelShader->setMat4("model", transform);
    s_RendererData._shaders3D.ModelShader->setBool("isAnimated", it->second->IsAnimated());
    s_RendererData._shaders3D.ModelShader->setBool("isInstanced", false);
    for (auto& mesh : it->second->GetMeshes())
    {

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
            s_RendererData._shaders3D.ModelShader->setInt(textures[i]->GetType() + number, i);
            glBindTexture(GL_TEXTURE_2D, textures[i]->GetRendererID());
        }
        
        glBindVertexArray(mesh.VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLuint>(mesh.m_Indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE0);
    }

    if(it->second->IsAnimated())
    {
      auto& transforms = it->second->GetFinalBoneMatrices();
      for (size_t i = 0; i < transforms.size(); ++i) {
          s_RendererData._shaders3D.ModelShader->setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);
      }
      it->second->UpdateAnimation(dt);
    }
    /*it->second->UpdatePhysXActor(transform);*/
  }
  else
  {
      GABGL_ERROR("Model not found: {0}", name);
      return;
  }
}

void Renderer::DrawModelInstanced(DeltaTime& dt, const std::string& name, const std::vector<Transform>& instances, int entityID)
{
    auto it = s_RendererData.models.find(name);
    if (it == s_RendererData.models.end())
    {
        GABGL_ERROR("Model not found: {0}", name);
        return;
    }

    auto& model = it->second;
    s_RendererData._shaders3D.ModelShader->Use();
    s_RendererData._shaders3D.ModelShader->setBool("isAnimated", model->IsAnimated());
    s_RendererData._shaders3D.ModelShader->setBool("isInstanced", true);
    for (auto& mesh : model->GetMeshes())
    {
        BakeModelInstancedBuffers(mesh, instances); // Upload instance data

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
            s_RendererData._shaders3D.ModelShader->setInt(textures[i]->GetType() + number, i);
            glBindTexture(GL_TEXTURE_2D, textures[i]->GetRendererID());
        }

        glBindVertexArray(mesh.VAO);
        glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLuint>(mesh.m_Indices.size()), GL_UNSIGNED_INT, 0, static_cast<GLsizei>(instances.size()));
        glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE0);
    }
  
    if (model->IsAnimated())
    {
        auto& transforms = model->GetFinalBoneMatrices();
        for (size_t i = 0; i < transforms.size(); ++i)
        {
            s_RendererData._shaders3D.ModelShader->setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);
        }
        model->UpdateAnimation(dt);
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

  s_RendererData.skyboxes[name] = std::move(cubemap);
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

  s_RendererData._shaders3D.skyboxShader->Use();
  s_RendererData._shaders3D.skyboxShader->setMat4("u_ViewProjection", s_RendererData._3DCameraBuffer.NonRotViewProjection);

  glActiveTexture(GL_TEXTURE0);
  auto it = s_RendererData.skyboxes.find(name);
  if (it != s_RendererData.skyboxes.end())
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

void Renderer::Draw3DText(const std::string& text, const glm::vec2& position, float size, const glm::vec4& color, int entityID)
{
    if (s_RendererData.Characters.empty()) return;

    // --- Precompute total width and height for centering ---
    float textWidth = 0.0f;
    float maxBearingY = 0.0f;
    float maxBelowBaseline = 0.0f;

    for (char c : text)
    {
        auto it = s_RendererData.Characters.find(c);
        if (it == s_RendererData.Characters.end())
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
        auto it = s_RendererData.Characters.find(c);
        if (it == s_RendererData.Characters.end())
            continue;

        Character& ch = it->second;

        if (s_RendererData._3DQuadIndexCount >= RendererData::MaxIndices)
            NextBatch();

        float xpos = x + ch.Bearing.x * size;
        float ypos = y + (maxBearingY - ch.Bearing.y * size);  // Adjust Y using max bearing

        float w = ch.Size.x * size;
        float h = ch.Size.y * size;

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(xpos, ypos, 0.0f)) *
                              glm::scale(glm::mat4(1.0f), glm::vec3(w, h, 1.0f));

        float textureIndex = 0.0f;
        for (uint32_t i = 1; i < s_RendererData._3DTextureSlotIndex; i++) {
            if (s_RendererData._3DTextureSlots[i]->GetRendererID() == ch.TextureID) {
                textureIndex = (float)i;
                break;
            }
        }

        if (textureIndex == 0.0f) {
            if (s_RendererData._3DTextureSlotIndex >= RendererData::MaxTextureSlots) NextBatch();

            textureIndex = (float)s_RendererData._3DTextureSlotIndex;
            s_RendererData._3DTextureSlots[s_RendererData._3DTextureSlotIndex++] = Texture::WrapExisting(ch.TextureID);
        }

        for (int i = 0; i < 4; i++) {
            s_RendererData._3DQuadVertexBufferPtr->Position = transform * glm::vec4(s_RendererData.quadPositions[i], 1.0f);
            s_RendererData._3DQuadVertexBufferPtr->Color = color;
            s_RendererData._3DQuadVertexBufferPtr->TexCoord = s_RendererData.texCoords[i];
            s_RendererData._3DQuadVertexBufferPtr->TexIndex = textureIndex;
            s_RendererData._3DQuadVertexBufferPtr->TilingFactor = 1.0f;
            s_RendererData._3DQuadVertexBufferPtr->EntityID = entityID;
            s_RendererData._3DQuadVertexBufferPtr++;
        }

        s_RendererData._3DQuadIndexCount += 6;

        x += (ch.Advance >> 6) * size;
    }

    s_RendererData._2DStats.QuadCount += (int)text.length();
}

void Renderer::Draw2DText(const std::string& text, const glm::vec2& position, float size, const glm::vec4& color, int entityID)
{
    if (s_RendererData.Characters.empty()) return;

    // --- Precompute total width and height for centering ---
    float textWidth = 0.0f;
    float maxBearingY = 0.0f;
    float maxBelowBaseline = 0.0f;

    for (char c : text)
    {
        auto it = s_RendererData.Characters.find(c);
        if (it == s_RendererData.Characters.end())
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
        auto it = s_RendererData.Characters.find(c);
        if (it == s_RendererData.Characters.end())
            continue;

        Character& ch = it->second;

        if (s_RendererData._2DQuadIndexCount >= RendererData::MaxIndices)
            NextBatch();

        float xpos = x + ch.Bearing.x * size;
        float ypos = y + (maxBearingY - ch.Bearing.y * size);  // Adjust Y using max bearing

        float w = ch.Size.x * size;
        float h = ch.Size.y * size;

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(xpos, ypos, 0.0f)) *
                              glm::scale(glm::mat4(1.0f), glm::vec3(w, h, 1.0f));

        float textureIndex = 0.0f;
        for (uint32_t i = 1; i < s_RendererData._2DTextureSlotIndex; i++) {
            if (s_RendererData._2DTextureSlots[i]->GetRendererID() == ch.TextureID) {
                textureIndex = (float)i;
                break;
            }
        }

        if (textureIndex == 0.0f) {
            if (s_RendererData._2DTextureSlotIndex >= RendererData::MaxTextureSlots) NextBatch();

            textureIndex = (float)s_RendererData._2DTextureSlotIndex;
            s_RendererData._2DTextureSlots[s_RendererData._2DTextureSlotIndex++] = Texture::WrapExisting(ch.TextureID);
        }

        for (int i = 0; i < 4; i++) {
            s_RendererData._2DQuadVertexBufferPtr->Position = transform * glm::vec4(s_RendererData.quadPositions[i], 1.0f);
            s_RendererData._2DQuadVertexBufferPtr->Color = color;
            s_RendererData._2DQuadVertexBufferPtr->TexCoord = s_RendererData.texCoords[i];
            s_RendererData._2DQuadVertexBufferPtr->TexIndex = textureIndex;
            s_RendererData._2DQuadVertexBufferPtr->TilingFactor = 1.0f;
            s_RendererData._2DQuadVertexBufferPtr->EntityID = entityID;
            s_RendererData._2DQuadVertexBufferPtr++;
        }

        s_RendererData._2DQuadIndexCount += 6;

        x += (ch.Advance >> 6) * size;
    }

    s_RendererData._2DStats.QuadCount += (int)text.length();
}

void Renderer::LoadFont(const std::string& path)
{
  Timer timer;

  FT_Face face;
  if (FT_New_Face(s_RendererData.ft, path.c_str(), 0, &face)) GABGL_ERROR("Failed to load font");
  else
  {
    FT_Set_Pixel_Sizes(face, 0, 48);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (unsigned char c = 0; c < 128; c++)
    {
      if (FT_Load_Char(face, c, FT_LOAD_RENDER)) { GABGL_ERROR("ERROR::FREETYPE: Failed to load Glyph"); continue; }

      GLuint texture;
      glGenTextures(1, &texture);
      glBindTexture(GL_TEXTURE_2D, texture);
      glTexImage2D(GL_TEXTURE_2D,0,GL_RED,face->glyph->bitmap.width,face->glyph->bitmap.rows,0,GL_RED,GL_UNSIGNED_BYTE,face->glyph->bitmap.buffer);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      GLint swizzleMask[] = {GL_ONE, GL_ONE, GL_ONE, GL_RED};
      glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

      Character character = {
          texture,
          { face->glyph->bitmap.width, face->glyph->bitmap.rows },
          { face->glyph->bitmap_left, face->glyph->bitmap_top },
          static_cast<uint32_t>(face->glyph->advance.x)
      };

      s_RendererData.Characters.insert({ c, character });
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    FT_Done_Face(face);
    FT_Done_FreeType(s_RendererData.ft);

    GABGL_WARN("Font uploading took {0} ms", timer.ElapsedMillis());
  }
}

void Renderer::OnWindowResize(uint32_t width, uint32_t height)
{
	SetViewport(0, 0, width, height);
}

void Renderer::Submit3D(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vertexArray, const glm::mat4& transform)
{
	shader->Use();
	shader->setMat4("u_ViewProjection", s_RendererData._3DCameraBuffer.ViewProjection);
	shader->setMat4("u_Transform", transform);

	vertexArray->Bind();
	DrawIndexed(vertexArray);
}

void Renderer::Submit2D(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vertexArray, const glm::mat4& transform)
{
	shader->Use();
	shader->setMat4("u_ViewProjection", s_RendererData._2DCameraBuffer.ViewProjection);
	shader->setMat4("u_Transform", transform);

	vertexArray->Bind();
	DrawIndexed(vertexArray);
}

void Renderer::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	glViewport(x, y, width, height);
}

void Renderer::SetClearColor(const glm::vec4& color)
{
	glClearColor(color.r, color.g, color.b, color.a);
}

void Renderer::Clear()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
	return s_RendererData.LineWidth;
}

void Renderer::ResetStats()
{
	memset(&s_RendererData._2DStats, 0, sizeof(Statistics));
	memset(&s_RendererData._3DStats, 0, sizeof(Statistics));
}

Renderer::Statistics Renderer::Get2DStats()
{
	return s_RendererData._2DStats;
}

Renderer::Statistics Renderer::Get3DStats()
{
	return s_RendererData._3DStats;
}

uint32_t Renderer::GetActiveWidgetID()
{
	return GImGui->ActiveId;
}

void Renderer::BlockEvents(bool block)
{ 
  s_RendererData.m_BlockEvents = block;
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
	s_RendererData.m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
	s_RendererData.m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

	s_RendererData.m_ViewportFocused = ImGui::IsWindowFocused();
	s_RendererData.m_ViewportHovered = ImGui::IsWindowHovered();

	BlockEvents(!s_RendererData.m_ViewportHovered);
	ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
	s_RendererData.m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

	uint64_t textureID = static_cast<uint64_t>(framebufferTexture);
	ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ s_RendererData.m_ViewportSize.x, s_RendererData.m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

	ImGui::End();
	ImGui::PopStyleVar();

	ImGui::End();
	io.DisplaySize = ImVec2((float)s_RendererData.m_WindowRef->GetWidth(), (float)s_RendererData.m_WindowRef->GetHeight());

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

