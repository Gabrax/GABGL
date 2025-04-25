#include "Engine.h"
#include "Window.h"
#include "Scene.h"
#include <cassert>

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
