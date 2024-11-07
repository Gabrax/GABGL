#include "Engine.h"
#include "LoadModel.h"
#include "Cube.h"
#include "Skybox.h"
#include "Light.h"
#include "Util.h"
#define GLT_IMPLEMENTATION
#include "gltext.h"
#include "raudio.h"
#include <sstream>  
#include <iomanip>
#include "DAE/Animator.h"
#include "DAE/DAEloader.h"

void Engine::Run(){

    Window::Init((int)(1920 * 0.75f),(int)(1920 * 0.75f));
    stbi_set_flip_vertically_on_load(true);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);

    Util::BakeShaders();
    Cube cube("res/diamond.jpg");
    LoadOBJ house("res/map/objHouse.obj");
    LoadOBJ backpack("res/backpack/backpack.obj");
    LoadDAE guy("res/guy/guy.dae");
    Light light;
    Skybox sky;

    InitAudioDevice();
    Sound fullscreen = LoadSound("res/audio/select1.wav");
    SetSoundVolume(fullscreen, 0.5f);
    Sound switchmode = LoadSound("res/audio/select2.wav");
    SetSoundVolume(switchmode, 0.5f);
    Sound hotload = LoadSound("res/audio/select3.wav");
    SetSoundVolume(hotload, 0.5f); 

    gltInit();
    GLTtext *text1 = gltCreateText();
    gltSetText(text1, "CamPos: (0.0, 0.0, 0.0)");
    GLTtext *text2 = gltCreateText();
    gltSetText(text2, "CamRot: (0.0, 0.0)");  
    
    while (Window::WindowIsOpen() && Window::WindowHasNotBeenForceClosed()){  
        Window::ShowFPS();
        Window::DeltaTime();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            cube.SetupCameraUniforms(Window::_camera, static_cast<float>(Window::width / Window::height),light.getLight());
            cube.Render(Window::_camera, glm::vec3(-2.0f,3.0f,8.0f));
        glEnable(GL_CULL_FACE);    

            light.SetupCameraUniforms(Window::_camera, static_cast<float>(Window::width / Window::height));
            light.Render(Window::_camera, glm::vec3(-2.0f,6.0f,8.0f),glm::vec3(0.5f));

            glCullFace(GL_FRONT);     

            house.SetupCameraUniforms(Window::_camera, static_cast<float>(Window::width / Window::height),light.getLight());
            house.Render(Window::_camera, glm::vec3(0.0f,0.0f,0.0f),glm::vec3(50.0f));
            backpack.SetupCameraUniforms(Window::_camera, static_cast<float>(Window::width / Window::height),light.getLight());
            backpack.Render(Window::_camera, glm::vec3(-2.0f,3.0f,10.0f),glm::vec3(0.25f),90.0f);
            guy.SetupCameraUniforms(Window::_camera, static_cast<float>(Window::width / Window::height),Window::_deltaTime,light.getLight());
            guy.Render(Window::_camera, glm::vec3(0.0f,0.50f,3.0f),glm::vec3(1.0f));

        glDisable(GL_CULL_FACE);

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

                gltBeginDraw();
                    gltColor(1.0f, 1.0f, 1.0f, 1.0f);
                    gltDrawText2D(text1, 0.0f, 0.0f, 2.0f);
                    gltDrawText2D(text2, 0.0f, 40.0f, 2.0f);  
                gltEndDraw();

            // Disable blending for 3D rendering
            glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);

        if (Input::KeyPressed(KEY_F)){
            Window::ToggleFullscreen();
            PlaySound(fullscreen);  
        }    
    
        if (Input::KeyPressed(KEY_H)){
            Window::ToggleWireframe();
            PlaySound(switchmode);
        } 

        if (Input::KeyPressed(KEY_R)){
            Util::HotReloadShaders();
            PlaySound(hotload);
        } 
        

        Window::ProcessInput();
        Input::Update();
        Window::SwapBuffersPollEvents();
    }
}
