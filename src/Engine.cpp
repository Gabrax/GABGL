#include "Engine.h"
#include "Input.h"
#include "LoadModel.h"
#include "Cube.h"
#include "Skybox.h"
#include "Light.h"
#include "raudio.h"
#include "Input.h"
#include "LoadText.h"
#include <sstream>  
#include <iomanip>
#include "Util.h"

void Engine::Run(){

    Window::Init(Window::_windowedWidth, Window::_windowedHeight);
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
    
    Sound fullscreen = LoadSound("res/audio/select1.wav");
    SetSoundVolume(fullscreen, 0.5f);
    Sound switchmode = LoadSound("res/audio/select2.wav");
    SetSoundVolume(switchmode, 0.5f);
    Sound hotload = LoadSound("res/audio/select3.wav");
    SetSoundVolume(hotload, 0.5f); 

    InitAudioDevice();
    gltInit();
    TextRenderer textRenderer(2);  
    
    while (Window::WindowIsOpen() && Window::WindowHasNotBeenForceClosed()){  
        Window::ShowFPS();
        Window::DeltaTime();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            cube.Render(glm::vec3(-2.0f,3.0f,8.0f), glm::vec3(1.0f), light.getLight());

        glEnable(GL_CULL_FACE);    
            
            light.Render(glm::vec3(-2.0f,6.0f,8.0f), glm::vec3(0.5f));

            glCullFace(GL_FRONT);     

            house.Render(glm::vec3(0.0f,0.0f,0.0f), glm::vec3(50.0f), light.getLight());
            backpack.Render(glm::vec3(-2.0f,3.0f,10.0f), glm::vec3(0.25f), light.getLight(), 90.0f);
            guy.Render(glm::vec3(0.0f,0.50f,3.0f), glm::vec3(1.0f), light.getLight());

        glDisable(GL_CULL_FACE);

        sky.Render();

        textRenderer.renderText(0, "CamPos: ",Window::_camera.Position.x , Window::_camera.Position.y, Window::_camera.Position.z);
        textRenderer.renderText(1, "CamRot: ",Window::_camera.Yaw, Window::_camera.Pitch);
        textRenderer.drawTexts(); 

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
