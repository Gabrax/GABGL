#include "Renderer2D.h"

#include "BackendLogger.h"
#include "Buffer.h"
#include <array>
#include "RendererAPI.h"
#include "glm/fwd.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H
#include "Shader.h"
#include "Texture.h"
#include "../engine.h"

struct QuadVertex
{
	glm::vec3 Position;
	glm::vec4 Color;
	glm::vec2 TexCoord;
	float TexIndex;
	float TilingFactor;

	// Editor-only
	int EntityID;
};

struct CircleVertex
{
	glm::vec3 WorldPosition;
	glm::vec3 LocalPosition;
	glm::vec4 Color;
	float Thickness;
	float Fade;

	// Editor-only
	int EntityID;
};

struct LineVertex
{
	glm::vec3 Position;
	glm::vec4 Color;

	// Editor-only
	int EntityID;
};

struct MeshVertex
{
  glm::vec3 Position;   
  glm::vec3 Normal;     
  glm::vec4 Color;

	// Editor-only
  int EntityID;        
};

struct CameraData
{
	glm::mat4 ViewProjection;
  glm::mat4 NonRotViewProjection;
} _3DCameraBuffer;
static std::shared_ptr<UniformBuffer> _3DCameraUniformBuffer;

static CameraData _2DCameraBuffer;
static std::shared_ptr<UniformBuffer> _2DCameraUniformBuffer;

struct Renderer2DData
{
	static const uint32_t MaxQuads = 20000;
	static const uint32_t MaxVertices = MaxQuads * 4;
	static const uint32_t MaxIndices = MaxQuads * 6;
	static const uint32_t MaxTextureSlots = 32; // TODO: RenderCaps

	std::shared_ptr<Texture> WhiteTexture;

	std::shared_ptr<VertexArray> _3DQuadVertexArray;
	std::shared_ptr<VertexBuffer> _3DQuadVertexBuffer;
	uint32_t _3DQuadIndexCount = 0;
	QuadVertex* _3DQuadVertexBufferBase = nullptr;
	QuadVertex* _3DQuadVertexBufferPtr = nullptr;
	glm::vec4 _3DQuadVertexPositions[4];
	std::array<std::shared_ptr<Texture>, MaxTextureSlots> _3DTextureSlots;
	uint32_t _3DTextureSlotIndex = 1; // 0 = white texture

	std::shared_ptr<VertexArray> _2DQuadVertexArray;
	std::shared_ptr<VertexBuffer> _2DQuadVertexBuffer;
	uint32_t _2DQuadIndexCount = 0;
	QuadVertex* _2DQuadVertexBufferBase = nullptr;
	QuadVertex* _2DQuadVertexBufferPtr = nullptr;
	std::array<std::shared_ptr<Texture>, MaxTextureSlots> _2DTextureSlots;
	uint32_t _2DTextureSlotIndex = 1; // 0 = white texture

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

	float LineWidth = 20.0f;

	Renderer::Statistics Stats;

	struct Shaders2D
	{
		std::shared_ptr<Shader> _3DQuadShader;
		std::shared_ptr<Shader> _2DQuadShader;
		std::shared_ptr<Shader> _CircleShader;
		std::shared_ptr<Shader> _LineShader;
		std::shared_ptr<Shader> _FramebufferShader;
	} _shaders2D;

  struct Character {
    uint32_t TextureID;
    glm::ivec2 Size;
    glm::ivec2 Bearing;
    uint32_t Advance;
  };

  std::unordered_map<char, Character> Characters;
  bool FontInitialized = false;

} s_2DData;

struct Renderer3DData
{
    static const uint32_t MaxVertices = 100000;
    static const uint32_t MaxIndices = MaxVertices * 6 / 4; // cube has 6 indices per 4 vertices
    static const uint32_t MaxTextureSlots = 32;

    std::shared_ptr<VertexArray> CubeVertexArray;
    std::shared_ptr<VertexBuffer> CubeVertexBuffer;
    std::shared_ptr<IndexBuffer> CubeIndexBuffer;
    MeshVertex* CubeVertexBufferBase = nullptr;
    MeshVertex* CubeVertexBufferPtr = nullptr;
    uint32_t CubeIndexCount = 0;
    uint32_t CubeVertexCount = 0;

    std::array<std::shared_ptr<Texture>, MaxTextureSlots> TextureSlots;
    uint32_t TextureSlotIndex = 1;

    struct Shaders3D
    {
        std::shared_ptr<Shader> modelShader;
        std::shared_ptr<Shader> skyboxShader;
    } _shaders3D;

    Renderer::Statistics Stats;


    std::array<std::string,6> skyboxfaces
    {
        "res/skybox/NightSky_Right.png",
        "res/skybox/NightSky_Left.png",
        "res/skybox/NightSky_Top.png",
        "res/skybox/NightSky_Bottom.png",
        "res/skybox/NightSky_Front.png",
        "res/skybox/NightSky_Back.png"
    };
    uint32_t skyboxTexture;

} s_3DData;

