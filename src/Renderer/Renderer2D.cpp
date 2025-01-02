#include "Renderer2D.h"
#include "../Backend/BackendLogger.h"
#include "../Engine.h"
#include "RendererAPI.h"

struct Shaders2D
{
	Shader basic;

} g_2Dshaders;

struct Shaders3D
{
	Shader basic;

} g_3Dshaders;

Renderer2D* Renderer2D::s_Renderer = nullptr;

Renderer2D::Renderer2D() : Layer("Renderer")
{
	s_Renderer = this;
	Load3DShaders();
	Load2DShaders();
}

void Renderer2D::OnAttach()
{
	FramebufferSpecification fb;
	fb.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
	fb.Width = Engine::GetInstance().GetMainWindow().GetWidth();
	fb.Height = Engine::GetInstance().GetMainWindow().GetHeight();
	m_FrameBuffer = Framebuffer::Create(fb);

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// Generate and bind VBO
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	// Define vertex attributes
	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Texture coordinate attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Unbind VAO (optional)
	glBindVertexArray(0);
}

void Renderer2D::OnDetach()
{
}

void Renderer2D::OnUpdate(DeltaTime dt)
{
	m_FrameBuffer->Bind();
	RendererAPI::Clear();
	Render3D();
	Render2D();
	m_FrameBuffer->Unbind();
}

void Renderer2D::OnEvent(Event& e)
{
}

void Renderer2D::Render3D()
{
	GABGL_PROFILE_SCOPE("Renderer3D");
}

void Renderer2D::Render2D()
{
	GABGL_PROFILE_SCOPE("Renderer2D");

	g_2Dshaders.basic.Use();
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Renderer2D::Load3DShaders()
{
}

void Renderer2D::Load2DShaders()
{
	g_2Dshaders.basic.Load("../res/shaders/2D.glsl");
}
