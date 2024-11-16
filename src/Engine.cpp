#include "Engine.h"
#include "Input.h"
#include "Key_Values.h"
#include "LoadModel.h"
#include "Skybox.h"
#include "LightManager.h"
#include "Window.h"
#include "raudio.h"
#include "Input.h"
#include "LoadText.h"
#include "FrameBuffer.h"

void Engine::Run() {
    Window::Init(Window::_windowedWidth, Window::_windowedHeight);
    stbi_set_flip_vertically_on_load(true);

    glEnable(GL_DEPTH_TEST);

    Renderer::BakeShaders();

    LoadOBJ house("res/map/objHouse.obj");
    LoadOBJ backpack("res/backpack/backpack.obj");
    LoadOBJ stairs("res/stairs/Stairs.obj");
    LoadDAE guy("res/guy/guy.dae");
    Skybox sky;
    LightManager lightmanager;
    lightmanager.AddLight(glm::vec3(17.0f, 5.0f, 10.0f), glm::vec4(1.0f), glm::vec3(0.5f));
    lightmanager.AddLight(glm::vec3(17.0f, 5.0f, -10.0f), glm::vec4(1.0f), glm::vec3(0.5f));
    lightmanager.AddLight(glm::vec3(-10.0f, 5.0f, -15.0f), glm::vec4(1.0f), glm::vec3(0.5f));

    Sound fullscreen = LoadSound("res/audio/select1.wav");
    SetSoundVolume(fullscreen, 0.5f);
    Sound switchmode = LoadSound("res/audio/select2.wav");
    SetSoundVolume(switchmode, 0.5f);
    Sound hotload = LoadSound("res/audio/select3.wav");
    SetSoundVolume(hotload, 0.5f);

    InitAudioDevice();
    gltInit();
    TextRenderer textRenderer(2);

    Framebuffer framebuffer;

    while (Window::WindowIsOpen() && Window::WindowHasNotBeenForceClosed()) {
        Window::BeginFrame();
        Window::ShowFPS();
        Window::DeltaTime();


        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.getFBO()); // Return to default framebuffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glFrontFace(GL_CW);

        lightmanager.RenderLights();

        stairs.Render(glm::vec3(1.0f, 0.5f, 2.0f), glm::vec3(1.1f));
        house.Render(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(50.0f));
        backpack.Render(glm::vec3(-2.0f, 3.0f, 10.0f), glm::vec3(0.25f), 90.0f);
        guy.Render(glm::vec3(0.0f, 0.50f, 8.0f), glm::vec3(1.0f));

        glDisable(GL_CULL_FACE);

        sky.Render();


        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Return to default framebuffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render framebuffer's texture to the screen
        framebuffer.render();

        textRenderer.renderText(0, "CamPos: ", Window::_camera.Position.x, Window::_camera.Position.y, Window::_camera.Position.z);
        textRenderer.renderText(1, "CamRot: ", Window::_camera.Yaw, Window::_camera.Pitch);
        textRenderer.drawTexts();

        if (Input::KeyPressed(KEY_F)) {
            Window::ToggleFullscreen();
            PlaySound(fullscreen);
        }

        if (Input::KeyPressed(KEY_H)) {
            Window::ToggleWireframe();
            PlaySound(switchmode);
        }

        if (Input::KeyPressed(KEY_R)) {
            Renderer::HotReloadShaders();
            PlaySound(hotload);
        }

        Window::ProcessInput();
        Input::Update();
        Window::EndFrame();
    }
}
