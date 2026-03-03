#include "SceneManager.h"


#include "Renderer.h"
#include "AudioManager.h"
#include "LightManager.h"
#include "../input/UserInput.h"
#include "json.hpp"
#include <fstream>

Scene::Scene() {}

void Scene::OnUpdate(DeltaTime& dt)
{
  Renderer::DrawScene(dt, 
    [&dt]()
    {
      if (Input::IsKeyPressed(Key::W) ||
          Input::IsKeyPressed(Key::S) ||
          Input::IsKeyPressed(Key::A) ||
          Input::IsKeyPressed(Key::D))
      {
        ModelManager::GetModel("harry")->StartBlendToAnimation(1, 0.8f);
      }
      else
      {
        ModelManager::GetModel("harry")->StartBlendToAnimation(0, 0.8f); 
      }

      if (Input::IsKeyPressed(Key::W)) ModelManager::MoveController("harry", Movement::FORWARD,10.0f,dt);
      if (Input::IsKeyPressed(Key::S)) ModelManager::MoveController("harry", Movement::BACKWARD,10.0f,dt);
      if (Input::IsKeyPressed(Key::A)) ModelManager::MoveController("harry", Movement::LEFT,10.0f,dt);
      if (Input::IsKeyPressed(Key::D)) ModelManager::MoveController("harry", Movement::RIGHT,10.0f,dt);
    }
  );
}

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

using json = nlohmann::json;

void Scene::SetupAssets()
{
  m_Assets.sounds =
  {
    "res/audio/select1.wav",
    "res/audio/select2.wav",
  };

  m_Assets.music =
  {
    "res/audio/menu.wav",
    "res/audio/night.wav",
    "res/audio/night_mono.wav",
  };

  m_Assets.skybox =
  {
    "res/textures/NightSky_Right.png",
    "res/textures/NightSky_Left.png",
    "res/textures/NightSky_Top.png",
    "res/textures/NightSky_Bottom.png",
    "res/textures/NightSky_Front.png",
    "res/textures/NightSky_Back.png"
  };

  m_Assets.static_models =
  {
    /*{ "res/models/street_lamp/street_lamp.obj", 0.7f, false, MeshType::NONE },*/
    { "res/map/objHouse.obj", 0.7f, false, MeshType::TRIANGLEMESH },
    { "res/models/aidkit.glb", 1.0f, false, MeshType::NONE },
    { "res/models/aidkit_convex.glb", 1.0f, false, MeshType::CONVEXMESH },
    { "res/models/pistolammo.glb", 1.0f, false, MeshType::NONE },
    { "res/models/pistolammo_convex.glb", 1.0f, false, MeshType::CONVEXMESH },
    { "res/models/shotgunammo.glb", 1.0f, false, MeshType::NONE },
    { "res/models/shotgunammo_convex.glb", 1.0f, false, MeshType::CONVEXMESH },
    { "res/models/pistol.glb", 1.0f, false, MeshType::NONE },
    { "res/models/pistol_convex.glb", 1.0f, false, MeshType::CONVEXMESH },
    { "res/models/shotgun.glb", 1.0f, false, MeshType::NONE },
    { "res/models/shotgun_convex.glb", 1.0f, false, MeshType::CONVEXMESH },
    /*{ "res/models/sphere.glb", 1.0f, false, MeshType::NONE },*/
    /*{ "res/models/outer_terrain.glb", 1.0f, false, MeshType::NONE },*/
  };

  m_Assets.animated_models =
  {
    { "res/zombie/zombie.glb", 0.5f, false, MeshType::CONTROLLER },
    { "res/models/harry.glb", 1.0f, false, MeshType::CONTROLLER },
  };
}

void Scene::StartLoading()
{
  if (m_Assets.loadingStarted)
      return;

  SetupAssets();

  m_Assets.loadingStarted = true;
  m_Assets.loadingDone = false;

  for (auto& sound : m_Assets.sounds) AudioManager::LoadSound(sound);
  for (auto& track : m_Assets.music) AudioManager::LoadMusic(track);

  // CPU async
  for (auto& [path, scale, flag, meshType] : m_Assets.static_models)
      m_Assets.futureStatic.push_back(
          std::async(std::launch::async, Model::CreateSTATIC, path, scale, flag, meshType)
      );

  for (auto& [path, scale, flag, meshType] : m_Assets.animated_models)
      m_Assets.futureAnim.push_back(
          std::async(std::launch::async, Model::CreateANIMATED, path, scale, flag, meshType)
      );

  m_Assets.futureTextures.push_back(
      std::async(std::launch::async, [skybox = m_Assets.skybox]() {
          return Texture::CreateCUBEMAP(skybox);
      })
  );
}

