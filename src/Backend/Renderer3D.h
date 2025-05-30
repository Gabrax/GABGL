#pragma once

#include "Camera.h"
#include "Texture.h"
#include "Transform.hpp"

struct Renderer3D
{
	static void Init();
	static void Shutdown();

	static void BeginScene(const Camera& camera, const glm::mat4& transform);
	static void BeginScene(const Camera& camera);
	static void EndScene();
	static void Flush();

	// Primitives
	static float GetLineWidth();
	static void SetLineWidth(float width);
	static void LoadShaders();

  static void DrawCube(const TransformComponent& transform, int entityID = -1);
	// Stats
	struct Statistics
	{
		uint32_t DrawCalls = 0;
		uint32_t QuadCount = 0;

		uint32_t GetTotalVertexCount() const { return QuadCount * 4; }
		uint32_t GetTotalIndexCount() const { return QuadCount * 6; }
	};
	static void ResetStats();
	static Statistics GetStats();

private:
	static void StartBatch();
	static void NextBatch();
};
