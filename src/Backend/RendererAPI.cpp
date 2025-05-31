#include "RendererAPI.h"
#include "Renderer2D.h"
#include "BackendLogger.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

void MessageCallback(
	unsigned source,
	unsigned type,
	unsigned id,
	unsigned severity,
	int length,
	const char* message,
	const void* userParam)
{
	switch (severity)
	{
		case GL_DEBUG_SEVERITY_HIGH:         GABGL_CRITICAL(message); return;
		case GL_DEBUG_SEVERITY_MEDIUM:       GABGL_ERROR(message); return;
		case GL_DEBUG_SEVERITY_LOW:          GABGL_WARN(message); return;
		case GL_DEBUG_SEVERITY_NOTIFICATION: GABGL_TRACE(message); return;
	}

	GABGL_ASSERT(false, "Unknown severity level!");
}

std::unique_ptr<RendererAPI::SceneData> RendererAPI::s_SceneData = std::unique_ptr<RendererAPI::SceneData>();

void RendererAPI::Init()
{

#ifdef DEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(MessageCallback, nullptr);

	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
#endif

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);

	Renderer::Init();
}

void RendererAPI::Shutdown()
{
	Renderer::Shutdown();
}

void RendererAPI::OnWindowResize(uint32_t width, uint32_t height)
{
	SetViewport(0, 0, width, height);
}

void RendererAPI::BeginScene(Camera& camera)
{
	s_SceneData->ViewProjectionMatrix = camera.GetViewProjection();
}

void RendererAPI::EndScene()
{

}

void RendererAPI::Submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vertexArray, const glm::mat4& transform)
{
	shader->Use();
	shader->setMat4("u_ViewProjection", s_SceneData->ViewProjectionMatrix);
	shader->setMat4("u_Transform", transform);

	vertexArray->Bind();
	DrawIndexed(vertexArray);
}

void RendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	glViewport(x, y, width, height);
}

void RendererAPI::SetClearColor(const glm::vec4& color)
{
	glClearColor(color.r, color.g, color.b, color.a);
}

void RendererAPI::Clear()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RendererAPI::DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount)
{
	vertexArray->Bind();
	uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
	glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
}

void RendererAPI::DrawLines(const std::shared_ptr<VertexArray>& vertexArray, uint32_t vertexCount)
{
	vertexArray->Bind();
	glDrawArrays(GL_LINES, 0, vertexCount);
}

void RendererAPI::SetLineWidth(float width)
{
	glLineWidth(width);
}
