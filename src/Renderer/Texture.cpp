#include "Texture.h"
#include "../Backend/BackendLogger.h"
#include <stb_image.h>

namespace Utils {

	static GLenum ImageFormatToGLDataFormat(ImageFormat format)
	{
		switch (format)
		{
			case ImageFormat::RGB8:  return GL_RGB;
			case ImageFormat::RGBA8: return GL_RGBA;
		}

		GABGL_ASSERT(false);
		return 0;
	}

	static GLenum ImageFormatToGLInternalFormat(ImageFormat format)
	{
		switch (format)
		{
			case ImageFormat::RGB8:  return GL_RGB8;
			case ImageFormat::RGBA8: return GL_RGBA8;
		}

		GABGL_ASSERT(false);
		return 0;
	}

}

Texture::Texture(const TextureSpecification& specification)
	: m_Specification(specification), m_Width(m_Specification.Width), m_Height(m_Specification.Height)
{
	m_InternalFormat = Utils::ImageFormatToGLInternalFormat(m_Specification.Format);
	m_DataFormat = Utils::ImageFormatToGLDataFormat(m_Specification.Format);

	glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
	glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

	glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

Texture::Texture(const std::string& path)
	: m_Path(path)
{
	int width, height, channels;
	stbi_set_flip_vertically_on_load(1);
	stbi_uc* data = nullptr;
	{
		data = stbi_load(path.c_str(), &width, &height, &channels, 0);
	}

	if (data)
	{
		m_RawData = data;
		m_IsLoaded = true;

		m_Width = width;
		m_Height = height;

		GLenum internalFormat = 0, dataFormat = 0;
		if (channels == 4)
		{
			internalFormat = GL_RGBA8;
			dataFormat = GL_RGBA;
		}
		else if (channels == 3)
		{
			internalFormat = GL_RGB8;
			dataFormat = GL_RGB;
		}

		m_InternalFormat = internalFormat;
		m_DataFormat = dataFormat;

		GABGL_ASSERT(internalFormat & dataFormat, "Format not supported!");

		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
		glTextureStorage2D(m_RendererID, 1, internalFormat, m_Width, m_Height);

		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, dataFormat, GL_UNSIGNED_BYTE, data);

		stbi_image_free(data);
	}
}

Texture::~Texture()
{
	glDeleteTextures(1, &m_RendererID);
}

void Texture::SetData(void* data, uint32_t size)
{
	uint32_t bpp = m_DataFormat == GL_RGBA ? 4 : 3;
	GABGL_ASSERT(size == m_Width * m_Height * bpp, "Data must be entire texture!");
	glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
}

void Texture::Bind(uint32_t slot) const
{
	glBindTextureUnit(slot, m_RendererID);
}

Ref<Texture> Texture::Create(const TextureSpecification& specification)
{
	return CreateRef<Texture>(specification);
}

Ref<Texture> Texture::Create(const std::string& path)
{
	return CreateRef<Texture>(path);
}
