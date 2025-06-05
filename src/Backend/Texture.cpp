#include "Texture.h"
#include "BackendLogger.h"
#include <stb_image.h>

namespace Utils {

	static GLenum ImageFormatToGLDataFormat(ImageFormat format)
	{
		switch (format)
		{
			case ImageFormat::RGB8:  return GL_RGB;
			case ImageFormat::RGBA8: return GL_RGBA;
		}

		/*GABGL_ASSERT(false);*/
		return 0;
	}

	static GLenum ImageFormatToGLInternalFormat(ImageFormat format)
	{
		switch (format)
		{
			case ImageFormat::RGB8:  return GL_RGB8;
			case ImageFormat::RGBA8: return GL_RGBA8;
		}

		/*GABGL_ASSERT(false);*/
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

Texture::Texture(const std::string& path, bool isGL) : m_Path(path)
{
  int width, height, channels;
  stbi_set_flip_vertically_on_load(1);
  stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

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

      if (isGL)
      {
          GABGL_ASSERT(internalFormat & dataFormat, "Format not supported!");

          glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
          glTextureStorage2D(m_RendererID, 1, internalFormat, m_Width, m_Height);

          glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
          glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

          glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
          glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

          glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, dataFormat, GL_UNSIGNED_BYTE, data);
      }

      stbi_image_free(data);
  }
}

Texture::Texture(const std::vector<std::string>& faces)
{
  GABGL_ASSERT(faces.size() == 6, "Cubemap must have exactly 6 faces!");

  stbi_set_flip_vertically_on_load(false); 

  for (int i = 0; i < 6; ++i)
  {
      int w, h, c;
      unsigned char* data = stbi_load(faces[i].c_str(), &w, &h, &c, 0);
      if (!data)
      {
          GABGL_ERROR("Failed to load cubemap face: " + faces[i]);
          continue;
      }
      if (i == 0)
      {
          m_Width = w;
          m_Height = h;
          channels = c;
      }
      else if (w != m_Width || h != m_Height || c != channels)
      {
          stbi_image_free(data);
          GABGL_ERROR("Cubemap face size or channels mismatch: " + faces[i]);
          continue;
      }

      pixels[i] = data;
  }

  stbi_set_flip_vertically_on_load(true); 
}

Texture::~Texture()
{
  if (m_OwnsTexture && m_RendererID != 0) glDeleteTextures(1, &m_RendererID);
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

std::shared_ptr<Texture> Texture::WrapExisting(uint32_t rendererID)
{
  if (rendererID == 0)
  {
      GABGL_ERROR("ID IS NULL");
      return nullptr;
  }

  std::shared_ptr<Texture> texture(new Texture());
  texture->m_RendererID = rendererID;
  texture->m_InternalFormat = GL_RED;
  texture->m_DataFormat = GL_RED;
  texture->m_IsLoaded = true;
  texture->m_OwnsTexture = false; // <== Prevent deletion

  return texture;
}

std::shared_ptr<Texture> Texture::CreateGL(const TextureSpecification& specification)
{
	return std::make_shared<Texture>(specification);
}

std::shared_ptr<Texture> Texture::CreateGL(const std::string& path)
{
	return std::make_shared<Texture>(path,true);
}

std::shared_ptr<Texture> Texture::CreateRAW(const std::string& path)
{
	return std::make_shared<Texture>(path,false);
}

std::shared_ptr<Texture> Texture::CreateCubemap(const std::vector<std::string>& faces)
{
	return std::make_shared<Texture>(faces);
}