void Renderer::LoadShaders()
{
	s_2DData._shaders2D._3DQuadShader = Shader::Create("res/shaders/Renderer2D_Quad.glsl");
	s_2DData._shaders2D._2DQuadShader = Shader::Create("res/shaders/Renderer2D_2DQuad.glsl");
	s_2DData._shaders2D._CircleShader = Shader::Create("res/shaders/Renderer2D_Circle.glsl");
	s_2DData._shaders2D._LineShader = Shader::Create("res/shaders/Renderer2D_Line.glsl");
	s_2DData._shaders2D._FramebufferShader = Shader::Create("res/shaders/FB.glsl");
  s_3DData._shaders3D.modelShader = Shader::Create("res/shaders/Renderer3D_static.glsl");
  s_3DData._shaders3D.skyboxShader = Shader::Create("res/shaders/skybox.glsl");
}

void Renderer::Init()
{
	s_2DData._3DQuadVertexArray = VertexArray::Create();

	s_2DData._3DQuadVertexBuffer = VertexBuffer::Create(s_2DData.MaxVertices * sizeof(QuadVertex));
	s_2DData._3DQuadVertexBuffer->SetLayout({
		{ ShaderDataType::Float3, "a_Position"     },
		{ ShaderDataType::Float4, "a_Color"        },
		{ ShaderDataType::Float2, "a_TexCoord"     },
		{ ShaderDataType::Float,  "a_TexIndex"     },
		{ ShaderDataType::Float,  "a_TilingFactor" },
		{ ShaderDataType::Int,    "a_EntityID"     }
		});
	s_2DData._3DQuadVertexArray->AddVertexBuffer(s_2DData._3DQuadVertexBuffer);

	s_2DData._3DQuadVertexBufferBase = new QuadVertex[s_2DData.MaxVertices];

  s_2DData._2DQuadVertexArray = VertexArray::Create();

	s_2DData._2DQuadVertexBuffer = VertexBuffer::Create(s_2DData.MaxVertices * sizeof(QuadVertex));
	s_2DData._2DQuadVertexBuffer->SetLayout({
		{ ShaderDataType::Float3, "a_Position"     },
		{ ShaderDataType::Float4, "a_Color"        },
		{ ShaderDataType::Float2, "a_TexCoord"     },
		{ ShaderDataType::Float,  "a_TexIndex"     },
		{ ShaderDataType::Float,  "a_TilingFactor" },
		{ ShaderDataType::Int,    "a_EntityID"     }
		});
	s_2DData._2DQuadVertexArray->AddVertexBuffer(s_2DData._2DQuadVertexBuffer);

	s_2DData._2DQuadVertexBufferBase = new QuadVertex[s_2DData.MaxVertices];

	uint32_t* quadIndices = new uint32_t[s_2DData.MaxIndices];

	uint32_t offset = 0;
	for (uint32_t i = 0; i < s_2DData.MaxIndices; i += 6)
	{
		quadIndices[i + 0] = offset + 0;
		quadIndices[i + 1] = offset + 1;
		quadIndices[i + 2] = offset + 2;

		quadIndices[i + 3] = offset + 2;
		quadIndices[i + 4] = offset + 3;
		quadIndices[i + 5] = offset + 0;

		offset += 4;
	}

	std::shared_ptr<IndexBuffer> _3DquadIB = IndexBuffer::Create(quadIndices, s_2DData.MaxIndices);
	s_2DData._3DQuadVertexArray->SetIndexBuffer(_3DquadIB);
	std::shared_ptr<IndexBuffer> _2DquadIB = IndexBuffer::Create(quadIndices, s_2DData.MaxIndices);
	s_2DData._2DQuadVertexArray->SetIndexBuffer(_2DquadIB);
	delete[] quadIndices;

	// Circles
	s_2DData.CircleVertexArray = VertexArray::Create();

	s_2DData.CircleVertexBuffer = VertexBuffer::Create(s_2DData.MaxVertices * sizeof(CircleVertex));
	s_2DData.CircleVertexBuffer->SetLayout({
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
	s_2DData.CircleVertexArray->AddVertexBuffer(s_2DData.CircleVertexBuffer);
	s_2DData.CircleVertexArray->SetIndexBuffer(_3DquadIB); // Use quad IB
	s_2DData.CircleVertexBufferBase = new CircleVertex[s_2DData.MaxVertices];

	// Lines
	s_2DData.LineVertexArray = VertexArray::Create();

	s_2DData.LineVertexBuffer = VertexBuffer::Create(s_2DData.MaxVertices * sizeof(LineVertex));
	s_2DData.LineVertexBuffer->SetLayout({
		{ ShaderDataType::Float3, "a_Position" },
		{ ShaderDataType::Float4, "a_Color"    },
		{ ShaderDataType::Int,    "a_EntityID" }
		});
	s_2DData.LineVertexArray->AddVertexBuffer(s_2DData.LineVertexBuffer);
	s_2DData.LineVertexBufferBase = new LineVertex[s_2DData.MaxVertices];

	s_2DData.WhiteTexture = Texture::Create(TextureSpecification());
	uint32_t whiteTextureData = 0xffffffff;
	s_2DData.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

	int32_t samplers[s_2DData.MaxTextureSlots];
	for (uint32_t i = 0; i < s_2DData.MaxTextureSlots; i++)
		samplers[i] = i;

	s_2DData._3DTextureSlots[0] = s_2DData.WhiteTexture;
	s_2DData._2DTextureSlots[0] = s_2DData.WhiteTexture;

	s_2DData._3DQuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
	s_2DData._3DQuadVertexPositions[1] = { 0.5f, -0.5f, 0.0f, 1.0f };
	s_2DData._3DQuadVertexPositions[2] = { 0.5f,  0.5f, 0.0f, 1.0f };
	s_2DData._3DQuadVertexPositions[3] = { -0.5f,  0.5f, 0.0f, 1.0f };

  s_3DData.CubeVertexBufferBase = new MeshVertex[s_3DData.MaxVertices];

  s_3DData.CubeVertexBuffer = VertexBuffer::Create(nullptr, s_3DData.MaxVertices * sizeof(MeshVertex));
  s_3DData.CubeVertexBuffer->SetLayout({
      { ShaderDataType::Float3, "a_Position" },   // 0
      { ShaderDataType::Float3, "a_Normal" },     // 1
      { ShaderDataType::Float4, "a_Color" },      // 3
      { ShaderDataType::Int,    "a_EntityID" }    // 5
  });

  std::vector<uint32_t> indices;
  indices.reserve(s_3DData.MaxIndices);

  const uint32_t cubeIndicesCount = 36;

  uint32_t cubeIndices[36] = {
      0, 1, 2, 2, 3, 0,       // Front
      4, 5, 6, 6, 7, 4,       // Back
      4, 0, 3, 3, 7, 4,       // Left
      1, 5, 6, 6, 2, 1,       // Right
      3, 2, 6, 6, 7, 3,       // Top
      4, 5, 1, 1, 0, 4        // Bottom
  };

  uint32_t maxCubes = s_2DData.MaxVertices / 8; // 8 vertices per cube
  for (uint32_t i = 0; i < maxCubes; i++)
  {
      for (uint32_t j = 0; j < cubeIndicesCount; j++)
      {
          indices.push_back(cubeIndices[j] + i * 8);
      }
  }

  s_3DData.CubeIndexBuffer = IndexBuffer::Create(indices.data(), (uint32_t)indices.size());

  // Setup Vertex Array
  s_3DData.CubeVertexArray = VertexArray::Create();
  s_3DData.CubeVertexArray->AddVertexBuffer(s_3DData.CubeVertexBuffer);
  s_3DData.CubeVertexArray->SetIndexBuffer(s_3DData.CubeIndexBuffer);

  s_3DData.CubeIndexCount = 0;
  s_3DData.CubeVertexCount = 0;
  s_3DData.CubeVertexBufferPtr = s_3DData.CubeVertexBufferBase;

  s_3DData.skyboxTexture = Texture::loadCubemap(s_3DData.skyboxfaces);

  LoadShaders();

  _3DCameraUniformBuffer = UniformBuffer::Create(sizeof(CameraData), 0);
  _2DCameraUniformBuffer = UniformBuffer::Create(sizeof(CameraData), 1);

  FT_Library ft;
  GABGL_ASSERT(!FT_Init_FreeType(&ft), "Could not init FreeType Library");

  FT_Face face;
  if (FT_New_Face(ft, "res/fonts/dpcomic.ttf", 0, &face)) {
      GABGL_ERROR("Failed to load font");
  } else {
      FT_Set_Pixel_Sizes(face, 0, 48);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

      for (unsigned char c = 0; c < 128; c++) {
          if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
              std::cout << "ERROR::FREETYPE: Failed to load Glyph" << std::endl;
              continue;
          }

          GLuint texture;
          glGenTextures(1, &texture);
          glBindTexture(GL_TEXTURE_2D, texture);
          glTexImage2D(
              GL_TEXTURE_2D,
              0,
              GL_RED,
              face->glyph->bitmap.width,
              face->glyph->bitmap.rows,
              0,
              GL_RED,
              GL_UNSIGNED_BYTE,
              face->glyph->bitmap.buffer
          );

          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

          GLint swizzleMask[] = {GL_ONE, GL_ONE, GL_ONE, GL_RED};
          glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

          Renderer2DData::Character character = {
              texture,
              { face->glyph->bitmap.width, face->glyph->bitmap.rows },
              { face->glyph->bitmap_left, face->glyph->bitmap_top },
              static_cast<uint32_t>(face->glyph->advance.x)
          };

          s_2DData.Characters.insert({ c, character });
      }

      glBindTexture(GL_TEXTURE_2D, 0);
      FT_Done_Face(face);
      FT_Done_FreeType(ft);

      s_2DData.FontInitialized = true;
      GABGL_WARN("FONT LOADED");
  }
}

void Renderer::Shutdown()
{
	delete[] s_2DData._3DQuadVertexBufferBase;
	delete[] s_2DData._2DQuadVertexBufferBase;

  delete[] s_3DData.CubeVertexBufferBase;
  s_3DData.CubeVertexBufferBase = nullptr;

  s_3DData.CubeVertexArray.reset();
  s_3DData.CubeVertexBuffer.reset();
  s_3DData.CubeIndexBuffer.reset();
}

void Renderer::StartBatch()
{
	s_2DData._3DQuadIndexCount = 0;
	s_2DData._3DQuadVertexBufferPtr = s_2DData._3DQuadVertexBufferBase;
	s_2DData._3DTextureSlotIndex = 1;

	s_2DData._2DQuadIndexCount = 0;
	s_2DData._2DQuadVertexBufferPtr = s_2DData._2DQuadVertexBufferBase;
	s_2DData._2DTextureSlotIndex = 1;

	s_2DData.CircleIndexCount = 0;
	s_2DData.CircleVertexBufferPtr = s_2DData.CircleVertexBufferBase;

	s_2DData.LineVertexCount = 0;
	s_2DData.LineVertexBufferPtr = s_2DData.LineVertexBufferBase;

  s_3DData.CubeVertexCount = 0;
  s_3DData.CubeIndexCount = 0;
  s_3DData.CubeVertexBufferPtr = s_3DData.CubeVertexBufferBase;

  s_3DData.TextureSlotIndex = 1;
}

void Renderer::BeginScene(const Camera& camera, const glm::mat4& transform)
{
	_3DCameraBuffer.ViewProjection = camera.GetProjection() * glm::inverse(transform);
  _3DCameraBuffer.NonRotViewProjection = camera.GetNonRotationViewProjection();
	_3DCameraUniformBuffer->SetData(&_3DCameraBuffer, sizeof(CameraData));

	_2DCameraBuffer.ViewProjection = camera.GetOrtoProjection() * glm::inverse(transform);
  _2DCameraUniformBuffer->SetData(&_2DCameraBuffer, sizeof(CameraData));

	StartBatch();
}

void Renderer::BeginScene(const Camera& camera)
{
	_3DCameraBuffer.ViewProjection = camera.GetViewProjection();
  _3DCameraBuffer.NonRotViewProjection = camera.GetNonRotationViewProjection();
	_3DCameraUniformBuffer->SetData(&_3DCameraBuffer, sizeof(CameraData));

	_2DCameraBuffer.ViewProjection = camera.GetOrtoProjection();
  _2DCameraUniformBuffer->SetData(&_2DCameraBuffer, sizeof(CameraData));

	StartBatch();
}

void Renderer::Flush()
{
	if (s_2DData._3DQuadIndexCount)
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_2DData._3DQuadVertexBufferPtr - (uint8_t*)s_2DData._3DQuadVertexBufferBase);
		s_2DData._3DQuadVertexBuffer->SetData(s_2DData._3DQuadVertexBufferBase, dataSize);

		// Bind textures
		for (uint32_t i = 0; i < s_2DData._3DTextureSlotIndex; i++)
			s_2DData._3DTextureSlots[i]->Bind(i);

		s_2DData._shaders2D._3DQuadShader->Use();
		RendererAPI::DrawIndexed(s_2DData._3DQuadVertexArray, s_2DData._3DQuadIndexCount);
		s_2DData.Stats.DrawCalls++;
	}
	if (s_2DData._2DQuadIndexCount)
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_2DData._2DQuadVertexBufferPtr - (uint8_t*)s_2DData._2DQuadVertexBufferBase);
		s_2DData._2DQuadVertexBuffer->SetData(s_2DData._2DQuadVertexBufferBase, dataSize);

		// Bind textures
		for (uint32_t i = 0; i < s_2DData._2DTextureSlotIndex; i++)
			s_2DData._2DTextureSlots[i]->Bind(i);

		s_2DData._shaders2D._2DQuadShader->Use();
		RendererAPI::DrawIndexed(s_2DData._2DQuadVertexArray, s_2DData._2DQuadIndexCount);
		s_2DData.Stats.DrawCalls++;
	}
	if (s_2DData.CircleIndexCount)
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_2DData.CircleVertexBufferPtr - (uint8_t*)s_2DData.CircleVertexBufferBase);
		s_2DData.CircleVertexBuffer->SetData(s_2DData.CircleVertexBufferBase, dataSize);

		s_2DData._shaders2D._CircleShader->Use();
		RendererAPI::DrawIndexed(s_2DData.CircleVertexArray, s_2DData.CircleIndexCount);
		s_2DData.Stats.DrawCalls++;
	}
	if (s_2DData.LineVertexCount)
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_2DData.LineVertexBufferPtr - (uint8_t*)s_2DData.LineVertexBufferBase);
		s_2DData.LineVertexBuffer->SetData(s_2DData.LineVertexBufferBase, dataSize);

		s_2DData._shaders2D._LineShader->Use();
		RendererAPI::SetLineWidth(s_2DData.LineWidth);
		RendererAPI::DrawLines(s_2DData.LineVertexArray, s_2DData.LineVertexCount);
		s_2DData.Stats.DrawCalls++;
	}
  if (s_3DData.CubeIndexCount) 
  {
    uint32_t dataSize = (uint32_t)((uint8_t*)s_3DData.CubeVertexBufferPtr - (uint8_t*)s_3DData.CubeVertexBufferBase);
    s_3DData.CubeVertexBuffer->SetData(s_3DData.CubeVertexBufferBase, dataSize);

    s_3DData._shaders3D.modelShader->Use();
    s_3DData.CubeVertexArray->Bind();

    RendererAPI::DrawIndexed(s_3DData.CubeVertexArray, s_3DData.CubeIndexCount);

    s_3DData.Stats.DrawCalls++;
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

void Renderer::Draw3DQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
{
	Draw3DQuad({ position.x, position.y, 0.0f }, size, color);
}

void Renderer::Draw3DQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
{
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
		* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

	Draw3DQuad(transform, color);
}

void Renderer::Draw3DQuad(const glm::vec2& position, const glm::vec2& size, const std::shared_ptr<Texture>& texture, float tilingFactor, const glm::vec4& tintColor)
{
	Draw3DQuad({ position.x, position.y, 0.0f }, size, texture, tilingFactor, tintColor);
}

void Renderer::Draw3DQuad(const glm::vec3& position, const glm::vec2& size, const std::shared_ptr<Texture>& texture, float tilingFactor, const glm::vec4& tintColor)
{
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
		* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

	Draw3DQuad(transform, texture, tilingFactor, tintColor);
}

void Renderer::Draw3DQuad(const glm::mat4& transform, const glm::vec4& color, int entityID)
{
	constexpr size_t quadVertexCount = 4;
	const float textureIndex = 0.0f; // White Texture
	constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
	const float tilingFactor = 1.0f;

	if (s_2DData._3DQuadIndexCount >= Renderer2DData::MaxIndices)
		NextBatch();

	for (size_t i = 0; i < quadVertexCount; i++)
	{
		s_2DData._3DQuadVertexBufferPtr->Position = transform * s_2DData._3DQuadVertexPositions[i];
		s_2DData._3DQuadVertexBufferPtr->Color = color;
		s_2DData._3DQuadVertexBufferPtr->TexCoord = textureCoords[i];
		s_2DData._3DQuadVertexBufferPtr->TexIndex = textureIndex;
		s_2DData._3DQuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_2DData._3DQuadVertexBufferPtr->EntityID = entityID;
		s_2DData._3DQuadVertexBufferPtr++;
	}

	s_2DData._3DQuadIndexCount += 6;

	s_2DData.Stats.QuadCount++;
}

void Renderer::Draw3DQuad(const glm::mat4& transform, const std::shared_ptr<Texture>& texture, float tilingFactor, const glm::vec4& tintColor, int entityID)
{
	constexpr size_t quadVertexCount = 4;
	constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

	if (s_2DData._3DQuadIndexCount >= Renderer2DData::MaxIndices)
		NextBatch();

	float textureIndex = 0.0f;
	for (uint32_t i = 1; i < s_2DData._3DTextureSlotIndex; i++)
	{
		if (*s_2DData._3DTextureSlots[i] == *texture)
		{
			textureIndex = (float)i;
			break;
		}
	}

	if (textureIndex == 0.0f)
	{
		if (s_2DData._3DTextureSlotIndex >= Renderer2DData::MaxTextureSlots)
			NextBatch();

		textureIndex = (float)s_2DData._3DTextureSlotIndex;
		s_2DData._3DTextureSlots[s_2DData._3DTextureSlotIndex] = texture;
		s_2DData._3DTextureSlotIndex++;
	}

	for (size_t i = 0; i < quadVertexCount; i++)
	{
		s_2DData._3DQuadVertexBufferPtr->Position = transform * s_2DData._3DQuadVertexPositions[i];
		s_2DData._3DQuadVertexBufferPtr->Color = tintColor;
		s_2DData._3DQuadVertexBufferPtr->TexCoord = textureCoords[i];
		s_2DData._3DQuadVertexBufferPtr->TexIndex = textureIndex;
		s_2DData._3DQuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_2DData._3DQuadVertexBufferPtr->EntityID = entityID;
		s_2DData._3DQuadVertexBufferPtr++;
	}

	s_2DData._3DQuadIndexCount += 6;

	s_2DData.Stats.QuadCount++;
}

void Renderer::DrawRotated3DQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
{
	DrawRotated3DQuad({ position.x, position.y, 0.0f }, size, rotation, color);
}

void Renderer::DrawRotated3DQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
{
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
		* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
		* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

	Draw3DQuad(transform, color);
}

void Renderer::DrawRotated3DQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const std::shared_ptr<Texture>& texture, float tilingFactor, const glm::vec4& tintColor)
{
	DrawRotated3DQuad({ position.x, position.y, 0.0f }, size, rotation, texture, tilingFactor, tintColor);
}

