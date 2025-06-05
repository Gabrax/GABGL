#include "AssetManager.h"

#include <future>
#include "../backend/Audio.h"
#include "../backend/Texture.h"
#include "../backend/Renderer.h"

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

  std::vector<std::future<void>> m_Void;
  std::vector<std::future<std::shared_ptr<Texture>>> m_Textures;

  for(auto& sound : sounds) m_Void.push_back(std::async(std::launch::async,AudioSystem::LoadSound,sound)); 
  for(auto& music : music) m_Void.push_back(std::async(std::launch::async,AudioSystem::LoadMusic,music));

  auto future = std::async(std::launch::async, [skybox]() { return Texture::CreateCubemap(skybox); });
  m_Textures.push_back(std::move(future));

  auto texture = m_Textures.back().get();  
  Renderer::UploadSkybox("night", texture);  
}


