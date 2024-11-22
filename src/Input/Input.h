#pragma once
#include "Key_Values.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Input {
	void Init();
	void Update();
	void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
	void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	bool KeyPressed(unsigned int keycode);
	bool KeyDown(unsigned int keycode);
	float GetMouseOffsetX();
	float GetMouseOffsetY();
	bool LeftMouseDown();
	bool RightMouseDown();
	bool LeftMousePressed();
	bool RightMousePressed();
	bool MouseWheelUp();
	bool MouseWheelDown();
	int GetMouseWheelValue();
	void PreventRightMouseHold();
	int GetMouseX();
	int GetMouseY();
}