void Renderer::DrawRotated3DQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const std::shared_ptr<Texture>& texture, float tilingFactor, const glm::vec4& tintColor)
{
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
		* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
		* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

	Draw3DQuad(transform, texture, tilingFactor, tintColor);
}

void Renderer::Draw3DCircle(const glm::mat4& transform, const glm::vec4& color, float thickness /*= 1.0f*/, float fade /*= 0.005f*/, int entityID /*= -1*/)
{
	// TODO: implement for circles
	// if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
	// 	NextBatch();

	for (size_t i = 0; i < 4; i++)
	{
		s_2DData.CircleVertexBufferPtr->WorldPosition = transform * s_2DData._3DQuadVertexPositions[i];
		s_2DData.CircleVertexBufferPtr->LocalPosition = s_2DData._3DQuadVertexPositions[i] * 2.0f;
		s_2DData.CircleVertexBufferPtr->Color = color;
		s_2DData.CircleVertexBufferPtr->Thickness = thickness;
		s_2DData.CircleVertexBufferPtr->Fade = fade;
		s_2DData.CircleVertexBufferPtr->EntityID = entityID;
		s_2DData.CircleVertexBufferPtr++;
	}

	s_2DData.CircleIndexCount += 6;

	s_2DData.Stats.QuadCount++;
}

