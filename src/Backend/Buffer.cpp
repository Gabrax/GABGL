#include "Buffer.h"
#include "BackendLogger.h"
#include "LightManager.h"
#include "glm/trigonometric.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <random>
#include <numbers>
#include <glad/glad.h>

VertexBuffer::VertexBuffer(uint32_t size)
{
	glCreateBuffers(1, &m_RendererID);
	glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
	glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
}

VertexBuffer::VertexBuffer(float* vertices, uint32_t size)
{
	glCreateBuffers(1, &m_RendererID);
	glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
	glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
}

VertexBuffer::~VertexBuffer()
{
	glDeleteBuffers(1, &m_RendererID);
}

void VertexBuffer::Bind() const
{
	glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
}

void VertexBuffer::Unbind() const
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexBuffer::SetData(const void* data, uint32_t size)
{
	glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
	glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
}

IndexBuffer::IndexBuffer(uint32_t* indices, uint32_t count)
	: m_Count(count)
{
	glCreateBuffers(1, &m_RendererID);

	// GL_ELEMENT_ARRAY_BUFFER is not valid without an actively bound VAO
	// Binding with GL_ARRAY_BUFFER allows the data to be loaded regardless of VAO state. 
	glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
	glBufferData(GL_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
}

IndexBuffer::~IndexBuffer()
{
	glDeleteBuffers(1, &m_RendererID);
}

void IndexBuffer::Bind() const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
}

void IndexBuffer::Unbind() const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
{
	switch (type)
	{
	case ShaderDataType::Float:    return GL_FLOAT;
	case ShaderDataType::Float2:   return GL_FLOAT;
	case ShaderDataType::Float3:   return GL_FLOAT;
	case ShaderDataType::Float4:   return GL_FLOAT;
	case ShaderDataType::Mat3:     return GL_FLOAT;
	case ShaderDataType::Mat4:     return GL_FLOAT;
	case ShaderDataType::Int:      return GL_INT;
	case ShaderDataType::Int2:     return GL_INT;
	case ShaderDataType::Int3:     return GL_INT;
	case ShaderDataType::Int4:     return GL_INT;
	case ShaderDataType::Bool:     return GL_BOOL;
	}

	GABGL_ASSERT(false, "Unknown ShaderDataType!");
	return 0;
}

VertexArray::VertexArray()
{
	glCreateVertexArrays(1, &m_RendererID);
}

VertexArray::~VertexArray()
{
	glDeleteVertexArrays(1, &m_RendererID);
}

void VertexArray::Bind() const
{
	glBindVertexArray(m_RendererID);
}

void VertexArray::Unbind() const
{
	glBindVertexArray(0);
}

void VertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer)
{
	GABGL_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Vertex Buffer has no layout!");

	glBindVertexArray(m_RendererID);
	vertexBuffer->Bind();

	const auto& layout = vertexBuffer->GetLayout();
	for (const auto& element : layout)
	{
		switch (element.Type)
		{
		case ShaderDataType::Float:
		case ShaderDataType::Float2:
		case ShaderDataType::Float3:
		case ShaderDataType::Float4:
		{
			glEnableVertexAttribArray(m_VertexBufferIndex);
			glVertexAttribPointer(m_VertexBufferIndex,
				element.GetComponentCount(),
				ShaderDataTypeToOpenGLBaseType(element.Type),
				element.Normalized ? GL_TRUE : GL_FALSE,
				layout.GetStride(),
				(const void*)element.Offset);
			m_VertexBufferIndex++;
			break;
		}
		case ShaderDataType::Int:
		case ShaderDataType::Int2:
		case ShaderDataType::Int3:
		case ShaderDataType::Int4:
		case ShaderDataType::Bool:
		{
			glEnableVertexAttribArray(m_VertexBufferIndex);
			glVertexAttribIPointer(m_VertexBufferIndex,
				element.GetComponentCount(),
				ShaderDataTypeToOpenGLBaseType(element.Type),
				layout.GetStride(),
				(const void*)element.Offset);
			m_VertexBufferIndex++;
			break;
		}
		case ShaderDataType::Mat3:
		case ShaderDataType::Mat4:
		{
			uint8_t count = element.GetComponentCount();
			for (uint8_t i = 0; i < count; i++)
			{
				glEnableVertexAttribArray(m_VertexBufferIndex);
				glVertexAttribPointer(m_VertexBufferIndex,
					count,
					ShaderDataTypeToOpenGLBaseType(element.Type),
					element.Normalized ? GL_TRUE : GL_FALSE,
					layout.GetStride(),
					(const void*)(element.Offset + sizeof(float) * count * i));
				glVertexAttribDivisor(m_VertexBufferIndex, 1);
				m_VertexBufferIndex++;
			}
			break;
		}
		default:
			GABGL_ASSERT(false, "Unknown ShaderDataType!");
		}
	}

	m_VertexBuffers.push_back(vertexBuffer);
}

void VertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer)
{
	glBindVertexArray(m_RendererID);
	indexBuffer->Bind();

	m_IndexBuffer = indexBuffer;
}

UniformBuffer::UniformBuffer(uint32_t size, uint32_t binding)
{
	glCreateBuffers(1, &m_RendererID);
	glNamedBufferData(m_RendererID, size, nullptr, GL_DYNAMIC_DRAW); // TODO: investigate usage hint
	glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_RendererID);
}

UniformBuffer::~UniformBuffer()
{
	glDeleteBuffers(1, &m_RendererID);
}


void UniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
{
	glNamedBufferSubData(m_RendererID, offset, size, data);
}

StorageBuffer::StorageBuffer(uint32_t size, uint32_t binding)
{
  Allocate(size);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, m_RendererID);
}

StorageBuffer::~StorageBuffer()
{
  if (m_RendererID != 0) {
      glDeleteBuffers(1, &m_RendererID);
      m_RendererID = 0;
      bufferSize = 0;
  }
}

void StorageBuffer::Allocate(size_t size)
{
  if (m_RendererID != 0) { SetData(size, nullptr); return; }

  glCreateBuffers(1, &m_RendererID);
  glNamedBufferStorage(m_RendererID, (GLsizeiptr)size, nullptr, GL_DYNAMIC_STORAGE_BIT);
  bufferSize = size;
}

void StorageBuffer::SetData(size_t size, void* data)
{
  if (size == 0) return;

  if (m_RendererID == 0) Allocate(size);

  if (bufferSize < size) {
      glDeleteBuffers(1, &m_RendererID);
      Allocate(size);  // Reallocate the buffer with new size
  }

  // Update buffer data (you could use glMapBufferRange for performance gains with large data)
  if (data != nullptr) glNamedBufferSubData(m_RendererID, 0, (GLsizeiptr)size, data); 
}

void* StorageBuffer::MapBuffer()
{
  if (m_RendererID == 0) return nullptr;
  return glMapNamedBuffer(m_RendererID, GL_READ_WRITE);
}

void StorageBuffer::UnmapBuffer()
{
  if (m_RendererID != 0) glUnmapNamedBuffer(m_RendererID);
}

PixelBuffer::PixelBuffer(size_t size) : m_Size(size)
{
  glGenBuffers(1, &m_ID);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_ID);
  glBufferData(GL_PIXEL_UNPACK_BUFFER, m_Size, nullptr, GL_STREAM_DRAW);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

PixelBuffer::~PixelBuffer()
{
  if (m_ID) glDeleteBuffers(1, &m_ID);
  if (m_Sync)
  {
      glDeleteSync(m_Sync);
      m_Sync = nullptr;
  }
}

void* PixelBuffer::Map()
{
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_ID);
  return glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, m_Size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
}

void PixelBuffer::Unmap()
{
  glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

  // Insert a fence sync after upload for later wait
  if (m_Sync) glDeleteSync(m_Sync);
  m_Sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void PixelBuffer::WaitForCompletion()
{
  if (m_Sync)
  {
      GLenum result = glClientWaitSync(m_Sync, GL_SYNC_FLUSH_COMMANDS_BIT, 1'000'000); // 1ms
      if (result == GL_TIMEOUT_EXPIRED || result == GL_WAIT_FAILED)
      {
          glWaitSync(m_Sync, 0, GL_TIMEOUT_IGNORED); // Block until done
      }
      glDeleteSync(m_Sync);
      m_Sync = nullptr;
  }
}

void PixelBuffer::Bind() const
{
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_ID);
}

