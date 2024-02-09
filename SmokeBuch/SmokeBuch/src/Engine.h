#pragma once
#include "glad/glad.h"
#include "GLFW/glfw3.h"

namespace Engine
{
	void Run();
	void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
	void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
}