void Renderer::DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, int entityID)
{
	s_2DData.LineVertexBufferPtr->Position = p0;
	s_2DData.LineVertexBufferPtr->Color = color;
	s_2DData.LineVertexBufferPtr->EntityID = entityID;
	s_2DData.LineVertexBufferPtr++;

	s_2DData.LineVertexBufferPtr->Position = p1;
	s_2DData.LineVertexBufferPtr->Color = color;
	s_2DData.LineVertexBufferPtr->EntityID = entityID;
	s_2DData.LineVertexBufferPtr++;

	s_2DData.LineVertexCount += 2;
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
	for (size_t i = 0; i < 4; i++)
		lineVertices[i] = transform * s_2DData._3DQuadVertexPositions[i];

	DrawLine(lineVertices[0], lineVertices[1], color, entityID);
	DrawLine(lineVertices[1], lineVertices[2], color, entityID);
	DrawLine(lineVertices[2], lineVertices[3], color, entityID);
	DrawLine(lineVertices[3], lineVertices[0], color, entityID);
}

void Renderer::Draw2DQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
{
	Draw2DQuad({ position.x, position.y, 0.0f }, size, color);
}

void Renderer::Draw2DQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
{
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
		* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

	Draw2DQuad(transform, color);
}

void Renderer::Draw2DQuad(const glm::vec2& position, const glm::vec2& size, const std::shared_ptr<Texture>& texture, float tilingFactor, const glm::vec4& tintColor)
{
	Draw2DQuad({ position.x, position.y, 0.0f }, size, texture, tilingFactor, tintColor);
}

