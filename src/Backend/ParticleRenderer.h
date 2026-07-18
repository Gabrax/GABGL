#pragma once

#include "DeltaTime.hpp"

struct ParticleRenderer
{
  static void Init();
  static void Shutdown();
  static void UpdateAndRender(const DeltaTime& dt);
  static void Emit();
};
