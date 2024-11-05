#pragma once
#include "LoadShader.h"


struct Shaders {
    Shader cube;
    Shader model;
    Shader skybox;
} g_shaders;

namespace Util{
    inline void BakeShaders(){
        g_shaders.cube.Load("resources/camera.vert","resources/camera.frag");
        g_shaders.model.Load("resources/model.vert","resources/model.frag");
        g_shaders.skybox.Load("resources/skybox.vert","resources/skybox.frag");
    }

    inline void HotReloadShaders(){
        std::cout << "HotReloading shaders...\n";
        BakeShaders();
    }
}
