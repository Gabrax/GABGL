#pragma once
#include "LoadShader.h"


struct Shaders {
    Shader cube;
    Shader model;
    Shader skybox;
    Shader animated;
    Shader light;
} g_shaders;

namespace Util{
    inline void BakeShaders(){
        g_shaders.cube.Load("res/shaders/camera.vert","res/shaders/camera.frag");
        g_shaders.model.Load("res/shaders/model.vert","res/shaders/model.frag");
        g_shaders.skybox.Load("res/shaders/skybox.vert","res/shaders/skybox.frag");
        g_shaders.animated.Load("res/shaders/anim_model.vert","res/shaders/anim_model.frag");
        g_shaders.light.Load("res/shaders/light.vert","res/shaders/light.frag");
    }

    inline void HotReloadShaders(){
        std::cout << "HotReloading shaders...\n";
        BakeShaders();
    }
}
