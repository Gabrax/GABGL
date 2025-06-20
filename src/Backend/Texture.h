#pragma once

#include <memory>
#include <string>
#include <glad/glad.h>
#include <vector>
#include <array>
#include <assimp/scene.h>

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
  Texture(const std::string& filename, const std::string& directory, bool isGL);
  Texture(const aiTexture* paiTexture, const std::string& path, bool isGL);
  Texture(const std::vector<std::string>& faces);
	~Texture();

	bool operator==(const Texture& other) const 
	{
		return m_RendererID == other.GetRendererID();
	}

	void SetData(void* data, uint32_t size);
	void Bind(uint32_t slot = 0) const;
	inline const TextureSpecification& GetSpecification() const { return m_Specification; }
	inline uint32_t GetWidth() const { return m_Width; }
	inline uint32_t GetHeight() const { return m_Height; }
  inline void SetRendererID(uint32_t id) { m_RendererID = id; }
	inline uint32_t GetRendererID() const { return m_RendererID; }
  inline uint32_t& GetRendererID() { return m_RendererID; }
	inline const uint8_t* GetRawData() const { return m_RawData; }
  inline void ClearRawData() { delete[] m_RawData; m_RawData = nullptr; }
	inline const std::string& GetPath() const { return m_Path; }
  inline const GLenum GetDataFormat() const { return m_DataFormat; }
  inline const GLenum GetInternalFormat() const { return m_InternalFormat; }
  inline void SetType(const std::string& type) { m_Type = type; }
  inline std::string& GetType() { return m_Type; }
  inline bool IsUnCompressed() { return m_IsEmbeddedUnCompressed; }
  inline const aiTexture* GetEmbeddedTexture() { return paiTexture; }
	inline bool IsLoaded() const { return m_IsLoaded; }

	static std::shared_ptr<Texture> Create(const TextureSpecification& specification);
	static std::shared_ptr<Texture> Create(const std::string& path);
	static std::shared_ptr<Texture> CreateRAW(const std::string& path);
  static std::shared_ptr<Texture> CreateRAW(const std::string& path, const std::string& directory);
  static std::shared_ptr<Texture> CreateRAWEMBEDDED(const aiTexture* paiTexture, const std::string& directory);
  static std::shared_ptr<Texture> CreateEMBEDDED(const aiTexture* paiTexture, const std::string& directory);
  static std::shared_ptr<Texture> CreateRAWCUBEMAP(const std::vector<std::string>& faces);
  static std::shared_ptr<Texture> CreateCUBEMAP(const std::vector<std::string>& faces);
  static std::shared_ptr<Texture> WrapExisting(uint32_t rendererID);

  inline std::array<unsigned char*, 6> GetPixels() { return pixels; }
  inline int32_t GetChannels() { return channels; }

private:

  void FlipImageVertically(unsigned char* data, int width, int height, int channels);

	TextureSpecification m_Specification;

  std::array<unsigned char*, 6> pixels;
  int32_t channels;

  const aiTexture* paiTexture;

	std::string m_Path;
	std::string m_Directory;
  std::string m_Type;
	bool m_IsLoaded = false;
	bool m_IsEmbeddedUnCompressed = false;
  bool m_OwnsTexture = true;
	uint32_t m_Width, m_Height;
	uint32_t m_RendererID;
	uint8_t* m_RawData = nullptr;
	GLenum m_InternalFormat, m_DataFormat;
};