void PixelBuffer::Unbind() const
{
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

static const uint32_t s_MaxFramebufferSize = 8192;

namespace Utils {

	static GLenum TextureTarget(bool multisampled)
	{
		return multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
	}

	static void CreateTextures(bool multisampled, uint32_t* outID, uint32_t count)
	{
		glCreateTextures(TextureTarget(multisampled), count, outID);
	}

	static void BindTexture(bool multisampled, uint32_t id)
	{
		glBindTexture(TextureTarget(multisampled), id);
	}

	static void AttachColorTexture(uint32_t id, int samples, GLenum internalFormat, GLenum format, uint32_t width, uint32_t height, int index)
  {
    bool multisampled = samples > 1;
    if (multisampled)
    {
      glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalFormat, width, height, GL_FALSE);
    }
    else
    {
      GLenum type = GL_UNSIGNED_BYTE;
      if (internalFormat == GL_RGBA16F || internalFormat == GL_R11F_G11F_B10F)
        type = GL_FLOAT;
      else if (internalFormat == GL_R32I)
        type = GL_INT;

      glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, nullptr);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, TextureTarget(multisampled), id, 0);
  }

	static void AttachDepthTexture(uint32_t id, int samples, GLenum format, GLenum attachmentType, uint32_t width, uint32_t height)
	{
		bool multisampled = samples > 1;
		if (multisampled)
		{
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
		}
		else
		{
			glTexStorage2D(GL_TEXTURE_2D, 1, format, width, height);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, TextureTarget(multisampled), id, 0);
	}

	static bool IsDepthFormat(FramebufferTextureFormat format)
	{
		switch (format)
		{
			case FramebufferTextureFormat::DEPTH24STENCIL8:  return true;
			case FramebufferTextureFormat::DEPTH:  return true;
		}

		return false;
	}

	static GLenum FBTextureFormatToGL(FramebufferTextureFormat format)
  {
    switch (format)
    {
      case FramebufferTextureFormat::RGBA8:         return GL_RGBA8;
      case FramebufferTextureFormat::RGBA16F:       return GL_RGBA16F;
      case FramebufferTextureFormat::R11F_G11F_B10F:return GL_R11F_G11F_B10F;
      case FramebufferTextureFormat::RED_INTEGER:   return GL_RED_INTEGER;
    }
    GABGL_ASSERT(false,"");
    return 0;
  }

}

FrameBuffer::FrameBuffer(const FramebufferSpecification& spec)
	: m_Specification(spec)
{
	for (auto spec : m_Specification.Attachments.Attachments)
	{
		if (!Utils::IsDepthFormat(spec.TextureFormat))
			m_ColorAttachmentSpecifications.emplace_back(spec);
		else
			m_DepthAttachmentSpecification = spec;
	}

	Invalidate();

  GABGL_WARN("Framebuffer created");
}

FrameBuffer::~FrameBuffer()
{
	glDeleteFramebuffers(1, &m_RendererID);
	glDeleteTextures(m_ColorAttachments.size(), m_ColorAttachments.data());
	glDeleteTextures(1, &m_DepthAttachment);
}

