#pragma once

#include "../input/Event.h"
#include "../input/KeyEvent.h"
#include "Buffer.h"
#include <future>

#include "Renderer.h"
#include "ModelManager.h"

struct Scene
{
  Scene();
  virtual ~Scene() = default;
  
	void OnEvent(Event& e);
	void OnUpdate(DeltaTime& dt);

  void StartLoading();
  void UpdateLoading();
  bool IsLoadingComplete() const;

private:
	bool OnKeyPressed(KeyPressedEvent& e);
	bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

  std::vector<std::string> m_Models;
  DrawIndirectBuffer m_drawBuffer;

  struct SceneAssets
  {
    std::vector<const char*> sounds;
    std::vector<const char*> music;
    std::vector<std::tuple<const char*, float, bool, MeshType>> static_models;
    std::vector<std::tuple<const char*, float, bool, MeshType>> animated_models;
    std::vector<std::string> skybox;

    std::vector<std::future<std::shared_ptr<Model>>> futureStatic;
    std::vector<std::future<std::shared_ptr<Model>>> futureAnim;
    std::vector<std::future<std::shared_ptr<Texture>>> futureTextures;

    bool loadingStarted = false;
    bool uploadStarted = false;
    bool loadingDone = false;
  };

  SceneAssets m_Assets;

  void SetupAssets();
  void OnSceneStart();
};

struct SceneManager
{
  static void LoadScene();
  static void Update(DeltaTime& dt);
  static bool IsLoading();

  static Scene* GetActiveScene();

private:
  static std::unique_ptr<Scene> s_ActiveScene;
  static std::unique_ptr<Scene> s_PendingScene;
  static bool s_Loading;
};
