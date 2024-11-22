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
  g_shaders.model.Load("res/shaders/model.glsl");
  g_shaders.animated.Load("res/shaders/anim_model.glsl");
  g_shaders.skybox.Load("res/shaders/skybox.glsl");
  g_shaders.light.Load("res/shaders/light.glsl");
  g_shaders.mainFB.Load("res/shaders/mainFB.glsl");
  g_shaders.bloom_downsample.Load("res/shaders/bloom_downsample.glsl");
  g_shaders.bloom_upsample.Load("res/shaders/bloom_upsample.glsl");
  g_shaders.bloom_final.Load("res/shaders/bloom_final.glsl");
}

void Renderer::HotReloadShaders()
{
  std::cout << "HotReloading shaders...\n";
  BakeShaders();
  PlaySound(g_sounds.hotload);
}
