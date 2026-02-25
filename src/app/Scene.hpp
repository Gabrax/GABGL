#pragma once

#include "../backend/DeltaTime.hpp"
#include "../input/KeyCodes.h"
#include "../input/UserInput.h"
#include "glm/fwd.hpp"
#include "../backend/AudioManager.h"
#include "../backend/LightManager.h"
#include "../backend/ModelManager.h"


namespace Scene
{

  inline void DrawMenu(DeltaTime& dt)
  {
    if (Input::IsKeyPressed(Key::X))
    {
      ModelManager::GetModel("harry")->StartBlendToAnimation(1, 0.8f);
      ModelManager::MoveController("harry", Movement::FORWARD,5.0f,dt);
    }
    else
    {
      ModelManager::GetModel("harry")->StartBlendToAnimation(0, 0.8f); 
    }
    if (Input::IsKeyPressed(Key::C))
    {
      ModelManager::MoveController("harry", Movement::BACKWARD,5.0f,dt);
    }
    if (Input::IsKeyPressed(Key::Z))
    {
      ModelManager::MoveController("harry", Movement::LEFT,5.0f,dt);
    }
    if (Input::IsKeyPressed(Key::V))
    {
      ModelManager::MoveController("harry", Movement::RIGHT,5.0f,dt);
    }
  }

};
