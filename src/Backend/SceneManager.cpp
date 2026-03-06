#include "SceneManager.h"


#include "Renderer.h"
#include "AudioManager.h"
#include <fstream>

#include "Scenes.hpp"

void Scene::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT(Scene::OnKeyPressed));
	dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT(Scene::OnMouseButtonPressed));
}

bool Scene::OnKeyPressed(KeyPressedEvent& e)
{
	// Shortcuts
	if (e.IsRepeat()) return false;

	switch (e.GetKeyCode())
	{
		case Key::R:
		{
      Renderer::SetFullscreen("select1", true);
			break;
		}
		case Key::T:
		{
      Renderer::SetFullscreen("select2", false);
			break;
		}
    case Key::Tab:
    {
      Renderer::SwitchRenderState();
      break;
    }
	}

	return false;
}

bool Scene::OnMouseButtonPressed(MouseButtonPressedEvent& e)
{
	/*if (e.GetMouseButton() == Mouse::ButtonLeft)*/
	/*{*/
	/*	if (m_ViewportHovered && !ImGuizmo::IsOver() && !Input::IsKeyPressed(Key::LeftAlt))*/
	/*		m_SelectionContext = m_HoveredEntity;*/
	/*}*/
	return false;
}

void Scene::StartLoading()
{
  if(m_Assets.loadingStarted)
      return;

  LoadSceneFromJSON("res/scenes/scene.json",m_Name);

  m_Assets.loadingStarted = true;
  m_Assets.loadingDone = false;

  for(auto& s : m_Assets.sounds) AudioManager::LoadSound(s.c_str());

  for(auto& m : m_Assets.music) AudioManager::LoadMusic(m.c_str());

  for(auto& model : m_Assets.static_models)
  {
    m_Assets.futureStatic.push_back(
        std::async(std::launch::async,
            Model::CreateSTATIC,
            model.path.c_str(),
            model.scale,
            model.flag,
            model.meshType));
  }

  for(auto& model : m_Assets.animated_models)
  {
    m_Assets.futureAnim.push_back(
        std::async(std::launch::async,
            Model::CreateANIMATED,
            model.path.c_str(),
            model.scale,
            model.flag,
            model.meshType));
  }

  m_Assets.futureTextures.push_back(
      std::async(std::launch::async,[skybox=m_Assets.skybox]()
      {
          return Texture::CreateCUBEMAP(skybox);
      }));
}

void Scene::SpawnEntities()
{
  for(auto& e : m_Assets.entities)
  {
    std::string model = e["model"];

    auto pos = e["position"];

    glm::vec3 position(
        pos[0],
        pos[1],
        pos[2]);

    Transform t;
    t.SetPosition(position);

    std::string type = e.value("type","static");

    if(type == "controller")
    {
      ModelManager::SetInitialControllerTransform(
          model,
          t,
          1.0f,
          1.0f,
          true);
    }
    else
    {
      ModelManager::SetInitialModelTransform(
          model,
          t.GetTransform());
    }
  }
}

void Scene::UpdateLoading()
{
  if(!m_Assets.loadingStarted || m_Assets.loadingDone)
      return;

  auto ready = [](auto& vec)
  {
    return std::all_of(vec.begin(),vec.end(),
    [](auto& f)
    {
        return f.wait_for(std::chrono::seconds(0))
            == std::future_status::ready;
    });
  };

  if(!ready(m_Assets.futureStatic) ||
     !ready(m_Assets.futureAnim) ||
     !ready(m_Assets.futureTextures))
      return;

  if(!m_Assets.uploadStarted)
  {
    m_Assets.uploadStarted = true;

    for(size_t i=0;i<m_Assets.static_models.size();i++)
    {
        ModelManager::BakeModel(
            m_Assets.static_models[i].path,
            m_Assets.futureStatic[i].get());
    }

    for(size_t i=0;i<m_Assets.animated_models.size();i++)
    {
        ModelManager::BakeModel(
            m_Assets.animated_models[i].path,
            m_Assets.futureAnim[i].get());
    }

    auto skyboxTex = m_Assets.futureTextures[0].get();

    Renderer::BakeSkyboxTextures("night",skyboxTex);

    ModelManager::UploadToGPU();
    Renderer::InitDrawCommandBuffer();

    SpawnEntities();

    m_Assets.futureStatic.clear();
    m_Assets.futureAnim.clear();
    m_Assets.futureTextures.clear();

    OnSceneStart();

    m_Assets.loadingDone = true;
    m_Assets.loadingStarted = false;
    m_Assets.uploadStarted = false;
  }
}

bool Scene::IsLoadingComplete() const
{
  return m_Assets.loadingDone;
}

void Scene::LoadSceneFromJSON(const std::string& path, const std::string& sceneName)
{
    std::ifstream file(path);
    json data;
    file >> data;

    auto& scene = data["scenes"][sceneName];

    if(scene.contains("sounds"))
        m_Assets.sounds = scene["sounds"].get<std::vector<std::string>>();

    if(scene.contains("music"))
        m_Assets.music = scene["music"].get<std::vector<std::string>>();

    if(scene.contains("skybox"))
        m_Assets.skybox = scene["skybox"].get<std::vector<std::string>>();

    if(scene.contains("static_models"))
    {
        for(auto& m : scene["static_models"])
        {
            SceneAssets::ModelDesc desc;

            desc.path = m["path"];
            desc.scale = m.value("scale",1.0f);
            desc.flag = false;

            std::string mesh = m.value("mesh","none");

            if(mesh == "trianglemesh") desc.meshType = MeshType::TRIANGLEMESH;
            else if(mesh == "convex")  desc.meshType = MeshType::CONVEXMESH;
            else desc.meshType = MeshType::NONE;

            m_Assets.static_models.push_back(desc);
        }
    }

    if(scene.contains("animated_models"))
    {
        for(auto& m : scene["animated_models"])
        {
            SceneAssets::ModelDesc desc;

            desc.path = m["path"];
            desc.scale = m.value("scale",1.0f);
            desc.flag = false;
            desc.meshType = MeshType::CONTROLLER;

            m_Assets.animated_models.push_back(desc);
        }
    }

    if(scene.contains("entities"))
        m_Assets.entities = scene["entities"];
}

std::unique_ptr<Scene> SceneManager::s_ActiveScene = nullptr;
std::unique_ptr<Scene> SceneManager::s_PendingScene = nullptr;
bool SceneManager::s_Loading = false;

void SceneManager::LoadScene(const std::string& name)
{
  if(name == "game") s_PendingScene = std::make_unique<GameScene>();
  if(name == "menu") s_PendingScene = std::make_unique<MenuScene>();

  s_PendingScene->StartLoading();
  s_Loading = true;
}

void SceneManager::Update(DeltaTime& dt)
{
  if (s_Loading)
  {
    s_PendingScene->UpdateLoading();

    Renderer::DrawLoadingScreen();

    if (s_PendingScene->IsLoadingComplete())
    {
      s_ActiveScene = std::move(s_PendingScene);
      s_Loading = false;
    }

    return;
  }

  if (s_ActiveScene) s_ActiveScene->OnUpdate(dt);
}

Scene* SceneManager::GetActiveScene()
{
  return s_ActiveScene.get();
}

bool SceneManager::IsLoading()
{
  return s_Loading;
}

