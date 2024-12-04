#include "Engine.h"
#include "FrameBuffers/Bloom.h"
#include "Input/Input.h"

#include "Managers/EnvironmentMap.h"

#include "Utilities.hpp"
#include "glad/glad.h"
#include "Window.h"
#include "MapEditor.h"
#include "Managers/TextManager.h"


/*#include <TracyClient.cpp> // PROFILER //*/
/*#include <tracy/TracyOpenGL.hpp>*/
/*#include <tracy/TracyC.h>*/


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
    editor.LoadScene();
    
    EnvironmentMap envmap;
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

        editor.RenderScene();

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
