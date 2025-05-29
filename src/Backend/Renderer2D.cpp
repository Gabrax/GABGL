#include "Renderer2D.h"

#include "BackendLogger.h"
#include "Buffer.h"
#include <array>
#include "RendererAPI.h"
#include "glm/fwd.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H  

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

struct Renderer2DData
{
	static const uint32_t MaxQuads = 20000;
	static const uint32_t MaxVertices = MaxQuads * 4;
	static const uint32_t MaxIndices = MaxQuads * 6;
	static const uint32_t MaxTextureSlots = 32; // TODO: RenderCaps

	std::shared_ptr<VertexArray> QuadVertexArray;
	std::shared_ptr<VertexBuffer> QuadVertexBuffer;
	std::shared_ptr<Texture> WhiteTexture;

	std::shared_ptr<VertexArray> CircleVertexArray;
	std::shared_ptr<VertexBuffer> CircleVertexBuffer;

	std::shared_ptr<VertexArray> LineVertexArray;
	std::shared_ptr<VertexBuffer> LineVertexBuffer;

	struct Shaders2D
	{
		std::shared_ptr<Shader> QuadShader;
		std::shared_ptr<Shader> CircleShader;
		std::shared_ptr<Shader> LineShader;
		std::shared_ptr<Shader> FramebufferShader;
	} _shaders2D;

	uint32_t QuadIndexCount = 0;
	QuadVertex* QuadVertexBufferBase = nullptr;
	QuadVertex* QuadVertexBufferPtr = nullptr;

	uint32_t CircleIndexCount = 0;
	CircleVertex* CircleVertexBufferBase = nullptr;
	CircleVertex* CircleVertexBufferPtr = nullptr;

	uint32_t LineVertexCount = 0;
	LineVertex* LineVertexBufferBase = nullptr;
	LineVertex* LineVertexBufferPtr = nullptr;

	float LineWidth = 20.0f;

	std::array<std::shared_ptr<Texture>, MaxTextureSlots> TextureSlots;
	uint32_t TextureSlotIndex = 1; // 0 = white texture

	std::shared_ptr<Texture> FontAtlasTexture;

	glm::vec4 QuadVertexPositions[4];

	Renderer2D::Statistics Stats;

	struct CameraData
	{
		glm::mat4 ViewProjection;
	};
	CameraData CameraBuffer;
	std::shared_ptr<UniformBuffer> CameraUniformBuffer;

  struct Character {
    uint32_t TextureID;
    glm::ivec2 Size;
    glm::ivec2 Bearing;
    uint32_t Advance;
  };

  std::unordered_map<char, Character> Characters;
  bool FontInitialized = false;

} s_Data;

void Renderer2D::LoadShaders()
{
	s_Data._shaders2D.QuadShader = Shader::Create("res/shaders/Renderer2D_Quad.glsl");
	s_Data._shaders2D.CircleShader = Shader::Create("res/shaders/Renderer2D_Circle.glsl");
	s_Data._shaders2D.LineShader = Shader::Create("res/shaders/Renderer2D_Line.glsl");
	s_Data._shaders2D.FramebufferShader = Shader::Create("res/shaders/FB.glsl");
}

void Renderer2D::Init()
{
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
	s_Data.CircleVertexArray->SetIndexBuffer(quadIB); // Use quad IB
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

	LoadShaders();

	// Set first texture slot to 0
	s_Data.TextureSlots[0] = s_Data.WhiteTexture;

	s_Data.QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
	s_Data.QuadVertexPositions[1] = { 0.5f, -0.5f, 0.0f, 1.0f };
	s_Data.QuadVertexPositions[2] = { 0.5f,  0.5f, 0.0f, 1.0f };
	s_Data.QuadVertexPositions[3] = { -0.5f,  0.5f, 0.0f, 1.0f };

	s_Data.CameraUniformBuffer = UniformBuffer::Create(sizeof(Renderer2DData::CameraData), 0);

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

          s_Data.Characters.insert({ c, character });
      }

      glBindTexture(GL_TEXTURE_2D, 0);
      FT_Done_Face(face);
      FT_Done_FreeType(ft);

      s_Data.FontInitialized = true;
      GABGL_WARN("FONT LOADED");
  }
}

void Renderer2D::Shutdown()
{
	delete[] s_Data.QuadVertexBufferBase;
}

