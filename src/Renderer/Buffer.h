#pragma once
#include <cstdint>
#include "../Backend/BackendLogger.h"
#include "../Backend/BackendScopeRef.h"
#include <string>
#include <vector>

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
	inline static Ref<VertexBuffer> Create(uint32_t size) { return CreateRef<VertexBuffer>(size); }
	inline static Ref<VertexBuffer> Create(float* vertices, uint32_t size) { return CreateRef<VertexBuffer>(vertices, size); }
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
	inline static Ref<IndexBuffer> Create(uint32_t* indices, uint32_t count) { return CreateRef<IndexBuffer>(indices,count); }
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

	void AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer);
	void SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer);

	inline const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const { return m_VertexBuffers; }
	inline const Ref<IndexBuffer>& GetIndexBuffer() const { return m_IndexBuffer; }
	inline static Ref<VertexArray> Create() { return CreateRef<VertexArray>(); }
private:
	uint32_t m_RendererID;
	uint32_t m_VertexBufferIndex = 0;
	std::vector<Ref<VertexBuffer>> m_VertexBuffers;
	Ref<IndexBuffer> m_IndexBuffer;
};

struct UniformBuffer 
{
	UniformBuffer(uint32_t size, uint32_t binding);
	virtual ~UniformBuffer();

	void SetData(const void* data, uint32_t size, uint32_t offset = 0);
	inline static Ref<UniformBuffer> Create(uint32_t size, uint32_t binding) { return CreateRef<UniformBuffer>(size, binding); }
private:
	uint32_t m_RendererID = 0;
};