void FrameBuffer::Invalidate()
{
	if (m_RendererID)
	{
		glDeleteFramebuffers(1, &m_RendererID);
		glDeleteTextures(m_ColorAttachments.size(), m_ColorAttachments.data());
		glDeleteTextures(1, &m_DepthAttachment);

		m_ColorAttachments.clear();
		m_DepthAttachment = 0;
	}

	glCreateFramebuffers(1, &m_RendererID);
	glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

	bool multisample = m_Specification.Samples > 1;

	// Attachments
	if (m_ColorAttachmentSpecifications.size())
	{
		m_ColorAttachments.resize(m_ColorAttachmentSpecifications.size());
		Utils::CreateTextures(multisample, m_ColorAttachments.data(), m_ColorAttachments.size());

		for (size_t i = 0; i < m_ColorAttachments.size(); i++)
		{
			Utils::BindTexture(multisample, m_ColorAttachments[i]);
			switch (m_ColorAttachmentSpecifications[i].TextureFormat)
			{
			case FramebufferTextureFormat::RGBA8:
				Utils::AttachColorTexture(m_ColorAttachments[i], m_Specification.Samples, GL_RGBA8, GL_RGBA, m_Specification.Width, m_Specification.Height, i);
				break;
			case FramebufferTextureFormat::RED_INTEGER:
				Utils::AttachColorTexture(m_ColorAttachments[i], m_Specification.Samples, GL_R32I, GL_RED_INTEGER, m_Specification.Width, m_Specification.Height, i);
				break;
      case FramebufferTextureFormat::RGBA16F:
        Utils::AttachColorTexture(m_ColorAttachments[i], m_Specification.Samples, GL_RGBA16F, GL_RGBA, m_Specification.Width, m_Specification.Height, i);
        break;
      case FramebufferTextureFormat::R11F_G11F_B10F:
        Utils::AttachColorTexture(m_ColorAttachments[i], m_Specification.Samples, GL_R11F_G11F_B10F, GL_RGB, m_Specification.Width, m_Specification.Height, i);
        break;
			}
		}
	}

	if (m_DepthAttachmentSpecification.TextureFormat != FramebufferTextureFormat::None)
	{
		Utils::CreateTextures(multisample, &m_DepthAttachment, 1);
		Utils::BindTexture(multisample, m_DepthAttachment);
		switch (m_DepthAttachmentSpecification.TextureFormat)
		{
		case FramebufferTextureFormat::DEPTH24STENCIL8:
			Utils::AttachDepthTexture(m_DepthAttachment, m_Specification.Samples, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT, m_Specification.Width, m_Specification.Height);
			break;
		case FramebufferTextureFormat::DEPTH:
			Utils::AttachDepthTexture(m_DepthAttachment, m_Specification.Samples, GL_DEPTH_COMPONENT32, GL_DEPTH_ATTACHMENT, m_Specification.Width, m_Specification.Height);
			break;
		}
	}

	if (m_ColorAttachments.size() > 1)
	{
		GABGL_ASSERT(m_ColorAttachments.size() <= 4,"");
		GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
		glDrawBuffers(m_ColorAttachments.size(), buffers);
	}
	else if (m_ColorAttachments.empty())
	{
		// Only depth-pass
		glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
	}

	GABGL_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::Bind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
	glViewport(0, 0, m_Specification.Width, m_Specification.Height);
}

void FrameBuffer::UnBind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::Resize(uint32_t width, uint32_t height)
{
	if (width == 0 || height == 0 || width > s_MaxFramebufferSize || height > s_MaxFramebufferSize)
	{
		GABGL_WARN("Attempted to rezize framebuffer to {0}, {1}", width, height);
		return;
	}
	m_Specification.Width = width;
	m_Specification.Height = height;

	Invalidate();
}

int FrameBuffer::ReadPixel(uint32_t attachmentIndex, int x, int y)
{
	GABGL_ASSERT(attachmentIndex < m_ColorAttachments.size(),"");

	glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
	int pixelData;
	glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);
	return pixelData;

}

void FrameBuffer::ClearAttachment(uint32_t attachmentIndex, int value)
{
	GABGL_ASSERT(attachmentIndex < m_ColorAttachments.size(),"");

	auto& spec = m_ColorAttachmentSpecifications[attachmentIndex];
	glClearTexImage(m_ColorAttachments[attachmentIndex], 0,
	Utils::FBTextureFormatToGL(spec.TextureFormat), GL_INT, &value);
}

void FrameBuffer::AttachExternalColorTexture(GLuint textureID, uint32_t slot)
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + slot, GL_TEXTURE_2D, textureID, 0);
	m_ColorAttachments[slot] = textureID;
}

void FrameBuffer::AttachExternalDepthTexture(GLuint textureID)
{
  glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, textureID, 0);
  m_DepthAttachment = textureID;
}

