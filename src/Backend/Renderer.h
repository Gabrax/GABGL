#pragma once

#include "Camera.h"
#include "Texture.h"
#include "Transform.hpp"
#include "Buffer.h"
#include "Shader.h"
#include "ModelManager.h"
#include "DeltaTime.h"
#include "FontManager.h"

struct Renderer
{
	static void Init();
	static void Shutdown();

  static void DrawScene(DeltaTime& dt, const std::function<void()>& geometry, const std::function<void()>& lights);
  static void DrawLoadingScreen();
	static void BeginScene(const Camera& camera, const glm::mat4& transform);
	static void BeginScene(const Camera& camera);
	static void EndScene();
	static void Flush();

	// 3D space
	static void Draw3DQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color);
	static void Draw3DQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const std::shared_ptr<Texture>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
	static void Draw3DQuad(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);
	static void Draw3DQuad(const glm::mat4& transform, const std::shared_ptr<Texture>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f), int entityID = -1);
	static void Draw3DCircle(const glm::mat4& transform, const glm::vec4& color, float thickness = 1.0f, float fade = 0.005f, int entityID = -1);
	static void Draw3DRect(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, int entityID = -1);
	static void Draw3DRect(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);
  static void Draw3DText(const Font* font, const std::string& text,  const glm::vec2& position, float size, const glm::vec4& color, int entityID = -1);
  static void DrawCube(const Transform& transform, int entityID = -1);
  static void DrawCubeContour(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color, int entityID = -1);
  static void DrawModel(const std::shared_ptr<Model>& model, const glm::vec3& position, const glm::vec3& size, const glm::vec3& rotation);
  static void DrawModel(const std::shared_ptr<Model>& model, const std::shared_ptr<Model>& convex, const glm::vec3& position, const glm::vec3& size, const glm::vec3& rotation);
  static void DrawModel(const std::shared_ptr<Model>& model, const glm::mat4& transform = glm::mat4(0.0f), int entityID = -1);
  static void DrawModel(const std::shared_ptr<Model>& model, const std::shared_ptr<Model>& convex, const glm::mat4& transform = glm::mat4(0.0f), int entityID = -1);
  static void DrawModelInstanced(const std::shared_ptr<Model>& model, const std::vector<Transform>& instances, int entityID = -1);
  static void DrawModelInstanced(const std::shared_ptr<Model>& model, const std::shared_ptr<Model>& convex, const std::vector<Transform>& instances, int entityID = -1);

  // 2D space
  static void Draw2DQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color);
	static void Draw2DQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const std::shared_ptr<Texture>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
	static void Draw2DQuad(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);
	static void Draw2DQuad(const glm::mat4& transform, const std::shared_ptr<Texture>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f), int entityID = -1);
	static void Draw2DCircle(const glm::mat4& transform, const glm::vec4& color, float thickness = 1.0f, float fade = 0.005f, int entityID = -1);
	static void Draw2DRect(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color, int entityID = -1);
	static void Draw2DRect(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);
  static void Draw2DText(const Font* font, const std::string& text,  const glm::vec2& position, float size, const glm::vec4& color, int entityID = -1);

  static void BakeSkyboxTextures(const std::string& name,const std::shared_ptr<Texture>& texture);
  static void DrawSkybox(const std::string& name);

	static float GetLineWidth();
	static void SetLineWidth(float width);
	static void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, int entityID = -1);

	static void LoadShaders();

  static void OnWindowResize(uint32_t width, uint32_t height);
	static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
	static void SetClearColor(const glm::vec4& color);
	static void ClearBuffers();
	static void ClearColorBuffers();
	static void ClearDepthBuffers();
  static void SetFullscreen(const std::string& sound, bool windowed);

private:

	static void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0);
	static void DrawLines(const std::shared_ptr<VertexArray>& vertexArray, uint32_t vertexCount);
	static void StartBatch();
	static void NextBatch();

  static void DrawFramebuffer(uint32_t textureID);
	static void DrawEditorFrameBuffer(uint32_t framebufferTexture);
  static uint32_t GetActiveWidgetID();
	static void BlockEvents(bool block);
	static bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);
};
