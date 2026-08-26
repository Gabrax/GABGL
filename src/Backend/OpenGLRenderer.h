#pragma once

#include "Camera.h"
#include "Texture.h"
#include "Buffer.h"
#include "DeltaTime.hpp"
#include "FontManager.h"
#include "ModelManager.h"
#include "RenderCommon.h"

struct ParticleRenderInstance;

struct OpenGLRenderer
{
	static void Init();
	static void Shutdown();

	static void DrawScene(DeltaTime& dt, const std::function<void()>& scene_logic, bool advanceSimulation = true);
	[[nodiscard]] static bool IsRenderingEditor();
	static void DrawNativeSceneOverlay(DeltaTime& dt, bool advanceSimulation, bool renderForEditor);
	static void DrawLoadingScreen();
	static void DrawScreenOverlay(float opacity, const glm::vec3& color = glm::vec3(0.0f));
	static void PrepareScreenUI(const glm::vec4& clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
		bool clear = true);
	static void BeginScene();
	static void EndScene();

	static void DrawQuad(const glm::vec3& position, const glm::vec3& size, const glm::vec3& rotation, const glm::vec4& color);
	static void DrawQuad(const glm::vec3& position, const glm::vec3& size, const glm::vec3& rotation, const std::shared_ptr<Texture>& texture, const glm::vec4& tintColor = glm::vec4(1.0f));
	static void DrawQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color);
	static void DrawQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const std::shared_ptr<Texture>& texture, const glm::vec4& tintColor = glm::vec4(1.0f));
	static void DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);
	static void DrawQuad(const glm::mat4& transform, const std::shared_ptr<Texture>& texture, const glm::vec4& tintColor = glm::vec4(1.0f), float tilingFactor = 1.0f, int entityID = -1);

	static void DrawQuadContour(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, int entityID = -1);
	static void DrawQuadContour(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color, int entityID = -1);
	static void DrawQuadContour(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);

	static void DrawCube(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color, int entityID = -1);
	static void DrawCube(const glm::vec3& position, const glm::vec3& size, const std::shared_ptr<Texture>& texture, const glm::vec4& tintColor = glm::vec4(1.0f), int entityID = -1);
	static void DrawCubeContour(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color, int entityID = -1);

	static void DrawText(const Font* font, const std::string& text, const glm::vec3& position, const glm::vec3& rotation, float size, const glm::vec4& color = glm::vec4(1.0f));
	static void DrawText(const Font* font, const std::string& text, const glm::vec2& position, float size, const glm::vec4& color = glm::vec4(1.0f));
	static void DrawText(const Font* font, const std::string& text, const glm::vec3& position, const glm::vec3& rotation, float size, const glm::vec4& color, int entityID = -1);
	static void DrawParticles(const std::vector<ParticleRenderInstance>& instances);

	// Screen-space (orthographic, pixel-coordinate) commands are consumed once per frame.
	// Quad position and text position are their visual centers.
	static void DebugDrawQuad2D(const glm::vec2& position, const glm::vec2& size,
		const glm::vec4& color = glm::vec4(1.0f), float rotation = 0.0f, bool outline = false);
	static void DebugDrawText2D(const std::string& text, const glm::vec2& position, float size = 0.35f,
		const glm::vec4& color = glm::vec4(1.0f));

	static void BakeSkyboxTextures(const std::string& name,const std::shared_ptr<Texture>& texture);
	static void DrawSkybox(const std::string& name);

	static float GetLineWidth();
	static void SetLineWidth(float width);
	static void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, int entityID = -1);

	static void AddDrawCommand(const std::string& modelName, uint32_t verticesSize, uint32_t indicesSize);
	static void RebuildDrawCommandsForModel(const std::shared_ptr<Model>& model, bool render);
	static void UpdateDrawCommandInstances(const std::shared_ptr<Model>& model);
	static void SetModelPreviews(const std::vector<RenderModelPreview>& previews);
	static void InitDrawCommandBuffer();
	static void ResetModelDrawCommands();

	static void DrawFullscreenQuad();
	static void SetFullscreen(const std::string& sound, bool windowed);
	static void ApplyDisplaySettings();
	static void ApplyGraphicsSettings();
	static void SwitchRenderState();
	static void SetLights(const std::vector<RenderLight>& lights);
	static void DrawEditorFrameBuffer(uint64_t framebufferTexture);

private:

	static void Set3D(bool is3D);
	static void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0);
	static void DrawLines(const std::shared_ptr<VertexArray>& vertexArray, uint32_t vertexCount);
	static void StartBatch();
	static void Flush();
	static void NextBatch();
	static void LoadShaders();
	static void DrawPhysicsDebug();
	static void DrawDebugVisualizations();
	static void DrawDebug2D();
	static void UpdateModelFrustumCulling();

	static void DrawFramebuffer(uint32_t textureID, bool applyPS1Effect = false);
	static uint32_t GetActiveWidgetID();
	static void BlockEvents(bool block);
	static bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);
};