std::shared_ptr<FrameBuffer> FrameBuffer::Create(const FramebufferSpecification& spec)
{
	return std::make_shared<FrameBuffer>(spec);
}

void FrameBuffer::BlitColor(const std::shared_ptr<FrameBuffer>& dst)
{
  glBindFramebuffer(GL_READ_FRAMEBUFFER, this->GetID());
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->GetID());

  glReadBuffer(GL_COLOR_ATTACHMENT0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);

  const auto& srcSpec = this->GetSpecification();
  const auto& dstSpec = dst->GetSpecification();

  glBlitFramebuffer(
      0, 0, srcSpec.Width, srcSpec.Height,
      0, 0, dstSpec.Width, dstSpec.Height,
      GL_COLOR_BUFFER_BIT,
      GL_NEAREST
  );

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}



static unsigned int quadVAO = 0;
static unsigned int quadVBO;
static void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}


#include "../engine.h"

BloomBuffer::BloomBuffer(const std::shared_ptr<Shader>& downsampleShader, const std::shared_ptr<Shader>& upsampleShader, const std::shared_ptr<Shader>& finalShader) : downsampleShader(downsampleShader), upsampleShader(upsampleShader), finalShader(finalShader)
{
  float windowWidth = Engine::GetInstance().GetMainWindow().GetWidth();
  float windowHeight = Engine::GetInstance().GetMainWindow().GetHeight();

  FramebufferSpecification hdrSpec;
	hdrSpec.Attachments = { FramebufferTextureFormat::RGBA16F, FramebufferTextureFormat::RGBA16F, FramebufferTextureFormat::DEPTH24STENCIL8 };
	hdrSpec.Width = Engine::GetInstance().GetMainWindow().GetWidth();
	hdrSpec.Height = Engine::GetInstance().GetMainWindow().GetHeight();
	m_hdrFB = FrameBuffer::Create(hdrSpec); 

  FramebufferSpecification blurSpec;
  blurSpec.Attachments = { FramebufferTextureFormat::RGBA16F};
  blurSpec.Width = windowWidth;
  blurSpec.Height = windowHeight;
  m_pingpongFB[0] = FrameBuffer::Create(blurSpec);
  m_pingpongFB[1] = FrameBuffer::Create(blurSpec);

  FramebufferSpecification mipFBOspec;
  mipFBOspec.Attachments = { FramebufferTextureFormat::R11F_G11F_B10F};
  mipFBOspec.Width = windowWidth;
  mipFBOspec.Height = windowHeight;
  m_mipFB = FrameBuffer::Create(mipFBOspec);

  // Store viewport size
  mSrcViewportSize = glm::ivec2(windowWidth, windowHeight);
  mSrcViewportSizeFloat = glm::vec2((float)windowWidth, (float)windowHeight);

  // Mip chain creation
  glm::vec2 mipSize((float)windowWidth, (float)windowHeight);
  glm::ivec2 mipIntSize((int)windowWidth, (int)windowHeight);

  for (GLuint i = 0; i < mipChainLength; i++)
  {
      bloomMip mip;

      mip.size = mipSize;
      mip.intSize = mipIntSize;

      glGenTextures(1, &mip.texture);
      glBindTexture(GL_TEXTURE_2D, mip.texture);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F, mip.intSize.x, mip.intSize.y, 0, GL_RGB, GL_FLOAT, nullptr);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

      GABGL_INFO("Created bloom mip x: {0}, y: {1}", mipIntSize.x, mipIntSize.y);

      mMipChain.emplace_back(mip);

      // Halve for next level, clamped to 1
      mipSize *= 0.5f;
      mipIntSize /= 2;
      mipSize = glm::max(mipSize, glm::vec2(1.0f));
      mipIntSize = glm::max(mipIntSize, glm::ivec2(1));
  }

  downsampleShader->Use();
  downsampleShader->setInt("srcTexture", 0);
  glUseProgram(0);

  upsampleShader->Use();
  upsampleShader->setInt("srcTexture", 0);
  glUseProgram(0);
}

