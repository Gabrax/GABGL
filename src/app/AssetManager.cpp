#include "AssetManager.h"

#include <future>
#include "../backend/Audio.h"
#include "../backend/Texture.h"
#include "../backend/Renderer.h"
#include "../backend/Model.h"

void AssetManager::LoadAssets()
{
  std::vector<const char*> sounds =
  {
    "res/audio/select1.wav",
    "res/audio/select2.wav", 
  };

  std::vector<const char*> music =
  {
    "res/audio/music.wav", 
  };

  bool cubemapUploaded = false;
  std::vector<std::string> skybox = 
  { 
    "res/skybox/NightSky_Right.png",
    "res/skybox/NightSky_Left.png",
    "res/skybox/NightSky_Top.png",
    "res/skybox/NightSky_Bottom.png",
    "res/skybox/NightSky_Front.png",
    "res/skybox/NightSky_Back.png"
  };

  std::vector<const char*> static_models = 
  { 
    "res/map/objHouse.obj",
    "res/backpack/backpack.obj",
  };

  std::vector<const char*> animated_models = 
  { 
    "res/lowpoly/MaleSurvivor1.glb",
  };


  std::vector<std::future<void>> m_FutureVoid;
  std::vector<std::future<std::shared_ptr<Texture>>> m_FutureTextures;
  std::vector<std::future<std::shared_ptr<Model>>> m_FutureModels;
  std::vector<std::future<std::shared_ptr<Model>>> m_FutureAnimModels;

  for(auto& sound : sounds) m_FutureVoid.push_back(std::async(std::launch::async, AudioSystem::LoadSound, sound)); 
  for(auto& music : music) m_FutureVoid.push_back(std::async(std::launch::async, AudioSystem::LoadMusic, music));

  auto future = std::async(std::launch::async, [skybox]() { return Texture::CreateRAWCUBEMAP(skybox); });
  m_FutureTextures.push_back(std::move(future));
  auto texture = m_FutureTextures.back().get();  
  Renderer::UploadSkybox("night", texture);

  for(auto& model : static_models) m_FutureModels.push_back(std::async(std::launch::async, Model::CreateSTATIC, model));
  for (size_t i = 0; i < static_models.size(); ++i)
  {
      std::shared_ptr<Model> model = m_FutureModels[i].get(); 
      Renderer::UploadModel(static_models[i], model); 
  }
  for(auto& model : animated_models) m_FutureAnimModels.push_back(std::async(std::launch::async, Model::CreateANIMATED, model));
  for (size_t i = 0; i < animated_models.size(); ++i)
  {
      std::shared_ptr<Model> model = m_FutureAnimModels[i].get(); 
      Renderer::UploadModel(animated_models[i], model); 
  }
}


