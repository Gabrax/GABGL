#pragma once

#include "../Backend/BackendScopeRef.h"
#include <string>
#include <glad/glad.h>

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
	Texture(const TextureSpecification& specification);
	Texture(const std::string& path);
	~Texture();

	inline const TextureSpecification& GetSpecification() const { return m_Specification; }

	inline uint32_t GetWidth() const { return m_Width; }
	inline uint32_t GetHeight() const { return m_Height; }
	inline uint32_t GetRendererID() const { return m_RendererID; }
	inline const uint8_t* GetRawData() const { return m_RawData; }
	inline const std::string& GetPath() const { return m_Path; }
	void SetData(void* data, uint32_t size);
	void Bind(uint32_t slot = 0) const;
	inline bool IsLoaded() const { return m_IsLoaded; }

	bool operator==(const Texture& other) const 
	{
		return m_RendererID == other.GetRendererID();
	}
	static Ref<Texture> Create(const TextureSpecification& specification);
	static Ref<Texture> Create(const std::string& path);
private:
	TextureSpecification m_Specification;

	std::string m_Path;
	bool m_IsLoaded = false;
	uint32_t m_Width, m_Height;
	uint32_t m_RendererID;
	uint8_t* m_RawData = nullptr;
	GLenum m_InternalFormat, m_DataFormat;
};

