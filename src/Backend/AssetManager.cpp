#include "AssetManager.h"

#include <future>
#include "../backend/AudioManager.h"
#include "../backend/Texture.h"
#include "../backend/Renderer.h"
#include "../backend/ModelManager.h"
#include <GLFW/glfw3.h>

struct AssetsData
{
  std::vector<const char*> sounds =
  {
    "res/audio/select1.wav",
    "res/audio/select2.wav",
  };

  std::vector<const char*> music =
  {
    "res/audio/menu.wav",
    "res/audio/night.wav",
    "res/audio/night_mono.wav",
  };

  std::vector<std::string> skybox =
  {
    "res/textures/NightSky_Right.png",
    "res/textures/NightSky_Left.png",
    "res/textures/NightSky_Top.png",
    "res/textures/NightSky_Bottom.png",
    "res/textures/NightSky_Front.png",
    "res/textures/NightSky_Back.png"
  };

  std::vector<std::tuple<const char*, float, bool, MeshType>> static_models =
  {
    /*{ "res/models/radio/radio.obj", 0.7f, false, MeshType::NONE },*/
    { "res/models/street_lamp/street_lamp.obj", 0.7f, false, MeshType::NONE },
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

  std::vector<std::tuple<const char*, float, bool, MeshType>> animated_models =
  {
    { "res/zombie/zombie.glb", 0.5f, false, MeshType::CONTROLLER },
    { "res/models/harry.glb", 1.0f, false, MeshType::CONTROLLER },
  };

  std::vector<std::future<void>> m_FutureVoid;
  std::vector<std::future<std::shared_ptr<Texture>>> m_FutureTextures;
  std::vector<std::future<std::shared_ptr<Model>>> m_FutureStaticModels;
  std::vector<std::future<std::shared_ptr<Model>>> m_FutureAnimModels;

  bool m_UploadStarted = false;
  bool m_LoadingStarted = false;
  bool m_LoadingDone = false;

} s_Data;

void AssetManager::Init()
{
  if (s_Data.m_LoadingStarted) return; // Already started

  s_Data.m_LoadingStarted = true;
  s_Data.m_LoadingDone = false;
  s_Data.m_UploadStarted = false;

  for (auto& sound : s_Data.sounds) AudioManager::LoadSound(sound);

  for (auto& track : s_Data.music) AudioManager::LoadMusic(track);

  for (auto& [path, scale, flag, meshType] : s_Data.static_models)
      s_Data.m_FutureStaticModels.push_back(std::async(std::launch::async, Model::CreateSTATIC, path, scale, flag, meshType));

  for (auto& [path, scale, flag, meshType] : s_Data.animated_models)
      s_Data.m_FutureAnimModels.push_back(std::async(std::launch::async, Model::CreateANIMATED, path, scale, flag, meshType));

  s_Data.m_FutureTextures.push_back(
      std::async(std::launch::async, [skybox = s_Data.skybox]() {
          return Texture::CreateCUBEMAP(skybox);
      })
  );
}

void AssetManager::Update()
{
  if (!s_Data.m_LoadingStarted || s_Data.m_LoadingDone)
      return;

  bool allReady = std::all_of(s_Data.m_FutureVoid.begin(), s_Data.m_FutureVoid.end(),
      [](auto& f) { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }) &&
      std::all_of(s_Data.m_FutureTextures.begin(), s_Data.m_FutureTextures.end(),
      [](auto& f) { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }) &&
      std::all_of(s_Data.m_FutureStaticModels.begin(), s_Data.m_FutureStaticModels.end(),
      [](auto& f) { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }) &&
      std::all_of(s_Data.m_FutureAnimModels.begin(), s_Data.m_FutureAnimModels.end(),
      [](auto& f) { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; });

  if (!allReady)
      return;

  if (!s_Data.m_UploadStarted)
  {
    s_Data.m_UploadStarted = true;

    for (size_t i = 0; i < s_Data.static_models.size(); ++i)
    {
      ModelManager::BakeModel(std::get<0>(s_Data.static_models[i]), s_Data.m_FutureStaticModels[i].get());
    }

    for (size_t i = 0; i < s_Data.animated_models.size(); ++i)
    {
      ModelManager::BakeModel(std::get<0>(s_Data.animated_models[i]), s_Data.m_FutureAnimModels[i].get());
    }

    auto skyboxTexture = s_Data.m_FutureTextures[0].get();
    Renderer::BakeSkyboxTextures("night", skyboxTexture);

    s_Data.m_FutureVoid.clear();
    s_Data.m_FutureTextures.clear();
    s_Data.m_FutureStaticModels.clear();
    s_Data.m_FutureAnimModels.clear();

    ModelManager::UploadToGPU();
    Renderer::InitDrawCommandBuffer();

    s_Data.m_LoadingDone = true;
    s_Data.m_LoadingStarted = false;
    s_Data.m_UploadStarted = false;
  }
}

bool AssetManager::LoadingComplete()
{
    return s_Data.m_LoadingDone;
}


