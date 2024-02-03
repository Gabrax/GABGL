#pragma once
#include <string>
namespace Renderer
{
	void Render();
	void Keyboard();
	void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
	void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
}