void Renderer::Draw2DQuad(const glm::vec3& position, const glm::vec2& size, const std::shared_ptr<Texture>& texture, float tilingFactor, const glm::vec4& tintColor)
{
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
		* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

	Draw2DQuad(transform, texture, tilingFactor, tintColor);
}

void Renderer::Draw2DQuad(const glm::mat4& transform, const glm::vec4& color, int entityID)
{
	constexpr size_t quadVertexCount = 4;
	const float textureIndex = 0.0f; // White Texture
	constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
	const float tilingFactor = 1.0f;

	if (s_2DData._2DQuadIndexCount >= Renderer2DData::MaxIndices)
		NextBatch();

	for (size_t i = 0; i < quadVertexCount; i++)
	{
		s_2DData._2DQuadVertexBufferPtr->Position = transform * s_2DData._3DQuadVertexPositions[i];
		s_2DData._2DQuadVertexBufferPtr->Color = color;
		s_2DData._2DQuadVertexBufferPtr->TexCoord = textureCoords[i];
		s_2DData._2DQuadVertexBufferPtr->TexIndex = textureIndex;
		s_2DData._2DQuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_2DData._2DQuadVertexBufferPtr->EntityID = entityID;
		s_2DData._2DQuadVertexBufferPtr++;
	}

	s_2DData._2DQuadIndexCount += 6;

	s_2DData.Stats.QuadCount++;
}

