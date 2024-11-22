#pragma once
#include "LoadShader.h"
#include "Window.h"
#include "raudio.h"

namespace Renderer {

    struct Shaders 
    {
      Shader model;
      Shader skybox;
      Shader animated;
      Shader light;
      Shader mainFB;
      Shader bloom_downsample;
      Shader bloom_upsample;
      Shader bloom_final;
    };
    extern Shaders g_shaders;

    struct Sounds
    {
      Sound hotload;
      Sound fullscreen;
      Sound switchmode;
    };
    extern Sounds g_sounds;

    void Initialize();
    void BakeShaders();
    void HotReloadShaders();
    
    namespace Scene {
      void Init();
      void Render();
    }
    
}
