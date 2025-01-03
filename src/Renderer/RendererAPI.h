#pragma once
#include "Buffer.h"
#include <glm/glm.hpp>

struct RendererAPI
{
	static void Init();
	static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

	static void SetClearColor(const glm::vec4& color);
	static void Clear();

	static void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0);
	static void DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount);

	static void SetLineWidth(float width);
};