void Renderer::Draw2DQuad(const glm::mat4& transform, const std::shared_ptr<Texture>& texture, float tilingFactor, const glm::vec4& tintColor, int entityID)
{
	constexpr size_t quadVertexCount = 4;
	constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

	if (s_2DData._2DQuadIndexCount >= Renderer2DData::MaxIndices)
		NextBatch();

	float textureIndex = 0.0f;
	for (uint32_t i = 1; i < s_2DData._2DTextureSlotIndex; i++)
	{
		if (*s_2DData._2DTextureSlots[i] == *texture)
		{
			textureIndex = (float)i;
			break;
		}
	}

	if (textureIndex == 0.0f)
	{
		if (s_2DData._2DTextureSlotIndex >= Renderer2DData::MaxTextureSlots)
			NextBatch();

		textureIndex = (float)s_2DData._2DTextureSlotIndex;
		s_2DData._2DTextureSlots[s_2DData._2DTextureSlotIndex] = texture;
		s_2DData._2DTextureSlotIndex++;
	}

	for (size_t i = 0; i < quadVertexCount; i++)
	{
		s_2DData._2DQuadVertexBufferPtr->Position = transform * s_2DData._3DQuadVertexPositions[i];
		s_2DData._2DQuadVertexBufferPtr->Color = tintColor;
		s_2DData._2DQuadVertexBufferPtr->TexCoord = textureCoords[i];
		s_2DData._2DQuadVertexBufferPtr->TexIndex = textureIndex;
		s_2DData._2DQuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_2DData._2DQuadVertexBufferPtr->EntityID = entityID;
		s_2DData._2DQuadVertexBufferPtr++;
	}

	s_2DData._2DQuadIndexCount += 6;

	s_2DData.Stats.QuadCount++;
}

void Renderer::DrawRotated2DQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
{
	DrawRotated2DQuad({ position.x, position.y, 0.0f }, size, rotation, color);
}

void Renderer::DrawRotated2DQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
{
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
		* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
		* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

	Draw2DQuad(transform, color);
}

void Renderer::DrawRotated2DQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const std::shared_ptr<Texture>& texture, float tilingFactor, const glm::vec4& tintColor)
{
	DrawRotated2DQuad({ position.x, position.y, 0.0f }, size, rotation, texture, tilingFactor, tintColor);
}

void Renderer::DrawRotated2DQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const std::shared_ptr<Texture>& texture, float tilingFactor, const glm::vec4& tintColor)
{
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
		* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
		* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

	Draw2DQuad(transform, texture, tilingFactor, tintColor);
}

