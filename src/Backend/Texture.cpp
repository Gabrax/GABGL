#include "Texture.h"
#include "Timer.hpp"
#include <gabdebug.h>
#include <stb_image.h>

namespace Utils {

	static GLenum ImageFormatToGLDataFormat(ImageFormat format)
	{
		switch (format)
		{
			case ImageFormat::RGB8:  return GL_RGB;
			case ImageFormat::RGBA8: return GL_RGBA;
		}

		return 0;
	}

	static GLenum ImageFormatToGLInternalFormat(ImageFormat format)
	{
		switch (format)
		{
			case ImageFormat::RGB8:  return GL_RGB8;
			case ImageFormat::RGBA8: return GL_RGBA8;
		}

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

	glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

Texture::Texture(const std::string& path) : m_Path(path)
{
  Timer timer;

  int width, height, sourceChannels;
  stbi_uc* data = stbi_load(path.c_str(), &width, &height, &sourceChannels, STBI_rgb_alpha);

  if (data)
  {
      constexpr int channels = 4;
      FlipImageVertically(data, width, height, channels);
      m_RawData = new uint8_t[width * height * channels];
      memcpy(m_RawData, data, width * height * channels);
      m_IsLoaded = true;

      m_Width = width;
      m_Height = height;
      this->channels = channels;

      m_InternalFormat = GL_RGBA8;
      m_DataFormat = GL_RGBA;

      stbi_image_free(data);
      gablog_log(LOG_WARN, __FILE__, __LINE__, "Texture loading took %.3f ms", timer.ElapsedMillis());
  }
  else
  {
    gablog_log(LOG_ERROR, __FILE__, __LINE__, "COUDLNT LOAD TEXTURE!");
  }
}

Texture::Texture(const std::string& path, const std::string& directory) : m_Path(path)
{
  Timer timer;

  std::string filename = directory + '/' + path;
  int width, height, sourceChannels;
  stbi_uc* data = stbi_load(filename.c_str(), &width, &height, &sourceChannels, STBI_rgb_alpha);
  if (data)
  {
      constexpr int channels = 4;
      FlipImageVertically(data, width, height, channels);
      m_RawData = new uint8_t[width * height * channels];
      memcpy(m_RawData, data, width * height * channels);
      m_IsLoaded = true;

      m_Width = width;
      m_Height = height;
      this->channels = channels;

      m_InternalFormat = GL_RGBA8;
      m_DataFormat = GL_RGBA;

      stbi_image_free(data);
      gablog_log(LOG_WARN, __FILE__, __LINE__, "Texture loading took %.3f ms", timer.ElapsedMillis());
  }
  else
  {
    gablog_log(LOG_ERROR, __FILE__, __LINE__, "COUDLNT LOAD TEXTURE!");
  }
}

Texture::Texture(const aiTexture* paiTexture, const std::string& path) : paiTexture(paiTexture), m_Path(path)
{
  if (paiTexture->mHeight == 0)
  {
    int width, height, sourceChannels;
    unsigned char* data = stbi_load_from_memory(
      reinterpret_cast<const unsigned char*>(paiTexture->pcData), paiTexture->mWidth,
      &width, &height, &sourceChannels, STBI_rgb_alpha);

    if (data)
    {
      constexpr int channels = 4;
      FlipImageVertically(data, width, height, channels);
      m_RawData = new uint8_t[width * height * channels];
      memcpy(m_RawData, data, width * height * channels);
      m_IsLoaded = true;

      m_Width = width;
      m_Height = height;
      this->channels = channels;

      m_InternalFormat = GL_RGBA8;
      m_DataFormat = GL_RGBA;

      stbi_image_free(data);
    }
    else
    {
        gablog_log(LOG_ERROR, __FILE__, __LINE__, "Failed to load compressed embedded texture!");
    }
  }
  else
  {
    m_Width = paiTexture->mWidth;
    m_Height = paiTexture->mHeight;
    channels = 4;
    m_RawData = new uint8_t[static_cast<size_t>(m_Width) * m_Height * channels];
    for (size_t i = 0; i < static_cast<size_t>(m_Width) * m_Height; ++i)
    {
      m_RawData[i * 4 + 0] = paiTexture->pcData[i].r;
      m_RawData[i * 4 + 1] = paiTexture->pcData[i].g;
      m_RawData[i * 4 + 2] = paiTexture->pcData[i].b;
      m_RawData[i * 4 + 3] = paiTexture->pcData[i].a;
    }
    FlipImageVertically(m_RawData, static_cast<int>(m_Width), static_cast<int>(m_Height), channels);
    m_IsLoaded = true;
    m_InternalFormat = GL_RGBA8;
    m_DataFormat = GL_RGBA;
  }
}

Texture::Texture(const std::vector<std::string>& faces)
{
  Timer timer;

  if (faces.size() != 6)
  {
    gablog_log(LOG_ASSERT, __FILE__, __LINE__, "Cubemap must have exactly 6 faces!");
    gabdebug_break();
  }

  for (int i = 0; i < 6; ++i)
  {
      int w, h, c;
      unsigned char* data = stbi_load(faces[i].c_str(), &w, &h, &c, 0);
      if (!data)
      {
          gablog_log(LOG_ERROR, __FILE__, __LINE__, "Failed to load cubemap face: %s", faces[i].c_str());
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
          gablog_log(LOG_ERROR, __FILE__, __LINE__, "Cubemap face size or channels mismatch: %s", faces[i].c_str());
          continue;
      }

      pixels[i] = data;
  }
  gablog_log(LOG_WARN, __FILE__, __LINE__, "Texture loading took %.3f ms", timer.ElapsedMillis());
}

Texture::~Texture()
{
  if (m_OwnsTexture && m_RendererID != 0) glDeleteTextures(1, &m_RendererID);
  delete[] m_RawData;
  for (unsigned char*& face : pixels)
  {
    if (face) stbi_image_free(face);
    face = nullptr;
  }
}

void Texture::FlipImageVertically(unsigned char* data, int width, int height, int channels)
{
  int stride = width * channels;
  std::vector<unsigned char> row(stride); // temporary buffer for a row

  for (int y = 0; y < height / 2; ++y)
  {
      unsigned char* rowTop = data + y * stride;
      unsigned char* rowBottom = data + (height - y - 1) * stride;

      // Swap the two rows
      std::memcpy(row.data(), rowTop, stride);
      std::memcpy(rowTop, rowBottom, stride);
      std::memcpy(rowBottom, row.data(), stride);
  }
}

void Texture::SetData(void* data, uint32_t size) const
{
	uint32_t bpp = m_DataFormat == GL_RGBA ? 4 : 3;
	if (size != m_Width * m_Height * bpp)
	{
		gablog_log(LOG_ASSERT, __FILE__, __LINE__, "Data must be entire texture!");
		gabdebug_break();
	}
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
      gablog_log(LOG_ERROR, __FILE__, __LINE__, "ID IS NULL");
      return nullptr;
  }

  std::shared_ptr<Texture> texture(new Texture());
  texture->m_RendererID = rendererID;
  texture->m_InternalFormat = GL_RED;
  texture->m_DataFormat = GL_RED;
  texture->m_IsLoaded = true;
  texture->m_OwnsTexture = false; // Prevent deletion

  return texture;
}

std::shared_ptr<Texture> Texture::Create(const TextureSpecification& specification)
{
	return std::make_shared<Texture>(specification);
}

std::shared_ptr<Texture> Texture::Create(const std::string& path)
{
	return std::make_shared<Texture>(path);
}

std::shared_ptr<Texture> Texture::Create(const std::string& filename, const std::string& directory)
{
	return std::make_shared<Texture>(filename,directory);
}

std::shared_ptr<Texture> Texture::CreateEMBEDDED(const aiTexture* paiTexture, const std::string& path)
{
	return std::make_shared<Texture>(paiTexture,path);
}

std::shared_ptr<Texture> Texture::CreateCUBEMAP(const std::vector<std::string>& faces)
{
	return std::make_shared<Texture>(faces);
}
