#include "Renderer.h"

namespace Renderer{
    Shaders g_shaders;
    Sounds g_sounds;
}

void Renderer::Initialize()
{
  g_sounds.hotload = LoadSound("res/audio/select3.wav");
  SetSoundVolume(g_sounds.hotload, 0.5f);
}

void Renderer::BakeShaders()
{
  g_shaders.model.Load("res/shaders/model.vert","res/shaders/model.frag");
  g_shaders.skybox.Load("res/shaders/skybox.vert","res/shaders/skybox.frag");
  g_shaders.animated.Load("res/shaders/anim_model.vert","res/shaders/anim_model.frag");
  g_shaders.light.Load("res/shaders/light.vert","res/shaders/light.frag");
  g_shaders.mainFB.Load("res/shaders/mainFB.vert","res/shaders/mainFB.frag");
}

void Renderer::HotReloadShaders()
{
  std::cout << "HotReloading shaders...\n";
  BakeShaders();
  PlaySound(g_sounds.hotload);
}
