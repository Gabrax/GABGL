#pragma once

#include "SceneManager.h"

#include "../input/UserInput.h"
#include "AudioManager.h"
#include "LightManager.h"
#include "Renderer.h"

struct GameScene : Scene
{
  GameScene() : Scene("game") {}

  void OnSceneStart() override
  {
    AudioManager::PlayMusic("night_mono",glm::vec3(25,2,15),true);

    LightManager::AddLight(LightType::DIRECT,glm::vec3(0.3,0.32,0.4),glm::vec3(0),glm::vec3(-2,-4,-1));
  }

  void OnUpdate(DeltaTime& dt) override
  {
    Renderer::DrawScene(dt,[&]()
    {
      if (Input::IsKeyPressed(Key::W) ||
          Input::IsKeyPressed(Key::S) ||
          Input::IsKeyPressed(Key::A) ||
          Input::IsKeyPressed(Key::D))
      {
        ModelManager::GetModel("harry")->StartBlendToAnimation(1,0.8f);
      }
      else
      {
        ModelManager::GetModel("harry")->StartBlendToAnimation(0,0.8f);
      }

      if(Input::IsKeyPressed(Key::W)) ModelManager::MoveController("harry",Movement::FORWARD,10.0f,dt);
      if(Input::IsKeyPressed(Key::S)) ModelManager::MoveController("harry",Movement::BACKWARD,10.0f,dt);
      if(Input::IsKeyPressed(Key::A)) ModelManager::MoveController("harry",Movement::LEFT,10.0f,dt);
      if(Input::IsKeyPressed(Key::D)) ModelManager::MoveController("harry",Movement::RIGHT,10.0f,dt);
    });
  }
};

struct MenuScene : Scene
{
  MenuScene() : Scene("menu") {}

  void OnSceneStart() override
  {
    AudioManager::PlayMusic("menu",true);
  }

  void OnUpdate(DeltaTime& dt) override
  {
    Renderer::DrawScene(dt,[&]()
    {
    });
  }
};
