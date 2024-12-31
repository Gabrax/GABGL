#pragma once

#include "../Backend/Layer.h"
#include "../Backend/DeltaTime.h"
#include "../Input/EngineEvent.h"
#include "../Input/KeyEvent.h"
#include "Shader.h"
#include "../Backend/BackendScopeRef.h"
#include "FrameBuffer.h"

struct Renderer : Layer
{
	Renderer();
	virtual ~Renderer() = default;

	void OnAttach() override;
	void OnDetach() override;

	void OnUpdate(DeltaTime dt) override;
	void OnEvent(Event& e) override;
	inline static Renderer& GetRenderer() { return *s_Renderer; }
	inline Ref<Framebuffer> GetFrameBuffer() { return m_FrameBuffer; }
	static void Load3DShaders();
	static void Load2DShaders();

private:
	bool OnKeyPressed(KeyPressedEvent& e);
	bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
private:
	void Render3D();
	void Render2D();
private:

	Ref<Framebuffer> m_FrameBuffer;

	float quadVertices[5*6] = {
		// Positions          // Texture Coords
		-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, // Bottom-left
		 0.5f, -0.5f, 0.0f,   1.0f, 0.0f, // Bottom-right
		 0.5f,  0.5f, 0.0f,   1.0f, 1.0f, // Top-right

		-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, // Bottom-left
		 0.5f,  0.5f, 0.0f,   1.0f, 1.0f, // Top-right
		-0.5f,  0.5f, 0.0f,   0.0f, 1.0f  // Top-left
	};
	unsigned int VAO, VBO, EBO;

	static Renderer* s_Renderer;
};