void Renderer::Draw2DCircle(const glm::mat4& transform, const glm::vec4& color, float thickness /*= 1.0f*/, float fade /*= 0.005f*/, int entityID /*= -1*/)
{
	// TODO: implement for circles
	// if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
	// 	NextBatch();

	for (size_t i = 0; i < 4; i++)
	{
		s_2DData.CircleVertexBufferPtr->WorldPosition = transform * s_2DData._3DQuadVertexPositions[i];
		s_2DData.CircleVertexBufferPtr->LocalPosition = s_2DData._3DQuadVertexPositions[i] * 2.0f;
		s_2DData.CircleVertexBufferPtr->Color = color;
		s_2DData.CircleVertexBufferPtr->Thickness = thickness;
		s_2DData.CircleVertexBufferPtr->Fade = fade;
		s_2DData.CircleVertexBufferPtr->EntityID = entityID;
		s_2DData.CircleVertexBufferPtr++;
	}

	s_2DData.CircleIndexCount += 6;

	s_2DData.Stats.QuadCount++;
}

/*void Renderer::DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, int entityID)*/
/*{*/
/*	s_2DData.LineVertexBufferPtr->Position = p0;*/
/*	s_2DData.LineVertexBufferPtr->Color = color;*/
/*	s_2DData.LineVertexBufferPtr->EntityID = entityID;*/
/*	s_2DData.LineVertexBufferPtr++;*/
/**/
/*	s_2DData.LineVertexBufferPtr->Position = p1;*/
/*	s_2DData.LineVertexBufferPtr->Color = color;*/
/*	s_2DData.LineVertexBufferPtr->EntityID = entityID;*/
/*	s_2DData.LineVertexBufferPtr++;*/
/**/
/*	s_2DData.LineVertexCount += 2;*/
/*}*/

void Renderer::Draw2DRect(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, int entityID)
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

void Renderer::Draw2DRect(const glm::mat4& transform, const glm::vec4& color, int entityID)
{
	glm::vec3 lineVertices[4];
	for (size_t i = 0; i < 4; i++)
		lineVertices[i] = transform * s_2DData._3DQuadVertexPositions[i];

	DrawLine(lineVertices[0], lineVertices[1], color, entityID);
	DrawLine(lineVertices[1], lineVertices[2], color, entityID);
	DrawLine(lineVertices[2], lineVertices[3], color, entityID);
	DrawLine(lineVertices[3], lineVertices[0], color, entityID);
}


// Cube vertices in object space (static, reused for all cubes)
static MeshVertex cubeVertices[8] = {
    {{-0.5f, -0.5f,  0.5f}, {0, 0, 1}, {1.0f, 1.0f, 1.0f, 1.0f}, 0},
    {{ 0.5f, -0.5f,  0.5f}, {0, 0, 1}, {1.0f, 1.0f, 1.0f, 1.0f}, 0},
    {{ 0.5f,  0.5f,  0.5f}, {0, 0, 1}, {1.0f, 1.0f, 1.0f, 1.0f}, 0},
    {{-0.5f,  0.5f,  0.5f}, {0, 0, 1}, {1.0f, 1.0f, 1.0f, 1.0f}, 0},

    {{-0.5f, -0.5f, -0.5f}, {0, 0, -1}, {1.0f, 1.0f, 1.0f, 1.0f}, 0},
    {{ 0.5f, -0.5f, -0.5f}, {0, 0, -1}, {1.0f, 1.0f, 1.0f, 1.0f}, 0},
    {{ 0.5f,  0.5f, -0.5f}, {0, 0, -1}, {1.0f, 1.0f, 1.0f, 1.0f}, 0},
    {{-0.5f,  0.5f, -0.5f}, {0, 0, -1}, {1.0f, 1.0f, 1.0f, 1.0f}, 0},
};

// Call this to batch a cube instance
void Renderer::DrawCube(const TransformComponent& transform, int entityID)
{
    if (s_3DData.CubeIndexCount >= Renderer3DData::MaxIndices)
        NextBatch(); // Flush batch if full

    glm::mat4 model = transform.GetTransform();
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

    for (int i = 0; i < 8; i++)
    {
        MeshVertex vertex = cubeVertices[i];

        // Transform position
        glm::vec4 transformedPos = model * glm::vec4(vertex.Position, 1.0f);
        vertex.Position = glm::vec3(transformedPos);
        vertex.Normal = glm::normalize(normalMatrix * vertex.Normal);
        vertex.Color = glm::vec4(2.0f);
        vertex.EntityID = entityID;

        *s_3DData.CubeVertexBufferPtr = vertex;
        s_3DData.CubeVertexBufferPtr++;
    }

    s_3DData.CubeVertexCount += 8;
    s_3DData.CubeIndexCount += 36; // 6 faces * 2 triangles * 3 indices = 36 indices per cube
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
    s_2DData._shaders2D._FramebufferShader->Use();
    s_2DData._shaders2D._FramebufferShader->setInt("u_Texture", 0);

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

void Renderer::DrawSkybox()
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

  glDepthFunc(GL_LEQUAL); // Important for correct skybox depth test

  s_3DData._shaders3D.skyboxShader->Use();
  s_3DData._shaders3D.skyboxShader->setMat4("u_ViewProjection", _3DCameraBuffer.NonRotViewProjection);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, s_3DData.skyboxTexture);
  glBindVertexArray(SkyboxVAO);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);

  glDepthFunc(GL_LESS); // Reset depth test
}

