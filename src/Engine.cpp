#include "Engine.h"
#include "Input.h"
#include "Key_Values.h"
#include "LoadModel.h"
#include "Renderer.h"
#include "Skybox.h"
#include "LightManager.h"
#include "Window.h"
#include "Input.h"
#include "LoadText.h"
#include "FrameBuffer.h"

void Engine::Run() {

    Window::Init();
    stbi_set_flip_vertically_on_load(true);

    glEnable(GL_DEPTH_TEST);

    Renderer::BakeShaders();
    Renderer::Initialize();

    LoadOBJ house("res/map/objHouse.obj");
    LoadOBJ backpack("res/backpack/backpack.obj");
    LoadOBJ stairs("res/stairs/Stairs.obj");
    LoadDAE guy("res/guy/guy.dae");
    Skybox sky;
    LightManager lightmanager;
    lightmanager.AddLight(Color::Red, glm::vec3(-2.0f, 5.0f, 10.0f), glm::vec3(0.5f));
    lightmanager.AddLight(Color::Blue, glm::vec3(17.0f, 5.0f, -10.0f), glm::vec3(0.5f));
    lightmanager.AddLight(Color::Green, glm::vec3(-12.0f, 5.0f, -1.0f), glm::vec3(0.5f));

    TextRenderer textRenderer;

    Framebuffer mainFB;

    glm::vec3 test = glm::vec3(0.0f);

    while (Window::WindowIsOpen() && Window::WindowHasNotBeenForceClosed()) {

        Window::BeginFrame();

        glBindFramebuffer(GL_FRAMEBUFFER, mainFB.getFBO());
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

        glBindFramebuffer(GL_FRAMEBUFFER, 0); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mainFB.render();

        textRenderer.renderText("CamPos: ", Window::_camera.Position.x, Window::_camera.Position.y, Window::_camera.Position.z);
        textRenderer.renderText("CamRot: ", Window::_camera.Yaw, Window::_camera.Pitch);
        textRenderer.renderText(": ", test.x);
        textRenderer.drawTexts();

        if (Input::KeyDown(KEY_UP)) {
            test.x += 5.0f;
        }
        if (Input::KeyDown(KEY_DOWN)) {
            test.x -= 5.0f;
        }
        
        if (Input::KeyPressed(KEY_BACKSPACE)) {
            lightmanager.AddLight(Color::Red, test, glm::vec3(0.5f));
        }

        if (Input::KeyPressed(KEY_R)) {
            Renderer::HotReloadShaders();
        }

        Window::EndFrame();
    }
}