void BloomBuffer::RenderBloomTexture(float filterRadius)
{
  m_mipFB->Bind();

  auto srcTexture = m_hdrFB->GetColorAttachmentRendererID(1);

  downsampleShader->Use();
  downsampleShader->setVec2("srcResolution", mSrcViewportSizeFloat);
  if (mKarisAverageOnDownsample) downsampleShader->setInt("mipLevel", 0);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, srcTexture);

  // ⬇️ Downsample through mip chain
  for (int i = 0; i < (int)mMipChain.size(); i++)
  {
    const bloomMip& mip = mMipChain[i];
    glViewport(0, 0, mip.size.x, mip.size.y);

    m_mipFB->AttachExternalColorTexture(mip.texture, 0);

    renderQuad();

    downsampleShader->setVec2("srcResolution", mip.size);
    glBindTexture(GL_TEXTURE_2D, mip.texture);

    if (i == 0) downsampleShader->setInt("mipLevel", 1);
  }

  glUseProgram(0);

  // ⬆️ Upsample pass
  upsampleShader->Use();
  upsampleShader->setFloat("filterRadius", filterRadius);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
  glBlendEquation(GL_FUNC_ADD);

  for (int i = (int)mMipChain.size() - 1; i > 0; i--)
  {
    const bloomMip& mip = mMipChain[i];
    const bloomMip& nextMip = mMipChain[i - 1];

    glViewport(0, 0, nextMip.size.x, nextMip.size.y);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mip.texture);

    m_mipFB->AttachExternalColorTexture(nextMip.texture, 0);

    renderQuad();
  }

  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_BLEND);
  glUseProgram(0);
}

void BloomBuffer::Resize(int newWidth, int newHeight)
{
  // Destroy old mip textures
  for (auto& mip : mMipChain)
  {
      glDeleteTextures(1, &mip.texture);
      mip.texture = 0;
  }
  mMipChain.clear();

  glm::vec2 mipSize = glm::vec2((float)newWidth, (float)newHeight);
  glm::ivec2 mipIntSize = glm::ivec2(newWidth, newHeight);

  // Rebuild mip chain
  for (GLuint i = 0; i < mipChainLength; i++)
  {
    bloomMip mip;
    mip.size = mipSize;
    mip.intSize = mipIntSize;

    glGenTextures(1, &mip.texture);
    glBindTexture(GL_TEXTURE_2D, mip.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F, mipIntSize.x, mipIntSize.y, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GABGL_INFO("Resized bloom mip x: {0}, y: {1}", mipIntSize.x, mipIntSize.y);
    mMipChain.emplace_back(mip);

    mipSize *= 0.5f;
    mipIntSize /= 2;
    mipSize = glm::max(mipSize, glm::vec2(1.0f));
    mipIntSize = glm::max(mipIntSize, glm::ivec2(1));
  }

  // Attach first mip texture to framebuffer using your API
  m_mipFB->Bind();
  m_mipFB->AttachExternalColorTexture(mMipChain[0].texture, 0);
  m_mipFB->UnBind();

  // Resize main HDR framebuffer
  m_hdrFB->Resize(newWidth, newHeight);
}

void BloomBuffer::Bind() const 
{
  m_hdrFB->Bind();
}

void BloomBuffer::UnBind() const 
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void BloomBuffer::CompositeBloomOver(const std::shared_ptr<FrameBuffer>& target)
{
  target->Bind();

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE); 
  glBlendEquation(GL_FUNC_ADD);

  finalShader->Use();

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_hdrFB->GetColorAttachmentRendererID(0));
  finalShader->setInt("scene", 0);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, mMipChain[0].texture);
  finalShader->setInt("bloomBlur", 1);

  renderQuad();

  glDisable(GL_BLEND);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void BloomBuffer::BlitDepthFrom(const std::shared_ptr<FrameBuffer>& src)
{
  glBindFramebuffer(GL_READ_FRAMEBUFFER, src->GetID());      
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_hdrFB->GetID());     

  const auto& srcSpec = src->GetSpecification();
  const auto& dstSpec = m_hdrFB->GetSpecification();

  glBlitFramebuffer(
      0, 0, srcSpec.Width, srcSpec.Height,
      0, 0, dstSpec.Width, dstSpec.Height,
      GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, 
      GL_NEAREST
  );

  glBindFramebuffer(GL_FRAMEBUFFER, 0); 
}

