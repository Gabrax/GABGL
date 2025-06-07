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

  std::vector<const char*> models = 
  { 
    "res/map/objHouse.obj",
  };

  std::vector<std::future<void>> m_FutureVoid;
  std::vector<std::future<std::shared_ptr<Texture>>> m_FutureTextures;
  std::vector<std::future<std::shared_ptr<Model>>> m_FutureModels;

  for(auto& sound : sounds) m_FutureVoid.push_back(std::async(std::launch::async, AudioSystem::LoadSound, sound)); 
  for(auto& music : music) m_FutureVoid.push_back(std::async(std::launch::async, AudioSystem::LoadMusic, music));

  auto future = std::async(std::launch::async, [skybox]() { return Texture::CreateCubemap(skybox); });
  m_FutureTextures.push_back(std::move(future));

  for(auto& model : models) m_FutureModels.push_back(std::async(std::launch::async, Model::CreateSTATIC, model));
  for (size_t i = 0; i < models.size(); ++i)
  {
      std::shared_ptr<Model> model = m_FutureModels[i].get(); // wait and get the model
      Renderer::UploadModel(models[i], model); // use the model name as the key
  }

  auto texture = m_FutureTextures.back().get();  
  Renderer::UploadSkybox("night", texture);
}


