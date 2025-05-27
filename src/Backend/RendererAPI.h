#pragma once

#include <memory>

#include "Shader.h"
#include "Buffer.h"
#include "Camera.h"
#include "Buffer.h"
#include <glm/glm.hpp>

struct RendererAPI
{
	static void Init();
	static void Shutdown();

	static void OnWindowResize(uint32_t width, uint32_t height);

	static void BeginScene(Camera& camera);
	static void EndScene();

	static void Submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vertexArray, const glm::mat4& transform = glm::mat4(1.0f));

	static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

	static void SetClearColor(const glm::vec4& color);
	static void Clear();

	static void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0);
	static void DrawLines(const std::shared_ptr<VertexArray>& vertexArray, uint32_t vertexCount);

	static void SetLineWidth(float width);


private:
	struct SceneData
	{
		glm::mat4 ViewProjectionMatrix;
	};

	static std::unique_ptr<SceneData> s_SceneData;
};
