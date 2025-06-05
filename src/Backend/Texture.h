#pragma once

#include <initializer_list>
#include <memory>
#include <string>
#include <glad/glad.h>
#include <vector>
#include <array>

enum class ImageFormat
{
	None = 0,
	R8,
	RGB8,
	RGBA8,
	RGBA32F
};

struct TextureSpecification
{
	uint32_t Width = 1;
	uint32_t Height = 1;
	ImageFormat Format = ImageFormat::RGBA8;
	bool GenerateMips = true;
};

struct Texture 
{
  Texture() = default;
	Texture(const TextureSpecification& specification);
  Texture(const std::string& path, bool isGL);
  Texture(const std::vector<std::string>& faces);
	~Texture();

	inline const TextureSpecification& GetSpecification() const { return m_Specification; }

	inline uint32_t GetWidth() const { return m_Width; }
	inline uint32_t GetHeight() const { return m_Height; }
  inline void SetRendererID(uint32_t id) { m_RendererID = id; }
	inline uint32_t GetRendererID() const { return m_RendererID; }
  inline uint32_t& GetRendererID() { return m_RendererID; }
	inline const uint8_t* GetRawData() const { return m_RawData; }
	inline const std::string& GetPath() const { return m_Path; }
	void SetData(void* data, uint32_t size);
	void Bind(uint32_t slot = 0) const;
	inline bool IsLoaded() const { return m_IsLoaded; }

	bool operator==(const Texture& other) const 
	{
		return m_RendererID == other.GetRendererID();
	}
	static std::shared_ptr<Texture> CreateGL(const TextureSpecification& specification);
	static std::shared_ptr<Texture> CreateGL(const std::string& path);
	static std::shared_ptr<Texture> CreateRAW(const std::string& path);
  static std::shared_ptr<Texture> WrapExisting(uint32_t rendererID);
  static std::shared_ptr<Texture> CreateCubemap(const std::vector<std::string>& faces);

  inline std::array<unsigned char*, 6> GetPixels() { return pixels; }
  inline int32_t GetChannels() { return channels; }

private:
	TextureSpecification m_Specification;

  std::array<unsigned char*, 6> pixels;
  int32_t channels;

	std::string m_Path;
	bool m_IsLoaded = false;
  bool m_OwnsTexture = true;
	uint32_t m_Width, m_Height;
	uint32_t m_RendererID;
	uint8_t* m_RawData = nullptr;
	GLenum m_InternalFormat, m_DataFormat;
};

