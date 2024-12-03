#include "Engine.h"
#include "FrameBuffers/Bloom.h"
#include "Input/Input.h"

#include "Managers/AnimatedModel.h"
#include "Managers/EnvironmentMap.h"
#include "Managers/LightManager.h"
#include "Managers/StaticModel.h"

#include "Utilities.hpp"
#include "glad/glad.h"
#include "Window.h"
#include "MapEditor.h"
#include "Managers/TextManager.h"

void Engine::Run() {

    Window::Init();
    stbi_set_flip_vertically_on_load(true);

    // Init subsystems
    Input::Init();
    InitAudioDevice();
    gltInit();


    Utilities::BakeShaders();
    Utilities::LoadSounds();

    MapEditor editor;
    editor.Init();

    StaticModel house("res/map/objHouse.obj");
    StaticModel backpack("res/backpack/backpack.obj");
    StaticModel stairs("res/stairs/Stairs.obj");
    AnimatedModel guy("res/lowpoly/MaleSurvivor1.glb");

    EnvironmentMap envmap;

    LightManager lightmanager;
    lightmanager.AddLight(Color::Red, glm::vec3(-2.0f, 5.0f, 10.0f), glm::vec3(0.5f));
    lightmanager.AddLight(Color::Blue, glm::vec3(17.0f, 5.0f, -10.0f), glm::vec3(0.5f));
    lightmanager.AddLight(Color::Green, glm::vec3(-12.0f, 5.0f, -1.0f), glm::vec3(0.5f));
    lightmanager.AddLight(Color::Orange, glm::vec3(17.0f, 5.0f, 10.0f), glm::vec3(0.5f));

    BloomRenderer bloom;
      
    while (Window::WindowIsOpen() && Window::WindowHasNotBeenForceClosed())
    {
        Window::BeginFrame();

        bloom.Bind();
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glFrontFace(GL_CW);

        stairs.Render(glm::vec3(1.0f, 0.5f, 2.0f), glm::vec3(1.1f));
        house.Render(glm::vec3(0.0f, 0.0f, 0.0f));
        backpack.Render(glm::vec3(-2.0f, 3.0f, 10.0f), glm::vec3(0.25f), 90.0f);
        guy.Render(glm::vec3(0.0f, 0.50f, 8.0f));

        lightmanager.RenderLights();

        glDisable(GL_CULL_FACE);

        envmap.Render();

        bloom.RenderBloomTexture(0.005f);
        bloom.UnBind();
       
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        bloom.Render();
        

        glDisable(GL_DEPTH_TEST);

        editor.Render();


        if (Input::KeyPressed(KEY_R)) {
          Utilities::HotReloadShaders();
        }

        if (Input::KeyPressed(KEY_F)) {
          Window::ToggleFullscreen();
          PlaySound(Utilities::g_sounds.fullscreen);
        }

        if (Input::KeyPressed(KEY_H)) {
          Window::ToggleWireframe();
          PlaySound(Utilities::g_sounds.switchmode);
        }

        Window::EndFrame();
    }
}
