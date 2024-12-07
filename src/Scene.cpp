#include "Scene.h"

#include "Managers/TextManager.h"
#include "Input/Input.h"
#include "Utilities.hpp"
#include "glad/glad.h"

void Scene::Init()
{
  Utilities::Timer timer;

  stbi_set_flip_vertically_on_load(true);

  // Init subsystems
  Input::Init();
  InitAudioDevice();
  gltInit();

  Utilities::BakeShaders();
  Utilities::LoadSounds();

  editor.Init();
  editor.LoadScene();

  envmap.Bake();
  bloom.Init();

  std::cout << timer.Elapsed() << '\n';

  isComplete = true;
}

void Scene::Render()
{
    bloom.Bind();
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glFrontFace(GL_CW);

    lightManager.RenderLights();
    modelManager.RenderModels();

    glDisable(GL_CULL_FACE);

    envmap.Render();

    bloom.RenderBloomTexture(0.005f);
    bloom.UnBind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    bloom.Render();


    glDisable(GL_DEPTH_TEST);

    editor.Render();


    if (Input::KeyPressed(KEY_H)) {
      Utilities::HotReloadShaders();
    }
}
