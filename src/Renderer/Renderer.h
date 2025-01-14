#pragma once

#include "Shader.h"
#include "../Backend/BackendScopeRef.h"
#include "../Editor/CameraEditor.h"
#include "Buffer.h"

struct Renderer
{
	static void Init();
	static void Shutdown();

	static void OnWindowResize(uint32_t width, uint32_t height);

	static void BeginScene(EditorCamera& camera);
	static void EndScene();

	static void Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform = glm::mat4(1.0f));

private:
	struct SceneData
	{
		glm::mat4 ViewProjectionMatrix;
	};

	static Scope<SceneData> s_SceneData;
};