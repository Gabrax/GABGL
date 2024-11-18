#pragma once
#include "LoadShader.h"
#include "Window.h"
#include "raudio.h"

namespace Renderer {

    struct Shaders 
    {
      Shader cube;
      Shader model;
      Shader skybox;
      Shader animated;
      Shader light;
      Shader mainFB;
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
}
