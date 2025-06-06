#include "Renderer.h"

#include <array>

#include "BackendLogger.h"
#include "Buffer.h"
#include "Renderer.h"
#include "Shader.h"
#include "Texture.h"
#include "glm/fwd.hpp"
#include <cstdint>
#include <stb_image.h>
#include <ft2build.h>
#include FT_FREETYPE_H

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
	static constexpr uint32_t MaxQuads = 20000;
	static constexpr uint32_t MaxVertices = MaxQuads * 4;
	static constexpr uint32_t MaxIndices = MaxQuads * 6;
	static constexpr uint32_t MaxTextureSlots = 32; // TODO: RenderCaps

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

	s_RendererData.WhiteTexture = Texture::CreateGL(TextureSpecification());
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
  s_RendererData.CubeIndexCount = 0;
  s_RendererData.CubeVertexCount = 0;
  s_RendererData.CubeVertexBufferPtr = s_RendererData.CubeVertexBufferBase;

  s_RendererData._3DCameraUniformBuffer = UniformBuffer::Create(sizeof(CameraData), 0);
  s_RendererData._2DCameraUniformBuffer = UniformBuffer::Create(sizeof(CameraData), 1);
  s_RendererData.LightPosStorageBuffer = StorageBuffer::Create(sizeof(glm::vec3) * 10, 0);
  s_RendererData.LightQuantityStorageBuffer= StorageBuffer::Create(sizeof(uint32_t) * 10, 1);
  s_RendererData.LightColorStorageBuffer= StorageBuffer::Create(sizeof(glm::vec4) * 10, 2);
  s_RendererData.LightTypeStorageBuffer= StorageBuffer::Create(sizeof(uint32_t) * 10, 3);

  GABGL_ASSERT(!FT_Init_FreeType(&s_RendererData.ft), "Could not init FreeType Library");

  LoadShaders();

  LoadFont("res/fonts/dpcomic.ttf");
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

		// Bind textures
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

		// Bind textures
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

void Renderer::DrawCube(const TransformComponent& transform, int entityID)
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

void Renderer::RenderFullscreenFramebufferTexture(uint32_t textureID)
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

  glDisable(GL_DEPTH_TEST);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textureID);

  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Renderer::UploadModel(const std::string& name, const std::shared_ptr<Model>& model)
{
  for(auto& mesh : model->GetMeshes())
  {
    auto VAO = mesh.VAO;
    auto VBO = mesh.VBO;
    auto EBO = mesh.EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.m_Vertices.size() * sizeof(Vertex), &mesh.m_Vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
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

    for (auto& texture : mesh.m_Textures)
    {
      GLuint id = 0;
      glGenTextures(1, &id);
      texture->SetRendererID(id);

      glBindTexture(GL_TEXTURE_2D, id);
      glTexImage2D(GL_TEXTURE_2D, 0, texture->GetDataFormat(), texture->GetWidth(), texture->GetHeight(), 0, texture->GetDataFormat(), GL_UNSIGNED_BYTE, texture->GetRawData());

      glGenerateMipmap(GL_TEXTURE_2D);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
  }

  s_RendererData.models[name] = std::move(model);
}

void Renderer::DrawModel(const std::string& name)
{
  auto it = s_RendererData.models.find(name);
  if (it != s_RendererData.models.end())
  {
    for(auto& mesh : it->second->GetMeshes())
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
          /*shader.setInt(textures[i].type + number, i);*/
          glBindTexture(GL_TEXTURE_2D, textures[i]->GetRendererID());
      }

      glBindVertexArray(mesh.VAO);
      glDrawElements(GL_TRIANGLES, static_cast<GLuint>(mesh.m_Indices.size()), GL_UNSIGNED_INT, 0);
      glBindVertexArray(0);
      glActiveTexture(GL_TEXTURE0);
    }
  }
  else
  {
      GABGL_ERROR("Model not found: " + name);
      return;
  }

}

void Renderer::UploadSkybox(const std::string& name, const std::shared_ptr<Texture>& cubemap)
{
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

    GABGL_WARN("FONT LOADED");
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

