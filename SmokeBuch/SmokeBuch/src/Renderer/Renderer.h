#pragma once
#include <string>
namespace Renderer
{
	void Render();
	void LoadShit();
	void Keyboard();
	void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
	void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	unsigned int loadTexture(char const* path);
}