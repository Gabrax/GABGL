#include "Engine.h"
#include "Bloom.h"
#include "Input/Input.h"
#include "AnimatedModel.h"
#include "Renderer.h"
#include "Skybox.h"
#include "LightManager.h"
#include "Window.h"
#include "LoadText.h"
#include "glad/glad.h"
#include "OBJ/StaticModel.h"

void Engine::Run() {

    Window::Init();
    stbi_set_flip_vertically_on_load(true);

    glEnable(GL_DEPTH_TEST);

    Renderer::BakeShaders();
    Renderer::Initialize();

    StaticModel house("res/map/objHouse.obj");
    StaticModel backpack("res/backpack/backpack.obj");
    StaticModel stairs("res/stairs/Stairs.obj");
    /*AnimatedModel guy("res/guy/guy.dae");*/
    AnimatedModel guy("res/lowpoly/MS1.glb");
    /*AnimatedModel guy("res/pistol/source/pistolAnimated.fbx");*/
    Skybox sky;

    LightManager lightmanager;
    lightmanager.AddLight(Color::Red, glm::vec3(-2.0f, 5.0f, 10.0f), glm::vec3(0.5f));
    lightmanager.AddLight(Color::Blue, glm::vec3(17.0f, 5.0f, -10.0f), glm::vec3(0.5f));
    lightmanager.AddLight(Color::Green, glm::vec3(-12.0f, 5.0f, -1.0f), glm::vec3(0.5f));
    lightmanager.AddLight(Color::Orange, glm::vec3(17.0f, 5.0f, 10.0f), glm::vec3(0.5f));

    TextRenderer textRenderer;

    BloomRenderer bloom;
      
    glm::vec3 test = glm::vec3(0.0f,5.0f,0.0f);

    while (Window::WindowIsOpen() && Window::WindowHasNotBeenForceClosed()) {

        Window::BeginFrame();

        bloom.Bind();
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glFrontFace(GL_CW);

        stairs.Render(glm::vec3(1.0f, 0.5f, 2.0f), glm::vec3(1.1f));
        house.Render(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(50.0f));
        backpack.Render(glm::vec3(-2.0f, 3.0f, 10.0f), glm::vec3(0.25f), 90.0f);
        guy.Render(glm::vec3(0.0f, 0.50f, 8.0f), glm::vec3(1.0f));

        lightmanager.RenderLights();

        glDisable(GL_CULL_FACE);

        sky.Render();
        bloom.RenderBloomTexture(0.005f);
        bloom.UnBind();
       
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        bloom.Render();
        

        glDisable(GL_DEPTH_TEST);

        textRenderer.renderText("CamPos: ", Window::_camera.Position.x, Window::_camera.Position.y, Window::_camera.Position.z);
        textRenderer.renderText("CamRot: ", Window::_camera.Yaw, Window::_camera.Pitch);
        textRenderer.renderText("testcube: ", test.x);
        textRenderer.drawTexts();

        if (Input::KeyDown(KEY_UP)) {
            test.x += 5.0f;
        }

        if (Input::KeyDown(KEY_DOWN)) {
            test.x -= 5.0f;
        }
        
        if (Input::KeyPressed(KEY_BACKSPACE)) {
            lightmanager.AddLight(Color::Orange_bright, test, glm::vec3(0.5f));
        }

        if (Input::KeyPressed(KEY_R)) {
            Renderer::HotReloadShaders();
        }

        Window::EndFrame();
    }
}
