#pragma once

#include "../input/Event.h"
#include "../input/KeyEvent.h"
#include <future>

#include "ModelManager.h"
#include "LightManager.h"

#include "json.hpp"

using json = nlohmann::json;

struct SceneEntity
{
  uint64_t id = 0;
  std::string name;
  std::string model;
  std::string type = "static";
  Transform transform;
  uint32_t instanceIndex = 0;
};

struct SceneLight
{
  uint64_t id = 0;
  std::string name;
  LightType type = LightType::POINT;
  glm::vec3 color = glm::vec3(1.0f);
  glm::vec3 position = glm::vec3(0.0f);
  glm::vec3 rotation = glm::vec3(0.0f, -1.0f, 0.0f);
};

struct Scene
{
  Scene(const std::string& name) : m_Name(name) {}

  virtual ~Scene() = default;

  virtual void OnUpdate(DeltaTime& dt) = 0;
  virtual void OnSceneStart() = 0;
  void OnEvent(Event& e);

  void StartLoading();
  void UpdateLoading();
  bool IsLoadingComplete() const;
  bool SaveToJSON(const std::string& path) const;
  uint64_t DuplicateEntity(uint64_t entityId);
  bool UpdateEntityTransform(uint64_t entityId, const Transform& transform);
  uint64_t AddLight(LightType type);
  bool UpdateLight(uint64_t lightId, const std::string& name, const glm::vec3& color,
    const glm::vec3& position, const glm::vec3& rotation);
  bool RemoveLight(uint64_t lightId);
  void SyncEditorEntityTransforms();
  SceneEntity* FindEntity(uint64_t entityId);
  SceneLight* FindLight(uint64_t lightId);
  const std::vector<SceneEntity>& GetEntities() const { return m_EditorEntities; }
  const std::vector<SceneLight>& GetLights() const { return m_EditorLights; }
  const std::string& GetName() const { return m_Name; }

private:

  bool OnKeyPressed(KeyPressedEvent& e);
  bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

  void LoadSceneFromJSON(const std::string& path, const std::string& sceneName);
  void SpawnEntities();
  void SpawnLights();

  struct SceneAssets
  {
    std::vector<std::string> sounds;
    std::vector<std::string> music;

    struct ModelDesc
    {
        std::string path;
        float scale;
        bool flag;
        MeshType meshType;
    };

    std::vector<ModelDesc> static_models;
    std::vector<ModelDesc> animated_models;

    std::vector<std::string> skybox;

    std::vector<std::future<std::shared_ptr<Model>>> futureStatic;
    std::vector<std::future<std::shared_ptr<Model>>> futureAnim;
    std::vector<std::future<std::shared_ptr<Texture>>> futureTextures;

    json entities;
    json lights = json::array();

    bool loadingStarted = false;
    bool uploadStarted = false;
    bool loadingDone = false;
  };

  SceneAssets m_Assets;

  std::string m_Name;
  std::vector<SceneEntity> m_EditorEntities;
  std::vector<SceneLight> m_EditorLights;
  uint64_t m_NextEntityId = 1;
  uint64_t m_NextLightId = 1;
};

struct SceneManager
{
  static void LoadScene(const std::string& scene);
  static void Update(DeltaTime& dt);
  static bool IsLoading();
  static std::vector<std::string> GetAvailableSceneNames();
  static const std::vector<SceneEntity>& GetEntities();
  static const std::vector<SceneLight>& GetLights();
  static SceneEntity* FindEntity(uint64_t entityId);
  static SceneLight* FindLight(uint64_t lightId);
  static uint64_t DuplicateEntity(uint64_t entityId);
  static bool UpdateEntityTransform(uint64_t entityId, const Transform& transform);
  static uint64_t AddLight(LightType type);
  static bool UpdateLight(uint64_t lightId, const std::string& name, const glm::vec3& color,
    const glm::vec3& position, const glm::vec3& rotation);
  static bool RemoveLight(uint64_t lightId);
  static void SyncEditorEntityTransforms();
  static bool SaveActiveScene();
  static std::string GetActiveSceneName();

  static Scene* GetActiveScene();

private:
  static std::unique_ptr<Scene> s_ActiveScene;
  static std::unique_ptr<Scene> s_PendingScene;
  static bool s_Loading;
};