void Scene::UpdateLoading()
{
  if (!m_Assets.loadingStarted || m_Assets.loadingDone)
      return;

  auto ready = [](auto& vec)
  {
    return std::all_of(vec.begin(), vec.end(),
        [](auto& f)
        {
          return f.wait_for(std::chrono::seconds(0))
                 == std::future_status::ready;
        });
  };

  if (!ready(m_Assets.futureStatic) ||
      !ready(m_Assets.futureAnim) ||
      !ready(m_Assets.futureTextures))
      return;

  if (!m_Assets.uploadStarted)
  {
    m_Assets.uploadStarted = true;

    for (size_t i = 0; i < m_Assets.static_models.size(); ++i)
        ModelManager::BakeModel(std::get<0>(m_Assets.static_models[i]),m_Assets.futureStatic[i].get());

    for (size_t i = 0; i < m_Assets.animated_models.size(); ++i)
        ModelManager::BakeModel(std::get<0>(m_Assets.animated_models[i]),m_Assets.futureAnim[i].get());

    auto skyboxTex = m_Assets.futureTextures[0].get();
    Renderer::BakeSkyboxTextures("night", skyboxTex);

    ModelManager::UploadToGPU();
    Renderer::InitDrawCommandBuffer();

    // cleanup
    m_Assets.futureStatic.clear();
    m_Assets.futureAnim.clear();
    m_Assets.futureTextures.clear();

    m_Assets.loadingDone = true;
    m_Assets.loadingStarted = false;
    m_Assets.uploadStarted = false;

    OnSceneStart();
  }
}

bool Scene::IsLoadingComplete() const
{
  return m_Assets.loadingDone;
}

void Scene::OnSceneStart()
{
  AudioManager::SetAttunation(AL_LINEAR_DISTANCE_CLAMPED);
  AudioManager::SetListenerVolume(0.05f);
  AudioManager::PlayMusic("menu",true);

  /*LightManager::AddLight(LightType::SPOT, glm::vec3(1.0f,1.0f,1.0), glm::vec3(15.0f,15.0f,15.0f), glm::vec3(0.0,-1.0,0.0)); */

  AudioManager::PlayMusic("night_mono",glm::vec3(25.0f,2.0f,15.0f),true);

  /*Transform lamp;*/
  /*lamp.SetPosition(glm::vec3(35.0f,0.0f,25.0f));*/
  /*ModelManager::SetInitialModelTransform("street_lamp", lamp.GetTransform());*/

  Transform transform;
  transform.SetPosition(glm::vec3(5.0f));
  /*ModelManager::SetInitialModelTransform("sphere", transform.GetTransform());*/
  LightManager::AddLight(LightType::POINT, glm::vec3(1.0f,1.0f,0.0), transform.GetPosition(), glm::vec3(1.0f));
  LightManager::AddLight(LightType::DIRECT, glm::vec3(0.3, 0.32, 0.4), glm::vec3(0.0f), glm::vec3(-2.0f, -4.0f, -1.0f));
  /*Transform terraintransform;*/
  /*terraintransform.SetPosition(glm::vec3(-200.0f,-200.0f,50.0f));*/
  /*ModelManager::SetInitialModelTransform("outer_terrain", terraintransform.GetTransform());*/
  Transform housetransform;
  housetransform.SetPosition(glm::vec3(0.0f));
  ModelManager::SetInitialModelTransform("objHouse", housetransform.GetTransform());
  Transform pistoltransform;
  pistoltransform.SetPosition(glm::vec3(15.0f,4.0f,13.0f));
  ModelManager::SetInitialModelTransform("pistol", pistoltransform.GetTransform());
  ModelManager::SetRender("pistol_convex", false);
  Transform pistolammotransform;
  pistolammotransform.SetPosition(glm::vec3(11.0f,4.0f,5.0f));
  ModelManager::SetInitialModelTransform("pistolammo", pistolammotransform.GetTransform());
  ModelManager::SetRender("pistolammo_convex", false);
  Transform shotguntransform;
  shotguntransform.SetPosition(glm::vec3(14.0f,4.0f,11.0f));
  ModelManager::SetInitialModelTransform("shotgun", shotguntransform.GetTransform());
  ModelManager::SetRender("shotgun_convex", false);
  Transform shotgunammotransform;
  shotgunammotransform.SetPosition(glm::vec3(12.0f,4.0f,7.0f));
  ModelManager::SetInitialModelTransform("shotgunammo", shotgunammotransform.GetTransform());
  ModelManager::SetRender("shotgunammo_convex", false);
  Transform aidkittransform;
  aidkittransform.SetPosition(glm::vec3(13.0f,4.0f,9.0f));
  ModelManager::SetInitialModelTransform("aidkit", aidkittransform.GetTransform());
  ModelManager::SetRender("aidkit_convex", false);
  Transform harrytransform;
  harrytransform.SetPosition(glm::vec3(5.0f,0.0f,0.0f));
  ModelManager::SetInitialControllerTransform("harry", harrytransform,1.0f,1.0f,true);
  Transform zombietransform;
  zombietransform.SetPosition(glm::vec3(10.0f,0.0f,0.0f));
  ModelManager::SetInitialControllerTransform("zombie", zombietransform,1.0f,1.0f,true);
}


std::unique_ptr<Scene> SceneManager::s_ActiveScene = nullptr;
std::unique_ptr<Scene> SceneManager::s_PendingScene = nullptr;
bool SceneManager::s_Loading = false;

void SceneManager::LoadScene()
{
  s_PendingScene = std::make_unique<Scene>();
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

