#include "Renderer.h"
#include "RendererAPI.h"
#include "Renderer2D.h"

Scope<Renderer::SceneData> Renderer::s_SceneData = CreateScope<Renderer::SceneData>();

void Renderer::Init()
{
	RendererAPI::Init();
	Renderer2D::Init();
}

void Renderer::Shutdown()
{
	Renderer2D::Shutdown();
}

void Renderer::OnWindowResize(uint32_t width, uint32_t height)
{
	RendererAPI::SetViewport(0, 0, width, height);
}

void Renderer::BeginScene(EditorCamera& camera)
{
	s_SceneData->ViewProjectionMatrix = camera.GetViewProjection();
}

void Renderer::EndScene()
{
}

void Renderer::Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform)
{
	shader->Use();
	shader->setMat4("u_ViewProjection", s_SceneData->ViewProjectionMatrix);
	shader->setMat4("u_Transform", transform);

	vertexArray->Bind();
	RendererAPI::DrawIndexed(vertexArray);
}