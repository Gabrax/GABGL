#include "Engine.h"
#include "Window.h"
#include "Scene.h"
#include <cassert>

/*#include <TracyClient.cpp> // PROFILER //*/
/*#include <tracy/TracyOpenGL.hpp>*/
/*#include <tracy/TracyC.h>*/

void Engine::Run() {

    Window::Init();
    Scene scene;

    while (Window::WindowIsOpen() && Window::WindowHasNotBeenForceClosed())
    {
        Window::BeginFrame();

        if(!scene.isLoadingDone()){
          scene.Init();
        }
        assert(scene.isLoadingDone());
        scene.Render();

        Window::EndFrame();
    }
}
