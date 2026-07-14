#pragma once

#include "KeyCodes.h"

#include <glm/glm.hpp>

struct Input
{
	static bool IsKeyPressed(KeyCode key);
	static bool IsMouseButtonPressed(MouseCode button);
	static glm::vec2 GetMousePosition();
	static float GetMouseX();
	static float GetMouseY();
	static bool IsGamepadConnected();
	static bool IsGamepadButtonPressed(int button);
	static float GetGamepadAxis(int axis);
};

namespace Gamepad
{
  enum Button
  {
    A = 0,
    B = 1,
    Start = 7,
    DPadUp = 11,
    DPadRight = 12,
    DPadDown = 13,
    DPadLeft = 14
  };

  enum Axis
  {
    LeftX = 0,
    LeftY = 1
  };
}
