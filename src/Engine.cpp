#include "Engine.h"
#include "LoadModel.h"
#include "Cube.h"
#include "Skybox.h"
#define GLT_IMPLEMENTATION
#include "gltext.h"
#include <sstream>  
#include <iomanip>

void Engine::Run(){

    Window::Init((int)(1920 * 0.75f),(int)(1920 * 0.75f));
    stbi_set_flip_vertically_on_load(true);

    glEnable(GL_DEPTH_TEST);
    
    Cube cube;
    LoadModel model("resources/backpack/backpack.obj");
    Skybox sky;
    
    
    glm::vec3 cubePositions[] = {
        glm::vec3( 0.0f,  0.0f,  0.0f),
        glm::vec3( 2.0f,  5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3( 2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f,  3.0f, -7.5f),
        glm::vec3( 1.3f, -2.0f, -2.5f),
        glm::vec3( 1.5f,  2.0f, -2.5f),
        glm::vec3( 1.5f,  0.2f, -1.5f),
        glm::vec3(-1.3f,  1.0f, -1.5f)
    };

    gltInit();

    GLTtext *text1 = gltCreateText();
    gltSetText(text1, "CamPos: (0.0, 0.0, 0.0)");

    GLTtext *text2 = gltCreateText();
    gltSetText(text2, "CamRot: (0.0, 0.0)");  
    
    while (Window::WindowIsOpen() && Window::WindowHasNotBeenForceClosed()){  
        Window::ShowFPS();
        Window::DeltaTime();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        cube.SetupCameraUniforms(Window::_camera, static_cast<float>(Window::width / Window::height));

        for(int i = 0; i < 10; i++){
            cube.Render(Window::_camera, cubePositions[i]);
        }
        model.SetupCameraUniforms(Window::_camera, static_cast<float>(Window::width / Window::height));
        model.Render(Window::_camera, glm::vec3(0.0f,2.0f,0.0f),glm::vec3(0.5f));
        sky.Render(Window::_camera, static_cast<float>(Window::width / Window::height));

        glm::vec3 cameraPosition = Window::_camera.Position;  
        std::ostringstream ossPos;
        ossPos << "CamPos: ("
            << std::fixed << std::setprecision(2)
            << cameraPosition.x << ", "
            << cameraPosition.y << ", "
            << cameraPosition.z << ")";
        gltSetText(text1, ossPos.str().c_str());  

        float cameraPitch = Window::_camera.Pitch;
        float cameraYaw = Window::_camera.Yaw;
        std::ostringstream ossRot;
        ossRot << "CamRot: ("
            << std::fixed << std::setprecision(2)
            << cameraPitch << ", "
            << cameraYaw << ")";
        gltSetText(text2, ossRot.str().c_str());  

        glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                // Draw text
                gltBeginDraw();
                    gltColor(1.0f, 1.0f, 1.0f, 1.0f);
                    gltDrawText2D(text1, 0.0f, 0.0f, 2.0f);
                    gltDrawText2D(text2, 0.0f, 40.0f, 2.0f);  
                gltEndDraw();

            // Disable blending for 3D rendering
            glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);

        if (Input::KeyPressed(KEY_F)) Window::ToggleFullscreen();   
    
        if (Input::KeyPressed(KEY_H)) Window::ToggleWireframe();
        

        Window::ProcessInput();
        Input::Update();
        Window::SwapBuffersPollEvents();
    }
}
