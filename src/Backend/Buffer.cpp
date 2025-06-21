#include "Buffer.h"
#include "BackendLogger.h"

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
    return glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, m_Size,
        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
}

void PixelBuffer::Unmap()
{
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    // Insert a fence sync after upload for later wait
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

std::shared_ptr<FrameBuffer> FrameBuffer::Create(const FramebufferSpecification& spec)
{
	return std::make_shared<FrameBuffer>(spec);
}

void FrameBuffer::Blit(const std::shared_ptr<FrameBuffer>& src, const std::shared_ptr<FrameBuffer>& dst)
{
  glBindFramebuffer(GL_READ_FRAMEBUFFER, src->GetID());
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->GetID());

  glReadBuffer(GL_COLOR_ATTACHMENT0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);

  const auto& srcSpec = src->GetSpecification();
  const auto& dstSpec = dst->GetSpecification();

  glBlitFramebuffer(
      0, 0, srcSpec.Width, srcSpec.Height,
      0, 0, dstSpec.Width, dstSpec.Height,
      GL_COLOR_BUFFER_BIT, GL_NEAREST
  );

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

BloomBuffer::BloomBuffer(const std::shared_ptr<Shader>& downsampleShader, const std::shared_ptr<Shader>& upsampleShader, const std::shared_ptr<Shader>& finalShader) : downsampleShader(downsampleShader), upsampleShader(upsampleShader), finalShader(finalShader) {};
BloomBuffer::~BloomBuffer() { Destroy(); }

bool BloomBuffer::Init(unsigned int windowWidth, unsigned int windowHeight, unsigned int mipChainLength)
{
    if (mInit) return true;

    srcViewportSize = glm::ivec2(windowWidth, windowHeight);
    srcViewportSizeFloat = glm::vec2((float)windowWidth, (float)windowHeight);

    // HDR framebuffer setup
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }

    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, windowWidth, windowHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "HDR Framebuffer not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Ping-pong framebuffers for blurring
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Pingpong framebuffer not complete!" << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return false;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Create bloom mip chain FBO and textures
    glGenFramebuffers(1, &bloomFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO);

    glm::vec2 mipSize((float)windowWidth, (float)windowHeight);
    glm::ivec2 mipIntSize(windowWidth, windowHeight);

    mipChain.clear();
    for (unsigned int i = 0; i < mipChainLength; i++)
    {
        mipSize *= 0.5f;
        mipIntSize /= 2;

        bloomMip mip;
        mip.size = mipSize;
        mip.intSize = mipIntSize;

        glGenTextures(1, &mip.texture);
        glBindTexture(GL_TEXTURE_2D, mip.texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F,
                     (int)mipSize.x, (int)mipSize.y, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        mipChain.push_back(mip);
    }

    // Attach first mip texture to bloom framebuffer color attachment 0
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mipChain[0].texture, 0);
    GLuint bloomAttachments[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, bloomAttachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Bloom framebuffer not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Setup shaders' texture unit bindings
    downsampleShader->Use();
    downsampleShader->setInt("srcTexture", 0);
    upsampleShader->Use();
    upsampleShader->setInt("srcTexture", 0);
    glUseProgram(0);

    mInit = true;
    return true;
}

void BloomBuffer::Destroy()
{
    if (!mInit) return;

    // Delete bloom mip textures
    for (auto& mip : mipChain)
    {
        if (mip.texture)
        {
            glDeleteTextures(1, &mip.texture);
            mip.texture = 0;
        }
    }
    mipChain.clear();

    // Delete bloom FBO
    if (bloomFBO)
    {
        glDeleteFramebuffers(1, &bloomFBO);
        bloomFBO = 0;
    }

    // Delete HDR color buffers
    for (int i = 0; i < 2; i++)
    {
        if (colorBuffers[i])
        {
            glDeleteTextures(1, &colorBuffers[i]);
            colorBuffers[i] = 0;
        }
    }

    // Delete ping-pong buffers and FBOs
    for (int i = 0; i < 2; i++)
    {
        if (pingpongColorbuffers[i])
        {
            glDeleteTextures(1, &pingpongColorbuffers[i]);
            pingpongColorbuffers[i] = 0;
        }
        if (pingpongFBO[i])
        {
            glDeleteFramebuffers(1, &pingpongFBO[i]);
            pingpongFBO[i] = 0;
        }
    }

    // Delete HDR FBO and renderbuffer
    if (hdrFBO)
    {
        glDeleteFramebuffers(1, &hdrFBO);
        hdrFBO = 0;
    }
    if (rboDepth)
    {
        glDeleteRenderbuffers(1, &rboDepth);
        rboDepth = 0;
    }

    // Delete quad buffers
    if (quadVBO)
    {
        glDeleteBuffers(1, &quadVBO);
        quadVBO = 0;
    }
    if (quadVAO)
    {
        glDeleteVertexArrays(1, &quadVAO);
        quadVAO = 0;
    }

    mInit = false;
}

void BloomBuffer::Resize(unsigned int newWidth, unsigned int newHeight)
{
    if (!mInit) return;

    srcViewportSize = glm::ivec2(newWidth, newHeight);
    srcViewportSizeFloat = glm::vec2((float)newWidth, (float)newHeight);

    // Resize HDR framebuffer textures
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, newWidth, newHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
    }

    // Resize depth renderbuffer
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, newWidth, newHeight);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete after resizing!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Resize ping-pong textures
    for (int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, newWidth, newHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
    }

    // Resize bloom mip chain textures
    glm::vec2 mipSize((float)newWidth, (float)newHeight);
    glm::ivec2 mipIntSize(newWidth, newHeight);

    for (auto& mip : mipChain)
    {
        mipSize *= 0.5f;
        mipIntSize /= 2;

        mip.size = mipSize;
        mip.intSize = mipIntSize;

        glBindTexture(GL_TEXTURE_2D, mip.texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F,
                     (int)mipSize.x, (int)mipSize.y, 0, GL_RGB, GL_FLOAT, nullptr);
    }

    // Reattach first bloom mip to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mipChain[0].texture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void BloomBuffer::Bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
}

