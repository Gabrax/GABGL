#pragma once

#include "../input/Event.h"
#include "../input/KeyEvent.h"
#include <future>

#include "ModelManager.h"

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
  void SyncEditorEntityTransforms();
  SceneEntity* FindEntity(uint64_t entityId);
  const std::vector<SceneEntity>& GetEntities() const { return m_EditorEntities; }
  const std::string& GetName() const { return m_Name; }

private:

  bool OnKeyPressed(KeyPressedEvent& e);
  bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

  void LoadSceneFromJSON(const std::string& path, const std::string& sceneName);
  void SpawnEntities();

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

    bool loadingStarted = false;
    bool uploadStarted = false;
    bool loadingDone = false;
  };

  SceneAssets m_Assets;

  std::string m_Name;
  std::vector<SceneEntity> m_EditorEntities;
  uint64_t m_NextEntityId = 1;
};

struct SceneManager
{
  static void LoadScene(const std::string& scene);
  static void Update(DeltaTime& dt);
  static bool IsLoading();
  static std::vector<std::string> GetAvailableSceneNames();
  static const std::vector<SceneEntity>& GetEntities();
  static SceneEntity* FindEntity(uint64_t entityId);
  static uint64_t DuplicateEntity(uint64_t entityId);
  static bool UpdateEntityTransform(uint64_t entityId, const Transform& transform);
  static void SyncEditorEntityTransforms();
  static bool SaveActiveScene();
  static std::string GetActiveSceneName();

  static Scene* GetActiveScene();

private:
  static std::unique_ptr<Scene> s_ActiveScene;
  static std::unique_ptr<Scene> s_PendingScene;
  static bool s_Loading;
};