void Renderer2D::StartBatch()
{
	s_Data.QuadIndexCount = 0;
	s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

	s_Data.CircleIndexCount = 0;
	s_Data.CircleVertexBufferPtr = s_Data.CircleVertexBufferBase;

	s_Data.LineVertexCount = 0;
	s_Data.LineVertexBufferPtr = s_Data.LineVertexBufferBase;

	s_Data.TextureSlotIndex = 1;
}

void Renderer2D::BeginScene(const Camera& camera, const glm::mat4& transform)
{
	s_Data.CameraBuffer.ViewProjection = camera.GetProjection() * glm::inverse(transform);
	s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer2DData::CameraData));

	StartBatch();
}

void Renderer2D::BeginScene(const Camera& camera)
{
	s_Data.CameraBuffer.ViewProjection = camera.GetViewProjection();
	s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer2DData::CameraData));

	StartBatch();
}

void Renderer2D::Flush()
{
	if (s_Data.QuadIndexCount)
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.QuadVertexBufferPtr - (uint8_t*)s_Data.QuadVertexBufferBase);
		s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, dataSize);

		// Bind textures
		for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
			s_Data.TextureSlots[i]->Bind(i);

		s_Data._shaders2D.QuadShader->Use();
		RendererAPI::DrawIndexed(s_Data.QuadVertexArray, s_Data.QuadIndexCount);
		s_Data.Stats.DrawCalls++;
	}
	if (s_Data.CircleIndexCount)
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.CircleVertexBufferPtr - (uint8_t*)s_Data.CircleVertexBufferBase);
		s_Data.CircleVertexBuffer->SetData(s_Data.CircleVertexBufferBase, dataSize);

		s_Data._shaders2D.CircleShader->Use();
		RendererAPI::DrawIndexed(s_Data.CircleVertexArray, s_Data.CircleIndexCount);
		s_Data.Stats.DrawCalls++;
	}
	if (s_Data.LineVertexCount)
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.LineVertexBufferPtr - (uint8_t*)s_Data.LineVertexBufferBase);
		s_Data.LineVertexBuffer->SetData(s_Data.LineVertexBufferBase, dataSize);

		s_Data._shaders2D.LineShader->Use();
		RendererAPI::SetLineWidth(s_Data.LineWidth);
		RendererAPI::DrawLines(s_Data.LineVertexArray, s_Data.LineVertexCount);
		s_Data.Stats.DrawCalls++;
	}
}

void Renderer2D::EndScene()
{
	Flush();
}

void Renderer2D::NextBatch()
{
	Flush();
	StartBatch();
}

void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
{
	DrawQuad({ position.x, position.y, 0.0f }, size, color);
}

void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
{
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
		* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

	DrawQuad(transform, color);
}

void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const std::shared_ptr<Texture>& texture, float tilingFactor, const glm::vec4& tintColor)
{
	DrawQuad({ position.x, position.y, 0.0f }, size, texture, tilingFactor, tintColor);
}

void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const std::shared_ptr<Texture>& texture, float tilingFactor, const glm::vec4& tintColor)
{
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
		* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

	DrawQuad(transform, texture, tilingFactor, tintColor);
}

void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID)
{
	constexpr size_t quadVertexCount = 4;
	const float textureIndex = 0.0f; // White Texture
	constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
	const float tilingFactor = 1.0f;

	if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
		NextBatch();

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

	s_Data.Stats.QuadCount++;
}

void Renderer2D::DrawQuad(const glm::mat4& transform, const std::shared_ptr<Texture>& texture, float tilingFactor, const glm::vec4& tintColor, int entityID)
{
	constexpr size_t quadVertexCount = 4;
	constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

	if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
		NextBatch();

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
		if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
			NextBatch();

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

	s_Data.Stats.QuadCount++;
}

void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
{
	DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, color);
}

void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
{
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
		* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
		* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

	DrawQuad(transform, color);
}

void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const std::shared_ptr<Texture>& texture, float tilingFactor, const glm::vec4& tintColor)
{
	DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, texture, tilingFactor, tintColor);
}

void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const std::shared_ptr<Texture>& texture, float tilingFactor, const glm::vec4& tintColor)
{
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
		* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
		* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

	DrawQuad(transform, texture, tilingFactor, tintColor);
}

