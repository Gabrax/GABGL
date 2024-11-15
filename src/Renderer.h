#pragma once
#include "LoadShader.h"
#include "Window.h"

namespace Renderer {
    struct Shaders {
        Shader cube;
        Shader model;
        Shader skybox;
        Shader animated;
        Shader light;
        Shader mainFB;
    };
    extern Shaders g_shaders;

    void BakeShaders();
    void HotReloadShaders();
}
