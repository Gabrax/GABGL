#pragma once

#include "../input/Event.h"
#include "../input/KeyEvent.h"
#include <future>

#include "ModelManager.h"

#include "json.hpp"

using json = nlohmann::json;

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
};

struct SceneManager
{
  static void LoadScene(const std::string& scene);
  static void Update(DeltaTime& dt);
  static bool IsLoading();

  static Scene* GetActiveScene();

private:
  static std::unique_ptr<Scene> s_ActiveScene;
  static std::unique_ptr<Scene> s_PendingScene;
  static bool s_Loading;
};
