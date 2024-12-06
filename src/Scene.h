#pragma once

#include "Managers/EnvironmentMap.h"
#include "FrameBuffers/Bloom.h"
#include "Managers/LightManager.h"
#include "Managers/ModelManager.h"
#include "MapEditor.h"

struct Scene {

  Scene() : editor(modelManager, lightManager){}
  ~Scene() = default;
  
  void Init();
  void Render();
  bool isLoadingDone() { return isComplete; }
  
private:

    bool isComplete = false;

    ModelManager modelManager;
    LightManager lightManager;
    SceneEditor editor;
    EnvironmentMap envmap;
    BloomRenderer bloom;
};
