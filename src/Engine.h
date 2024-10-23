#pragma once 

#include <iostream>
#include <gl2d/gl2d.h>

#include "Window.h"
#include "Input.h"
#include "Cube.h"

namespace Engine {

  inline void Run() {

  	Window::Init((int)(1920 * 0.75f),(int)(1920 * 0.75f));
    stbi_set_flip_vertically_on_load(true);

    glEnable(GL_DEPTH_TEST);
    
    Cube::BindandLoad();

    while (Window::WindowIsOpen() && Window::WindowHasNotBeenForceClosed()){  
        Window::ShowFPS();

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Cube::Render();

        if (Input::KeyPressed(KEY_F)){
          Window::ToggleFullscreen();   
        }
        if (Input::KeyPressed(KEY_H)){
			    Window::ToggleWireframe();
        }

        Window::ProcessInput();
        Input::Update();
        Window::SwapBuffersPollEvents();
    }
  }
}


