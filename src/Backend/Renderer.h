#pragma once

#include "Camera.h"
#include "Texture.h"
#include "Buffer.h"
#include "DeltaTime.hpp"
#include "FontManager.h"
#include "ModelManager.h"

struct Renderer
{
	static void Init();
	static void Shutdown();

  static void DrawScene(DeltaTime& dt, const std::function<void()>& geometry, const std::function<void()>& lights);
  static void DrawLoadingScreen();
	static void BeginScene(const Camera& camera);
	static void EndScene();

	static void DrawQuad(glm::vec3& position, const glm::vec3& size, const glm::vec3& rotation, const glm::vec4& color);
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

  static void BakeSkyboxTextures(const std::string& name,const std::shared_ptr<Texture>& texture);
  static void DrawSkybox(const std::string& name);

	static float GetLineWidth();
	static void SetLineWidth(float width);
	static void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, int entityID = -1);

  static void AddDrawCommand(const std::string& modelName, uint32_t verticesSize, uint32_t indicesSize);
  static void RebuildDrawCommandsForModel(const std::shared_ptr<Model>& model, bool render);
  static void InitDrawCommandBuffer();

  static void DrawFullscreenQuad();
  static void SetFullscreen(const std::string& sound, bool windowed);
  static void SwitchRenderState();

private:

  static void Set3D(bool is3D);
	static void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0);
	static void DrawLines(const std::shared_ptr<VertexArray>& vertexArray, uint32_t vertexCount);
	static void StartBatch();
	static void Flush();
	static void NextBatch();
	static void LoadShaders();

  static void DrawFramebuffer(uint32_t textureID);
	static void DrawEditorFrameBuffer(uint32_t framebufferTexture);
  static uint32_t GetActiveWidgetID();
	static void BlockEvents(bool block);
	static bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);
};
