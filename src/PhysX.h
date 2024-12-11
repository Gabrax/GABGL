#pragma once

#include "PxPhysicsAPI.h"
#include "Utilities.hpp"
#include "LoadShader.h"

using namespace physx;

namespace PhysX {

  void Init();
  void RenderActors(const Shader& shader, unsigned int vao);
  void Simulate(float deltatime);

  PxScene* getScene();
}