void BloomBuffer::BlitDepthTo(const std::shared_ptr<FrameBuffer>& dst)
{
  glBindFramebuffer(GL_READ_FRAMEBUFFER, m_hdrFB->GetID());      
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->GetID());     

  const auto& srcSpec = m_hdrFB->GetSpecification();
  const auto& dstSpec = dst->GetSpecification();

  glBlitFramebuffer(
      0, 0, srcSpec.Width, srcSpec.Height,
      0, 0, dstSpec.Width, dstSpec.Height,
      GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, 
      GL_NEAREST
  );

  glBindFramebuffer(GL_FRAMEBUFFER, 0); 
}

std::shared_ptr<BloomBuffer> BloomBuffer::Create(const std::shared_ptr<Shader>& downsampleShader, const std::shared_ptr<Shader>& upsampleShader, const std::shared_ptr<Shader>& finalShader)
{
  return std::make_shared<BloomBuffer>(downsampleShader,upsampleShader,finalShader);
}

DirectShadowBuffer::DirectShadowBuffer(float shadowWidth, float shadowHeight, float offsetSize, float filterSize, float randomRadius) : m_shadowWidth(shadowWidth), m_shadowHeight(shadowHeight)
{
  glGenTextures(1, &m_depthMap);
  glBindTexture(GL_TEXTURE_2D, m_depthMap);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowWidth, shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  float borderColor[] = {1.0, 1.0, 1.0, 1.0};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

  glGenFramebuffers(1, &m_FBO);
  glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthMap, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  std::vector<float> Data;

  int BufferSize = offsetSize * offsetSize * filterSize * filterSize * 2;

  Data.resize(BufferSize);

  float PI = std::numbers::pi;

  int Index = 0;
  for (int TexY = 0; TexY < offsetSize; TexY++)
  {
    for (int TexX = 0; TexX < offsetSize; TexX++)
    {
      for (int v = filterSize - 1; v >= 0; v--)
      {
        for (int u = 0; u < filterSize; u++)
        {
          float x = ((float)u + 0.5f + Jitter()) / (float)filterSize;
          float y = ((float)v + 0.5f + Jitter()) / (float)filterSize;

          assert(Index + 1 < Data.size());
          Data[Index]     = sqrtf(y) * cosf(2 * PI * x);
          Data[Index + 1] = sqrtf(y) * sinf(2 * PI * x);

          Index += 2;
        }
      }
    }
  }

  int NumFilterSamples = filterSize * filterSize;

  glActiveTexture(GL_TEXTURE1);
  glGenTextures(1, &m_offsetTexture);
  glBindTexture(GL_TEXTURE_3D, m_offsetTexture);
  glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA32F, NumFilterSamples / 2, offsetSize, offsetSize );
  glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, NumFilterSamples / 2, offsetSize, offsetSize, GL_RGBA, GL_FLOAT, &Data[0]);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glBindTexture(GL_TEXTURE_3D, 0);

  float near_plane = 0.01f, far_plane = 100.0f, orthoSize = 50.0f; 
  m_shadowProj = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, near_plane, far_plane);

  struct Data
  {
    glm::vec2 windowSize; 
    glm::vec2 offsetSize_filterSize; 
    glm::vec2 randomRadius;
  } data;

  data.windowSize = glm::vec2(shadowWidth, shadowHeight);
  data.offsetSize_filterSize = glm::vec2(float(offsetSize), float(filterSize));
  data.randomRadius = glm::vec2(3.0f, 0.0f);

  buffer = UniformBuffer::Create(sizeof(Data), 1);
  buffer->SetData(&data, sizeof(Data));

  m_shadowVIew = glm::mat4(1);
}

DirectShadowBuffer::~DirectShadowBuffer()
{
  glDeleteFramebuffers(1, &m_FBO);
  glDeleteTextures(1, &m_depthMap);
}

void DirectShadowBuffer::Bind() const
{
  glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
  glViewport(0, 0, m_shadowWidth, m_shadowHeight);
}