void BloomBuffer::UnBind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, srcViewportSize.x, srcViewportSize.y);
}

GLuint BloomBuffer::getBloomTexture() const
{
    return mipChain.empty() ? 0 : mipChain[0].texture;
}

GLuint BloomBuffer::BloomMip_i(int index) const
{
    if (index < 0 || index >= (int)mipChain.size()) return 0;
    return mipChain[index].texture;
}

void BloomBuffer::renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions   // texCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
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
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void BloomBuffer::RenderDownsamples(GLuint srcTexture)
{
    // Downsample from srcTexture through mipChain
    downsampleShader->Use();

    int mipCount = (int)mipChain.size();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, srcTexture);

    for (int i = 0; i < mipCount; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mipChain[i].texture, 0);

        glViewport(0, 0, mipChain[i].intSize.x, mipChain[i].intSize.y);
        glClear(GL_COLOR_BUFFER_BIT);

        glm::vec2 srcSize = (i == 0) ? srcViewportSizeFloat : mipChain[i - 1].size;

        downsampleShader->setVec2("srcSize", srcSize);
        downsampleShader->setInt("KarisAverage", mKarisAverageOnDownsample ? 1 : 0);

        renderQuad();

        // Bind current mip texture for next iteration
        glBindTexture(GL_TEXTURE_2D, mipChain[i].texture);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void BloomBuffer::RenderUpsamples(float filterRadius)
{
    upsampleShader->Use();
    int mipCount = (int)mipChain.size();

    // Upsample starting from smallest mip, blend progressively into higher mips
    for (int i = mipCount - 2; i >= 0; i--)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mipChain[i].texture, 0);

        glViewport(0, 0, mipChain[i].intSize.x, mipChain[i].intSize.y);
        glClear(GL_COLOR_BUFFER_BIT);

        // Bind lower mip as source texture to upsample
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mipChain[i + 1].texture);

        upsampleShader->setFloat("filterRadius", filterRadius);
        upsampleShader->setVec2("srcSize", mipChain[i + 1].size);

        renderQuad();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, srcViewportSize.x, srcViewportSize.y);
}

void BloomBuffer::RenderBloomTexture(float filterRadius)
{
    // Combine downsampling and upsampling passes to create bloom mip chain
    RenderDownsamples(colorBuffers[1]); // assuming colorBuffers[1] holds bright parts
    RenderUpsamples(filterRadius);
}

void BloomBuffer::Render()
{
    // Bind default framebuffer for final render
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    finalShader->Use();

    // Bind HDR color buffer 0 (original scene)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
    finalShader->setInt("sceneTexture", 0);

    // Bind bloom texture (first mip in chain)
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mipChain[0].texture);
    finalShader->setInt("bloomTexture", 1);

    renderQuad();
}