void Renderer2D::DrawCircle(const glm::mat4& transform, const glm::vec4& color, float thickness /*= 1.0f*/, float fade /*= 0.005f*/, int entityID /*= -1*/)
{
	// TODO: implement for circles
	// if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
	// 	NextBatch();

	for (size_t i = 0; i < 4; i++)
	{
		s_Data.CircleVertexBufferPtr->WorldPosition = transform * s_Data.QuadVertexPositions[i];
		s_Data.CircleVertexBufferPtr->LocalPosition = s_Data.QuadVertexPositions[i] * 2.0f;
		s_Data.CircleVertexBufferPtr->Color = color;
		s_Data.CircleVertexBufferPtr->Thickness = thickness;
		s_Data.CircleVertexBufferPtr->Fade = fade;
		s_Data.CircleVertexBufferPtr->EntityID = entityID;
		s_Data.CircleVertexBufferPtr++;
	}

	s_Data.CircleIndexCount += 6;

	s_Data.Stats.QuadCount++;
}

void Renderer2D::DrawLine(const glm::vec3& p0, glm::vec3& p1, const glm::vec4& color, int entityID)
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

void Renderer2D::DrawRect(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, int entityID)
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

void Renderer2D::DrawRect(const glm::mat4& transform, const glm::vec4& color, int entityID)
{
	glm::vec3 lineVertices[4];
	for (size_t i = 0; i < 4; i++)
		lineVertices[i] = transform * s_Data.QuadVertexPositions[i];

	DrawLine(lineVertices[0], lineVertices[1], color, entityID);
	DrawLine(lineVertices[1], lineVertices[2], color, entityID);
	DrawLine(lineVertices[2], lineVertices[3], color, entityID);
	DrawLine(lineVertices[3], lineVertices[0], color, entityID);
}

/*void Renderer2D::DrawSprite(const glm::mat4& transform, TextureComponent& src, int entityID)*/
/*{*/
/*	if (src.Texture)*/
/*		DrawQuad(transform, src.Texture, src.TilingFactor, src.Color, entityID);*/
/*	else*/
/*		DrawQuad(transform, src.Color, entityID);*/
/*}*/

void Renderer2D::RenderFullscreenFramebufferTexture(uint32_t textureID)
{
    s_Data._shaders2D.FramebufferShader->Use();
    s_Data._shaders2D.FramebufferShader->setInt("u_Texture", 0);
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

    // You need a shader bound here that uses a sampler2D at location 0.
    // Assume the shader is already active.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Renderer2D::DrawText(const std::string& text, const glm::vec2& position, float size, const glm::vec4& color, int entityID)
{
    if (!s_Data.FontInitialized)
        return;

    // --- Precompute total width and height for centering ---
    float textWidth = 0.0f;
    float maxBearingY = 0.0f;
    float maxBelowBaseline = 0.0f;

    for (char c : text)
    {
        auto it = s_Data.Characters.find(c);
        if (it == s_Data.Characters.end())
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
        auto it = s_Data.Characters.find(c);
        if (it == s_Data.Characters.end())
            continue;

        Renderer2DData::Character& ch = it->second;

        if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
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
            { 0.0f, 0.0f },
            { 1.0f, 0.0f },
            { 1.0f, 1.0f },
            { 0.0f, 1.0f }
        };

        float textureIndex = 0.0f;
        for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++) {
            if (s_Data.TextureSlots[i]->GetRendererID() == ch.TextureID) {
                textureIndex = (float)i;
                break;
            }
        }

        if (textureIndex == 0.0f) {
            if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
                NextBatch();

            textureIndex = (float)s_Data.TextureSlotIndex;
            s_Data.TextureSlots[s_Data.TextureSlotIndex++] = Texture::WrapExisting(ch.TextureID);
        }

        for (int i = 0; i < 4; i++) {
            s_Data.QuadVertexBufferPtr->Position = transform * glm::vec4(quadPositions[i], 1.0f);
            s_Data.QuadVertexBufferPtr->Color = color;
            s_Data.QuadVertexBufferPtr->TexCoord = texCoords[i];
            s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
            s_Data.QuadVertexBufferPtr->TilingFactor = 1.0f;
            s_Data.QuadVertexBufferPtr->EntityID = entityID;
            s_Data.QuadVertexBufferPtr++;
        }

        s_Data.QuadIndexCount += 6;

        x += (ch.Advance >> 6) * size;
    }

    s_Data.Stats.QuadCount += (int)text.length();
}


float Renderer2D::GetLineWidth()
{
	return s_Data.LineWidth;
}

void Renderer2D::SetLineWidth(float width)
{
	s_Data.LineWidth = width;
}

void Renderer2D::ResetStats()
{
	memset(&s_Data.Stats, 0, sizeof(Statistics));
}

Renderer2D::Statistics Renderer2D::GetStats()
{
	return s_Data.Stats;
}

