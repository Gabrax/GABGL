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
};