void Renderer::Draw3DText(const std::string& text, const glm::vec2& position, float size, const glm::vec4& color, int entityID)
{
    if (!s_2DData.FontInitialized)
        return;

    // --- Precompute total width and height for centering ---
    float textWidth = 0.0f;
    float maxBearingY = 0.0f;
    float maxBelowBaseline = 0.0f;

    for (char c : text)
    {
        auto it = s_2DData.Characters.find(c);
        if (it == s_2DData.Characters.end())
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
        auto it = s_2DData.Characters.find(c);
        if (it == s_2DData.Characters.end())
            continue;

        Renderer2DData::Character& ch = it->second;

        if (s_2DData._3DQuadIndexCount >= Renderer2DData::MaxIndices)
            NextBatch();

        float xpos = x + ch.Bearing.x * size;
        float ypos = y + (maxBearingY - ch.Bearing.y * size);  // Adjust Y using max bearing

        float w = ch.Size.x * size;
        float h = ch.Size.y * size;

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(xpos, ypos, 0.0f)) *
                              glm::scale(glm::mat4(1.0f), glm::vec3(w, h, 1.0f));

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

        float textureIndex = 0.0f;
        for (uint32_t i = 1; i < s_2DData._3DTextureSlotIndex; i++) {
            if (s_2DData._3DTextureSlots[i]->GetRendererID() == ch.TextureID) {
                textureIndex = (float)i;
                break;
            }
        }

        if (textureIndex == 0.0f) {
            if (s_2DData._3DTextureSlotIndex >= Renderer2DData::MaxTextureSlots)
                NextBatch();

            textureIndex = (float)s_2DData._3DTextureSlotIndex;
            s_2DData._3DTextureSlots[s_2DData._3DTextureSlotIndex++] = Texture::WrapExisting(ch.TextureID);
        }

        for (int i = 0; i < 4; i++) {
            s_2DData._3DQuadVertexBufferPtr->Position = transform * glm::vec4(quadPositions[i], 1.0f);
            s_2DData._3DQuadVertexBufferPtr->Color = color;
            s_2DData._3DQuadVertexBufferPtr->TexCoord = texCoords[i];
            s_2DData._3DQuadVertexBufferPtr->TexIndex = textureIndex;
            s_2DData._3DQuadVertexBufferPtr->TilingFactor = 1.0f;
            s_2DData._3DQuadVertexBufferPtr->EntityID = entityID;
            s_2DData._3DQuadVertexBufferPtr++;
        }

        s_2DData._3DQuadIndexCount += 6;

        x += (ch.Advance >> 6) * size;
    }

    s_2DData.Stats.QuadCount += (int)text.length();
}

void Renderer::Draw2DText(const std::string& text, const glm::vec2& position, float size, const glm::vec4& color, int entityID)
{
    if (!s_2DData.FontInitialized)
        return;

    // --- Precompute total width and height for centering ---
    float textWidth = 0.0f;
    float maxBearingY = 0.0f;
    float maxBelowBaseline = 0.0f;

    for (char c : text)
    {
        auto it = s_2DData.Characters.find(c);
        if (it == s_2DData.Characters.end())
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
        auto it = s_2DData.Characters.find(c);
        if (it == s_2DData.Characters.end())
            continue;

        Renderer2DData::Character& ch = it->second;

        if (s_2DData._2DQuadIndexCount >= Renderer2DData::MaxIndices)
            NextBatch();

        float xpos = x + ch.Bearing.x * size;
        float ypos = y + (maxBearingY - ch.Bearing.y * size);  // Adjust Y using max bearing

        float w = ch.Size.x * size;
        float h = ch.Size.y * size;

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(xpos, ypos, 0.0f)) *
                              glm::scale(glm::mat4(1.0f), glm::vec3(w, h, 1.0f));

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

        float textureIndex = 0.0f;
        for (uint32_t i = 1; i < s_2DData._2DTextureSlotIndex; i++) {
            if (s_2DData._2DTextureSlots[i]->GetRendererID() == ch.TextureID) {
                textureIndex = (float)i;
                break;
            }
        }

        if (textureIndex == 0.0f) {
            if (s_2DData._2DTextureSlotIndex >= Renderer2DData::MaxTextureSlots)
                NextBatch();

            textureIndex = (float)s_2DData._2DTextureSlotIndex;
            s_2DData._2DTextureSlots[s_2DData._2DTextureSlotIndex++] = Texture::WrapExisting(ch.TextureID);
        }

        for (int i = 0; i < 4; i++) {
            s_2DData._2DQuadVertexBufferPtr->Position = transform * glm::vec4(quadPositions[i], 1.0f);
            s_2DData._2DQuadVertexBufferPtr->Color = color;
            s_2DData._2DQuadVertexBufferPtr->TexCoord = texCoords[i];
            s_2DData._2DQuadVertexBufferPtr->TexIndex = textureIndex;
            s_2DData._2DQuadVertexBufferPtr->TilingFactor = 1.0f;
            s_2DData._2DQuadVertexBufferPtr->EntityID = entityID;
            s_2DData._2DQuadVertexBufferPtr++;
        }

        s_2DData._2DQuadIndexCount += 6;

        x += (ch.Advance >> 6) * size;
    }

    s_2DData.Stats.QuadCount += (int)text.length();
}


float Renderer::GetLineWidth()
{
	return s_2DData.LineWidth;
}

void Renderer::SetLineWidth(float width)
{
	s_2DData.LineWidth = width;
}

void Renderer::ResetStats()
{
	memset(&s_2DData.Stats, 0, sizeof(Statistics));
	memset(&s_3DData.Stats, 0, sizeof(Statistics));
}

Renderer::Statistics Renderer::GetStats()
{
	return s_2DData.Stats;
}