void DirectShadowBuffer::UnBind() const
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DirectShadowBuffer::BindShadowTextureForReading(GLenum textureUnit) const
{
  glActiveTexture(textureUnit);
  glBindTexture(GL_TEXTURE_2D, m_depthMap);
}

void DirectShadowBuffer::BindOffsetTextureForReading(GLenum textureUnit) const
{
  glActiveTexture(textureUnit);
  glBindTexture(GL_TEXTURE_3D, m_offsetTexture);
}

void DirectShadowBuffer::UpdateShadowView(const glm::vec3& rotation)
{
  glm::vec3 lightDir = glm::normalize(rotation); 
  glm::vec3 lightTarget = glm::vec3(0.0f); 
  glm::vec3 lightPos = lightTarget - lightDir * 30.0f; 

  m_shadowVIew = glm::lookAt(lightPos, lightTarget, glm::vec3(0, 1, 0));
}

float DirectShadowBuffer::Jitter()
{
  static std::default_random_engine generator;
  static std::uniform_real_distribution<float> distrib(-0.5f, 0.5f);
  return distrib(generator);
}

std::shared_ptr<DirectShadowBuffer> DirectShadowBuffer::Create(float shadowWidth, float shadowHeight, float offsetSize, float filterSize, float randomRadius)
{
  return std::make_shared<DirectShadowBuffer>(shadowWidth, shadowHeight, offsetSize, filterSize, randomRadius);
}

OmniDirectShadowBuffer::OmniDirectShadowBuffer(uint32_t shadowWidth, uint32_t shadowHeight) 
{
  m_shadowProj = glm::perspective(glm::radians(90.0f), float(shadowWidth) / float(shadowHeight), 0.1f, 20.0f);

  FramebufferSpecification mipFBOspec;
  mipFBOspec.Attachments = { FramebufferTextureFormat::DEPTH};
  mipFBOspec.Width = shadowWidth;
  mipFBOspec.Height = shadowHeight;
  m_testFB = FrameBuffer::Create(mipFBOspec);

  glGenTextures(1, &m_depthCubemapArray);
  glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_depthCubemapArray);
  glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY,0,GL_R16F,shadowWidth,shadowHeight,6 * 20,0,GL_RED,GL_FLOAT,nullptr);
  glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  m_Directions =
  {
    CubemapDirection{ GL_TEXTURE_CUBE_MAP_POSITIVE_X, glm::vec3(1, 0, 0), glm::vec3(0, -1, 0) },
    CubemapDirection{ GL_TEXTURE_CUBE_MAP_NEGATIVE_X, glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0) },
    CubemapDirection{ GL_TEXTURE_CUBE_MAP_POSITIVE_Y, glm::vec3(0, 1, 0), glm::vec3(0, 0, 1) },
    CubemapDirection{ GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, glm::vec3(0, -1, 0), glm::vec3(0, 0, -1) },
    CubemapDirection{ GL_TEXTURE_CUBE_MAP_POSITIVE_Z, glm::vec3(0, 0, 1), glm::vec3(0, -1, 0) },
    CubemapDirection{ GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, glm::vec3(0, 0, -1), glm::vec3(0, -1, 0) }
  };
}

void OmniDirectShadowBuffer::Bind() const 
{
  m_testFB->Bind();
}

void OmniDirectShadowBuffer::BindForWriting(uint32_t cubemapIndex, uint32_t faceIndex)
{
  glFramebufferTextureLayer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,m_depthCubemapArray,0,cubemapIndex * 6 + faceIndex);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
}

void OmniDirectShadowBuffer::BindForReading(GLenum TextureUnit)
{
  glActiveTexture(TextureUnit);
  glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_depthCubemapArray);
}

void OmniDirectShadowBuffer::UnBind() const 
{
  m_testFB->UnBind();
}

std::shared_ptr<OmniDirectShadowBuffer> OmniDirectShadowBuffer::Create(uint32_t shadowWidth, uint32_t shadowHeight)
{
  return std::make_shared<OmniDirectShadowBuffer>(shadowWidth, shadowHeight);
}

