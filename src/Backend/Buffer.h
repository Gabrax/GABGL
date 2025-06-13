#pragma once
#include <cstdint>
#include "BackendLogger.h"
#include <string>
#include <vector>
#include <glad/glad.h>

enum class ShaderDataType
{
	None = 0, Float, Float2, Float3, Float4, Mat3, Mat4, Int, Int2, Int3, Int4, Bool
};

static uint32_t ShaderDataTypeSize(ShaderDataType type)
{
	switch (type)
	{
		case ShaderDataType::Float:    return 4;
		case ShaderDataType::Float2:   return 4 * 2;
		case ShaderDataType::Float3:   return 4 * 3;
		case ShaderDataType::Float4:   return 4 * 4;
		case ShaderDataType::Mat3:     return 4 * 3 * 3;
		case ShaderDataType::Mat4:     return 4 * 4 * 4;
		case ShaderDataType::Int:      return 4;
		case ShaderDataType::Int2:     return 4 * 2;
		case ShaderDataType::Int3:     return 4 * 3;
		case ShaderDataType::Int4:     return 4 * 4;
		case ShaderDataType::Bool:     return 1;
	}

	GABGL_ASSERT(false, "Unknown ShaderDataType!");
	return 0;
}

struct BufferElement
{
	std::string Name;
	ShaderDataType Type;
	uint32_t Size;
	size_t Offset;
	bool Normalized;

	BufferElement() = default;

	BufferElement(ShaderDataType type, const std::string& name, bool normalized = false)
		: Name(name), Type(type), Size(ShaderDataTypeSize(type)), Offset(0), Normalized(normalized)
	{
	}

	uint32_t GetComponentCount() const
	{
		switch (Type)
		{
		case ShaderDataType::Float:   return 1;
		case ShaderDataType::Float2:  return 2;
		case ShaderDataType::Float3:  return 3;
		case ShaderDataType::Float4:  return 4;
		case ShaderDataType::Mat3:    return 3; // 3* float3
		case ShaderDataType::Mat4:    return 4; // 4* float4
		case ShaderDataType::Int:     return 1;
		case ShaderDataType::Int2:    return 2;
		case ShaderDataType::Int3:    return 3;
		case ShaderDataType::Int4:    return 4;
		case ShaderDataType::Bool:    return 1;
		}

		GABGL_ASSERT(false, "Unknown ShaderDataType!");
		return 0;
	}
};

struct BufferLayout
{
	BufferLayout() {}

	BufferLayout(std::initializer_list<BufferElement> elements)
		: m_Elements(elements)
	{
		CalculateOffsetsAndStride();
	}

	inline uint32_t GetStride() const { return m_Stride; }
	inline const std::vector<BufferElement>& GetElements() const { return m_Elements; }

	inline std::vector<BufferElement>::iterator begin() { return m_Elements.begin(); }
	inline std::vector<BufferElement>::iterator end() { return m_Elements.end(); }
	inline std::vector<BufferElement>::const_iterator begin() const { return m_Elements.begin(); }
	inline std::vector<BufferElement>::const_iterator end() const { return m_Elements.end(); }
private:
	void CalculateOffsetsAndStride()
	{
		size_t offset = 0;
		m_Stride = 0;
		for (auto& element : m_Elements)
		{
			element.Offset = offset;
			offset += element.Size;
			m_Stride += element.Size;
		}
	}
private:
	std::vector<BufferElement> m_Elements;
	uint32_t m_Stride = 0;
};

struct VertexBuffer
{
	VertexBuffer(uint32_t size);
	VertexBuffer(float* vertices, uint32_t size);
	virtual ~VertexBuffer();

	void Bind() const;
	void Unbind() const;

	void SetData(const void* data, uint32_t size);

	const BufferLayout& GetLayout() const { return m_Layout; }
	void SetLayout(const BufferLayout& layout) { m_Layout = layout; }
	inline static std::shared_ptr<VertexBuffer> Create(uint32_t size) { return std::make_shared<VertexBuffer>(size); }
	inline static std::shared_ptr<VertexBuffer> Create(float* vertices, uint32_t size) { return std::make_shared<VertexBuffer>(vertices, size); }
private:
	uint32_t m_RendererID;
	BufferLayout m_Layout;
};

struct IndexBuffer
{
	IndexBuffer(uint32_t* indices, uint32_t count);
	virtual ~IndexBuffer();

	void Bind() const;
	void Unbind() const;

	uint32_t GetCount() const { return m_Count; }
	inline static std::shared_ptr<IndexBuffer> Create(uint32_t* indices, uint32_t count) { return std::make_shared<IndexBuffer>(indices,count); }
private:
	uint32_t m_RendererID;
	uint32_t m_Count;
};

struct VertexArray
{
	VertexArray();
	virtual ~VertexArray();

	void Bind() const;
	void Unbind() const;

	void AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer);
	void SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer);

	inline const std::vector<std::shared_ptr<VertexBuffer>>& GetVertexBuffers() const { return m_VertexBuffers; }
	inline const std::shared_ptr<IndexBuffer>& GetIndexBuffer() const { return m_IndexBuffer; }
	inline static std::shared_ptr<VertexArray> Create() { return std::make_shared<VertexArray>(); }
private:
	uint32_t m_RendererID;
	uint32_t m_VertexBufferIndex = 0;
	std::vector<std::shared_ptr<VertexBuffer>> m_VertexBuffers;
  std::shared_ptr<IndexBuffer> m_IndexBuffer;
};

struct UniformBuffer 
{
	UniformBuffer(uint32_t size, uint32_t binding);
	virtual ~UniformBuffer();

	void SetData(const void* data, uint32_t size, uint32_t offset = 0);
	inline static std::shared_ptr<UniformBuffer> Create(uint32_t size, uint32_t binding) { return std::make_shared<UniformBuffer>(size, binding); }
private:
	uint32_t m_RendererID = 0;
};

struct StorageBuffer
{
  StorageBuffer(uint32_t size, uint32_t binding);
  virtual ~StorageBuffer();
  void Allocate(size_t size);
  void SetData(size_t size, void* data);
  void CleanUp();
  void* MapBuffer();
  void UnmapBuffer();
  inline static std::shared_ptr<StorageBuffer> Create(uint32_t size, uint32_t binding) { return std::make_shared<StorageBuffer>(size, binding); }
private:
  uint32_t m_RendererID = 0;
  size_t bufferSize = 0;
};

struct PixelBuffer
{
  explicit PixelBuffer(size_t size);
  ~PixelBuffer();

  void* Map();
  void Unmap();
  void Bind() const;
  void Unbind() const;
  void WaitForCompletion();

  GLuint GetID() const { return m_ID; }
  size_t GetSize() const { return m_Size; }

private:
  PixelBuffer(const PixelBuffer&) = delete;
  PixelBuffer& operator=(const PixelBuffer&) = delete;

  GLuint m_ID = 0;
  size_t m_Size = 0;
  GLsync m_Sync = nullptr;
};
