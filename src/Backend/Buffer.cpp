#include "Buffer.h"
#include "BackendLogger.h"
#include "glm/trigonometric.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <random>
#include <numbers>
#include <glad/glad.h>
#include "../engine.h"

VertexBuffer::VertexBuffer(uint32_t size)
{
  glCreateBuffers(1, &m_RendererID);
  glNamedBufferData(m_RendererID, size, nullptr, GL_DYNAMIC_DRAW);
}

VertexBuffer::VertexBuffer(float* vertices, uint32_t size)
{
  glCreateBuffers(1, &m_RendererID);
  glNamedBufferData(m_RendererID, size, vertices, GL_STATIC_DRAW);
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
  // LEGACY
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexBuffer::SetData(const void* data, uint32_t size)
{
  glNamedBufferSubData(m_RendererID, 0, size, data);
}

IndexBuffer::IndexBuffer(uint32_t* indices, uint32_t count) : m_Count(count)
{
  glCreateBuffers(1, &m_RendererID);
  glNamedBufferData(m_RendererID, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
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
  // LEGACY
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

  const auto& layout = vertexBuffer->GetLayout();
  GLuint vbID = vertexBuffer->GetID();

  for (const auto& element : layout)
  {
    switch (element.Type)
    {
      case ShaderDataType::Float:
      case ShaderDataType::Float2:
      case ShaderDataType::Float3:
      case ShaderDataType::Float4:
      {
          glEnableVertexArrayAttrib(m_RendererID, m_VertexBufferIndex);
          glVertexArrayVertexBuffer(m_RendererID, m_VertexBufferIndex, vbID, 0, layout.GetStride());
          glVertexArrayAttribFormat(m_RendererID, m_VertexBufferIndex,
              element.GetComponentCount(),
              ShaderDataTypeToOpenGLBaseType(element.Type),
              element.Normalized ? GL_TRUE : GL_FALSE,
              element.Offset);
          glVertexArrayAttribBinding(m_RendererID, m_VertexBufferIndex, m_VertexBufferIndex);
          m_VertexBufferIndex++;
          break;
      }
      case ShaderDataType::Int:
      case ShaderDataType::Int2:
      case ShaderDataType::Int3:
      case ShaderDataType::Int4:
      case ShaderDataType::Bool:
      {
          glEnableVertexArrayAttrib(m_RendererID, m_VertexBufferIndex);
          glVertexArrayVertexBuffer(m_RendererID, m_VertexBufferIndex, vbID, 0, layout.GetStride());
          glVertexArrayAttribIFormat(m_RendererID, m_VertexBufferIndex,
              element.GetComponentCount(),
              ShaderDataTypeToOpenGLBaseType(element.Type),
              element.Offset);
          glVertexArrayAttribBinding(m_RendererID, m_VertexBufferIndex, m_VertexBufferIndex);
          m_VertexBufferIndex++;
          break;
      }
      case ShaderDataType::Mat3:
      case ShaderDataType::Mat4:
      {
          uint8_t count = element.GetComponentCount();
          for (uint8_t i = 0; i < count; i++)
          {
              glEnableVertexArrayAttrib(m_RendererID, m_VertexBufferIndex);
              glVertexArrayVertexBuffer(m_RendererID, m_VertexBufferIndex, vbID, 0, layout.GetStride());
              glVertexArrayAttribFormat(m_RendererID, m_VertexBufferIndex,
                  count,
                  ShaderDataTypeToOpenGLBaseType(element.Type),
                  element.Normalized ? GL_TRUE : GL_FALSE,
                  element.Offset + sizeof(float) * count * i);
              glVertexArrayAttribBinding(m_RendererID, m_VertexBufferIndex, m_VertexBufferIndex);
              glVertexArrayBindingDivisor(m_RendererID, m_VertexBufferIndex, 1);
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
  glVertexArrayElementBuffer(m_RendererID, indexBuffer->GetID());
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

void StorageBuffer::SetSubData(GLintptr offset, GLsizeiptr size, const void* data)
{
  if (m_RendererID != 0 && data != nullptr)
  {
      glNamedBufferSubData(m_RendererID, offset, size, data);
  }
}

void* StorageBuffer::MapBuffer()
{
  return (m_RendererID != 0) ? glMapNamedBuffer(m_RendererID, GL_READ_WRITE) : nullptr;
}

void StorageBuffer::UnmapBuffer()
{
  if (m_RendererID != 0) glUnmapNamedBuffer(m_RendererID);
}

PixelBuffer::PixelBuffer(size_t size) : m_Size(size)
{
  glCreateBuffers(1, &m_ID);
  glNamedBufferData(m_ID, m_Size, nullptr, GL_STREAM_DRAW); 
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
  return glMapNamedBufferRange(m_ID, 0, m_Size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
}

void PixelBuffer::Unmap()
{
  glUnmapNamedBuffer(m_ID);

  // Insert fence after unmap for sync (same behavior)
  if (m_Sync)
      glDeleteSync(m_Sync);
  m_Sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void PixelBuffer::WaitForCompletion()
{
  if (m_Sync)
  {
    GLenum result = glClientWaitSync(m_Sync, GL_SYNC_FLUSH_COMMANDS_BIT, 1'000'000); // 1ms
    if (result == GL_TIMEOUT_EXPIRED || result == GL_WAIT_FAILED)
    {
        glWaitSync(m_Sync, 0, GL_TIMEOUT_IGNORED); // Block until finished
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

namespace Utils
{
	static GLenum TextureTarget(bool multisampled)
	{
		return multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
	}

	static void CreateTextures(bool multisampled, uint32_t* outID, uint32_t count)
	{
		glCreateTextures(TextureTarget(multisampled), count, outID);
	}

	static void AttachColorTexture(uint32_t id, int samples, GLenum internalFormat, GLenum format, uint32_t width, uint32_t height, int index, uint32_t framebufferID)
	{
		bool multisampled = samples > 1;

		if (multisampled)
		{
			glTextureStorage2DMultisample(id, samples, internalFormat, width, height, GL_FALSE);
		}
		else
		{
			GLenum type = GL_UNSIGNED_BYTE;
			if (internalFormat == GL_RGBA16F || internalFormat == GL_R11F_G11F_B10F)
				type = GL_FLOAT;
			else if (internalFormat == GL_R32I)
				type = GL_INT;

			glTextureStorage2D(id, 1, internalFormat, width, height);

			glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTextureParameteri(id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		}

		glNamedFramebufferTexture(framebufferID, GL_COLOR_ATTACHMENT0 + index, id, 0);
	}

	static void AttachDepthTexture(uint32_t id, int samples, GLenum format, GLenum attachmentType, uint32_t width, uint32_t height, uint32_t framebufferID)
	{
		bool multisampled = samples > 1;

		if (multisampled)
		{
			glTextureStorage2DMultisample(id, samples, format, width, height, GL_FALSE);
		}
		else
		{
			glTextureStorage2D(id, 1, format, width, height);

			glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTextureParameteri(id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		}

		glNamedFramebufferTexture(framebufferID, attachmentType, id, 0);
	}

	static bool IsDepthFormat(FramebufferTextureFormat format)
	{
		switch (format)
		{
			case FramebufferTextureFormat::DEPTH24STENCIL8:  return true;
			case FramebufferTextureFormat::DEPTH:            return true;
		}
		return false;
	}

	static GLenum FBTextureFormatToGL(FramebufferTextureFormat format)
	{
		switch (format)
		{
			case FramebufferTextureFormat::RGBA8:          return GL_RGBA8;
			case FramebufferTextureFormat::RGBA16F:        return GL_RGBA16F;
			case FramebufferTextureFormat::R11F_G11F_B10F: return GL_R11F_G11F_B10F;
			case FramebufferTextureFormat::RED_INTEGER:    return GL_RED_INTEGER;
		}
		GABGL_ASSERT(false, "Unknown FramebufferTextureFormat!");
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
	bool multisample = m_Specification.Samples > 1;

	// Color attachments
	if (!m_ColorAttachmentSpecifications.empty())
	{
		m_ColorAttachments.resize(m_ColorAttachmentSpecifications.size());
		Utils::CreateTextures(multisample, m_ColorAttachments.data(), m_ColorAttachments.size());

		for (size_t i = 0; i < m_ColorAttachments.size(); i++)
		{
			auto& spec = m_ColorAttachmentSpecifications[i];
			GLenum internalFormat = 0, format = 0;

			switch (spec.TextureFormat)
			{
				case FramebufferTextureFormat::RGBA8:
					internalFormat = GL_RGBA8; format = GL_RGBA; break;
				case FramebufferTextureFormat::RED_INTEGER:
					internalFormat = GL_R32I; format = GL_RED_INTEGER; break;
				case FramebufferTextureFormat::RGBA16F:
					internalFormat = GL_RGBA16F; format = GL_RGBA; break;
				case FramebufferTextureFormat::R11F_G11F_B10F:
					internalFormat = GL_R11F_G11F_B10F; format = GL_RGB; break;
			}

			Utils::AttachColorTexture(m_ColorAttachments[i], m_Specification.Samples, internalFormat, format,
									  m_Specification.Width, m_Specification.Height, static_cast<int>(i), m_RendererID);
		}
	}

	// Depth attachment
	if (m_DepthAttachmentSpecification.TextureFormat != FramebufferTextureFormat::None)
	{
		Utils::CreateTextures(multisample, &m_DepthAttachment, 1);

		switch (m_DepthAttachmentSpecification.TextureFormat)
		{
			case FramebufferTextureFormat::DEPTH24STENCIL8:
				Utils::AttachDepthTexture(m_DepthAttachment, m_Specification.Samples, GL_DEPTH24_STENCIL8,
										  GL_DEPTH_STENCIL_ATTACHMENT, m_Specification.Width, m_Specification.Height, m_RendererID);
				break;
			case FramebufferTextureFormat::DEPTH:
				Utils::AttachDepthTexture(m_DepthAttachment, m_Specification.Samples, GL_DEPTH_COMPONENT32,
										  GL_DEPTH_ATTACHMENT, m_Specification.Width, m_Specification.Height, m_RendererID);
				break;
		}
	}

	// Draw buffers
	if (m_ColorAttachments.size() > 1)
	{
		GABGL_ASSERT(m_ColorAttachments.size() <= 4, "");
		GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
		glNamedFramebufferDrawBuffers(m_RendererID, static_cast<GLsizei>(m_ColorAttachments.size()), buffers);
	}
	else if (m_ColorAttachments.empty())
	{
		// Only depth pass
		glNamedFramebufferDrawBuffer(m_RendererID, GL_NONE);
		glNamedFramebufferReadBuffer(m_RendererID, GL_NONE);
	}

	GABGL_ASSERT(glCheckNamedFramebufferStatus(m_RendererID, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");
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
	glClearTexImage(m_ColorAttachments[attachmentIndex],0,Utils::FBTextureFormatToGL(spec.TextureFormat), GL_INT, &value);
}

void FrameBuffer::AttachExternalColorTexture(GLuint textureID, uint32_t slot)
{
	GABGL_ASSERT(slot < m_ColorAttachments.size(), "Invalid attachment slot");
	glNamedFramebufferTexture(m_RendererID, GL_COLOR_ATTACHMENT0 + slot, textureID, 0);
	m_ColorAttachments[slot] = textureID;
}

void FrameBuffer::AttachExternalDepthTexture(GLuint textureID)
{
	glNamedFramebufferTexture(m_RendererID, GL_DEPTH_ATTACHMENT, textureID, 0);
	m_DepthAttachment = textureID;
}

void FrameBuffer::BlitColor(const std::shared_ptr<FrameBuffer>& dst)
{
  glBindFramebuffer(GL_READ_FRAMEBUFFER, this->GetID());
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->GetID());

  glReadBuffer(GL_COLOR_ATTACHMENT0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);

  const auto& srcSpec = this->GetSpecification();
  const auto& dstSpec = dst->GetSpecification();

  glBlitNamedFramebuffer(
		this->GetID(), dst->GetID(),
		0, 0, srcSpec.Width, srcSpec.Height,
		0, 0, dstSpec.Width, dstSpec.Height,
		GL_COLOR_BUFFER_BIT,
		GL_NEAREST
	);
}

std::shared_ptr<FrameBuffer> FrameBuffer::Create(const FramebufferSpecification& spec)
{
	return std::make_shared<FrameBuffer>(spec);
}

GeometryBuffer::GeometryBuffer(uint32_t width, uint32_t height) : m_Width(width), m_Height(height)
{
  Invalidate();
}

GeometryBuffer::~GeometryBuffer()
{
  glDeleteFramebuffers(1, &m_FBO);
  glDeleteTextures(1, &m_PositionAttachment);
  glDeleteTextures(1, &m_NormalAttachment);
  glDeleteTextures(1, &m_AlbedoSpecAttachment);
  glDeleteTextures(1, &m_DepthAttachment);
}

void GeometryBuffer::Invalidate()
{
  if (m_FBO)
  {
    glDeleteFramebuffers(1, &m_FBO);
    glDeleteTextures(1, &m_PositionAttachment);
    glDeleteTextures(1, &m_NormalAttachment);
    glDeleteTextures(1, &m_AlbedoSpecAttachment);
    glDeleteTextures(1, &m_DepthAttachment);
  }

  glCreateFramebuffers(1, &m_FBO);

  glCreateTextures(GL_TEXTURE_2D, 1, &m_PositionAttachment);
  glTextureStorage2D(m_PositionAttachment, 1, GL_RGBA16F, m_Width, m_Height);
  glTextureParameteri(m_PositionAttachment, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTextureParameteri(m_PositionAttachment, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glNamedFramebufferTexture(m_FBO, GL_COLOR_ATTACHMENT0, m_PositionAttachment, 0);

  glCreateTextures(GL_TEXTURE_2D, 1, &m_NormalAttachment);
  glTextureStorage2D(m_NormalAttachment, 1, GL_RGBA16F, m_Width, m_Height);
  glTextureParameteri(m_NormalAttachment, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTextureParameteri(m_NormalAttachment, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glNamedFramebufferTexture(m_FBO, GL_COLOR_ATTACHMENT1, m_NormalAttachment, 0);

  glCreateTextures(GL_TEXTURE_2D, 1, &m_AlbedoSpecAttachment);
  glTextureStorage2D(m_AlbedoSpecAttachment, 1, GL_RGBA8, m_Width, m_Height);
  glTextureParameteri(m_AlbedoSpecAttachment, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTextureParameteri(m_AlbedoSpecAttachment, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glNamedFramebufferTexture(m_FBO, GL_COLOR_ATTACHMENT2, m_AlbedoSpecAttachment, 0);

  glCreateTextures(GL_TEXTURE_2D, 1, &m_DepthAttachment);
  glTextureStorage2D(m_DepthAttachment, 1, GL_DEPTH_COMPONENT24, m_Width, m_Height);
  glTextureParameteri(m_DepthAttachment, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTextureParameteri(m_DepthAttachment, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glNamedFramebufferTexture(m_FBO, GL_DEPTH_ATTACHMENT, m_DepthAttachment, 0);

  GLenum attachments[3] = {GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2};
  glNamedFramebufferDrawBuffers(m_FBO, 3, attachments);

  if (glCheckNamedFramebufferStatus(m_FBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) GABGL_ERROR("GeometryBuffer Error: Framebuffer is not complete!");

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GeometryBuffer::Bind() const
{
  glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
	glViewport(0, 0, m_Width, m_Height);
}

void GeometryBuffer::UnBind() const
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GeometryBuffer::BindPositionTextureForReading(GLenum textureUnit)
{
  glBindTextureUnit(textureUnit - GL_TEXTURE0, m_PositionAttachment);
}

void GeometryBuffer::BindNormalTextureForReading(GLenum textureUnit)
{
  glBindTextureUnit(textureUnit - GL_TEXTURE0, m_NormalAttachment);
}

void GeometryBuffer::BindAlbedoTextureForReading(GLenum textureUnit)
{
  glBindTextureUnit(textureUnit - GL_TEXTURE0, m_AlbedoSpecAttachment);
}

void GeometryBuffer::BlitDepthTo(const std::shared_ptr<FrameBuffer>& dst)
{
  glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);      
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->GetID());     

  const auto& dstSpec = dst->GetSpecification();

  glBlitNamedFramebuffer(
		m_FBO, dst->GetID(),
		0, 0, m_Width, m_Height,
		0, 0, dstSpec.Width, dstSpec.Height,
		GL_DEPTH_BUFFER_BIT,
		GL_NEAREST
	);
}

void GeometryBuffer::Resize(uint32_t width, uint32_t height)
{
  if (m_Width == width && m_Height == height) return;

  m_Width = width;
  m_Height = height;
  Invalidate();
}

std::shared_ptr<GeometryBuffer> GeometryBuffer::Create(uint32_t width, uint32_t height)
{
  return std::make_shared<GeometryBuffer>(width, height);
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

BloomBuffer::BloomBuffer(const std::shared_ptr<Shader>& downsampleShader, const std::shared_ptr<Shader>& upsampleShader, const std::shared_ptr<Shader>& finalShader) : downsampleShader(downsampleShader), upsampleShader(upsampleShader), finalShader(finalShader)
{
  float windowWidth = Engine::GetInstance().GetMainWindow().GetWidth();
  float windowHeight = Engine::GetInstance().GetMainWindow().GetHeight();

  FramebufferSpecification hdrSpec;
	hdrSpec.Attachments = { FramebufferTextureFormat::RGBA16F, FramebufferTextureFormat::RGBA16F };
	hdrSpec.Width = Engine::GetInstance().GetMainWindow().GetWidth();
	hdrSpec.Height = Engine::GetInstance().GetMainWindow().GetHeight();
	m_hdrFB = FrameBuffer::Create(hdrSpec); 

  FramebufferSpecification blurSpec;
  blurSpec.Attachments = { FramebufferTextureFormat::RGBA16F };
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

    glCreateTextures(GL_TEXTURE_2D, 1, &mip.texture);
    glTextureStorage2D(mip.texture, 1, GL_R11F_G11F_B10F, mip.intSize.x, mip.intSize.y);

    glTextureParameteri(mip.texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(mip.texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(mip.texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(mip.texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GABGL_INFO("Created bloom mip x: {0}, y: {1}", mipIntSize.x, mipIntSize.y);

    mMipChain.emplace_back(mip);

    // Halve for next level, clamped to 1
    mipSize *= 0.5f;
    mipIntSize /= 2;
    mipSize = glm::max(mipSize, glm::vec2(1.0f));
    mipIntSize = glm::max(mipIntSize, glm::ivec2(1));
  }

  downsampleShader->Bind();
  downsampleShader->SetInt("srcTexture", 0);
  downsampleShader->UnBind();

  upsampleShader->Bind();
  upsampleShader->SetInt("srcTexture", 0);
  upsampleShader->UnBind();
}

void BloomBuffer::RenderBloomTexture(float filterRadius)
{
  m_mipFB->Bind();

  auto srcTexture = m_hdrFB->GetColorAttachmentRendererID(1);

  downsampleShader->Bind();
  downsampleShader->SetVec2("srcResolution", mSrcViewportSizeFloat);
  if (mKarisAverageOnDownsample) downsampleShader->SetInt("mipLevel", 0);

  glBindTextureUnit(0, srcTexture);

  for (int i = 0; i < (int)mMipChain.size(); i++)
  {
    const bloomMip& mip = mMipChain[i];
    glViewport(0, 0, mip.size.x, mip.size.y);

    m_mipFB->AttachExternalColorTexture(mip.texture, 0);

    renderQuad();

    downsampleShader->SetVec2("srcResolution", mip.size);
    glBindTextureUnit(0, mip.texture);

    if (i == 0) downsampleShader->SetInt("mipLevel", 1);
  }

  downsampleShader->UnBind();

  upsampleShader->Bind();
  upsampleShader->SetFloat("filterRadius", filterRadius);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
  glBlendEquation(GL_FUNC_ADD);

  for (int i = (int)mMipChain.size() - 1; i > 0; i--)
  {
    const bloomMip& mip = mMipChain[i];
    const bloomMip& nextMip = mMipChain[i - 1];
    glViewport(0, 0, nextMip.size.x, nextMip.size.y);

    glBindTextureUnit(0, mip.texture);

    m_mipFB->AttachExternalColorTexture(nextMip.texture, 0);

    renderQuad();
  }

  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_BLEND);
  upsampleShader->UnBind();
}

void BloomBuffer::Resize(int newWidth, int newHeight)
{
  for (auto& mip : mMipChain)
  {
    glDeleteTextures(1, &mip.texture);
    mip.texture = 0;
  }
  mMipChain.clear();

  glm::vec2 mipSize = glm::vec2((float)newWidth, (float)newHeight);
  glm::ivec2 mipIntSize = glm::ivec2(newWidth, newHeight);

  for (GLuint i = 0; i < mipChainLength; i++)
  {
      bloomMip mip;
      mip.size = mipSize;
      mip.intSize = mipIntSize;

      glCreateTextures(GL_TEXTURE_2D, 1, &mip.texture);

      glTextureStorage2D(mip.texture, 1, GL_R11F_G11F_B10F, mip.intSize.x, mip.intSize.y);

      glTextureParameteri(mip.texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTextureParameteri(mip.texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTextureParameteri(mip.texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTextureParameteri(mip.texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

      GABGL_INFO("Resized bloom mip x: {0}, y: {1}", mipIntSize.x, mipIntSize.y);
      mMipChain.emplace_back(mip);

      mipSize *= 0.5f;
      mipIntSize /= 2;
      mipSize = glm::max(mipSize, glm::vec2(1.0f));
      mipIntSize = glm::max(mipIntSize, glm::ivec2(1));
  }

  m_mipFB->Bind();
  m_mipFB->AttachExternalColorTexture(mMipChain[0].texture, 0);
  m_mipFB->UnBind();

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

void BloomBuffer::CompositeBloomOver()
{
  m_hdrFB->Bind();
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  finalShader->Bind();

  glBindTextureUnit(0, m_hdrFB->GetColorAttachmentRendererID(0));
  finalShader->SetInt("scene", 0);

  glBindTextureUnit(1, mMipChain[0].texture);
  finalShader->SetInt("bloomBlur", 1);

  renderQuad();

  finalShader->UnBind();
}

void BloomBuffer::BlitColorFrom(const std::shared_ptr<FrameBuffer>& src, uint32_t attachmentIndex)
{
  glBindFramebuffer(GL_READ_FRAMEBUFFER, src->GetID());
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_hdrFB->GetID());

  glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
  glDrawBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex); 

  const auto& srcSpec = src->GetSpecification();
  const auto& dstSpec = m_hdrFB->GetSpecification();

  glBlitFramebuffer(
      0, 0, srcSpec.Width, srcSpec.Height,
      0, 0, dstSpec.Width, dstSpec.Height,
      GL_COLOR_BUFFER_BIT,
      GL_LINEAR
  );

  glReadBuffer(GL_COLOR_ATTACHMENT0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
}

void BloomBuffer::BlitColorTo(const std::shared_ptr<FrameBuffer>& dst)
{
  const auto& srcSpec = m_hdrFB->GetSpecification();
  const auto& dstSpec = dst->GetSpecification();

  glNamedFramebufferReadBuffer(m_hdrFB->GetID(), GL_COLOR_ATTACHMENT0);

  glBlitNamedFramebuffer(
      m_hdrFB->GetID(), dst->GetID(),
      0, 0, srcSpec.Width, srcSpec.Height,
      0, 0, dstSpec.Width, dstSpec.Height,
      GL_COLOR_BUFFER_BIT,
      GL_LINEAR
  );
}

std::shared_ptr<BloomBuffer> BloomBuffer::Create(const std::shared_ptr<Shader>& downsampleShader, const std::shared_ptr<Shader>& upsampleShader, const std::shared_ptr<Shader>& finalShader)
{
  return std::make_shared<BloomBuffer>(downsampleShader,upsampleShader,finalShader);
}

DirectShadowBuffer::DirectShadowBuffer(float shadowWidth, float shadowHeight, float offsetSize, float filterSize, float randomRadius) : m_shadowWidth(shadowWidth), m_shadowHeight(shadowHeight)
{
  glCreateTextures(GL_TEXTURE_2D, 1, &m_depthMap);
  glTextureStorage2D(m_depthMap, 1, GL_DEPTH_COMPONENT32F, shadowWidth, shadowHeight);
  glTextureParameteri(m_depthMap, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTextureParameteri(m_depthMap, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTextureParameteri(m_depthMap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTextureParameteri(m_depthMap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
  glTextureParameterfv(m_depthMap, GL_TEXTURE_BORDER_COLOR, borderColor);

  glCreateFramebuffers(1, &m_FBO);
  glNamedFramebufferTexture(m_FBO, GL_DEPTH_ATTACHMENT, m_depthMap, 0);
  glNamedFramebufferDrawBuffer(m_FBO, GL_NONE);
  glNamedFramebufferReadBuffer(m_FBO, GL_NONE);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  std::vector<float> Data;
  int BufferSize = offsetSize * offsetSize * filterSize * filterSize * 2;
  Data.resize(BufferSize);

  float PI = std::numbers::pi_v<float>;
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

  int32_t NumFilterSamples = filterSize * filterSize;
  
  glCreateTextures(GL_TEXTURE_3D, 1, &m_offsetTexture);
  glTextureStorage3D(m_offsetTexture, 1, GL_RGBA32F, NumFilterSamples / 2, offsetSize, offsetSize);
  glTextureSubImage3D(m_offsetTexture, 0, 0, 0, 0, NumFilterSamples / 2, offsetSize, offsetSize,GL_RGBA, GL_FLOAT, Data.data());
  glTextureParameteri(m_offsetTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTextureParameteri(m_offsetTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  float near_plane = 0.01f, far_plane = 100.0f, orthoSize = 50.0f;
  m_shadowProj = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, near_plane, far_plane);

  struct UBOData {
      glm::vec2 windowSize;
      glm::vec2 offsetSize_filterSize;
      glm::vec2 randomRadius;
  } uboData;

  uboData.windowSize = glm::vec2(shadowWidth, shadowHeight);
  uboData.offsetSize_filterSize = glm::vec2(offsetSize, filterSize);
  uboData.randomRadius = glm::vec2(randomRadius, 0.0f);

  buffer = UniformBuffer::Create(sizeof(UBOData), 2);
  buffer->SetData(&uboData, sizeof(UBOData));
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
  glBindTextureUnit(textureUnit - GL_TEXTURE0, m_depthMap);
}

void DirectShadowBuffer::BindOffsetTextureForReading(GLenum textureUnit) const
{
  glBindTextureUnit(textureUnit - GL_TEXTURE0, m_offsetTexture);
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

  glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, &m_depthCubemapArray);
  glTextureStorage3D(m_depthCubemapArray, 1, GL_R16F, shadowWidth, shadowHeight, 6 * 20);

  glTextureParameteri(m_depthCubemapArray, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTextureParameteri(m_depthCubemapArray, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTextureParameteri(m_depthCubemapArray, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTextureParameteri(m_depthCubemapArray, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTextureParameteri(m_depthCubemapArray, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

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

void OmniDirectShadowBuffer::BindCubemapFaceForWriting(uint32_t cubemapIndex, uint32_t faceIndex)
{
  glNamedFramebufferTextureLayer(m_testFB->GetID(), GL_COLOR_ATTACHMENT0, m_depthCubemapArray, 0, cubemapIndex * 6 + faceIndex);
  glNamedFramebufferDrawBuffer(m_testFB->GetID(), GL_COLOR_ATTACHMENT0);
}

void OmniDirectShadowBuffer::BindShadowTextureForReading(GLenum textureUnit)
{
  glBindTextureUnit(textureUnit - GL_TEXTURE0, m_depthCubemapArray);
}

void OmniDirectShadowBuffer::UnBind() const 
{
  m_testFB->UnBind();
}

std::shared_ptr<OmniDirectShadowBuffer> OmniDirectShadowBuffer::Create(uint32_t shadowWidth, uint32_t shadowHeight)
{
  return std::make_shared<OmniDirectShadowBuffer>(shadowWidth, shadowHeight);
}

