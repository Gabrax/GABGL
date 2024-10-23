#pragma once 

#include <iostream>
#include <gl2d/gl2d.h>

#include "Window.h"
#include "Input.h"
#include "Cube.h"
#include "Camera.h"

namespace Engine {

  void Run();

  inline void BindandLoad(){
    Cube::BindandLoad();
  }

  inline void Render(){
     Cube::Render();
  }
}


