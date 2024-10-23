#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Camera.h"


namespace Window
{
	void Init(int width, int height);
	void ShowFPS();
	void ProcessInput();
	void SwapBuffersPollEvents();
	void Cleanup();
	bool WindowIsOpen();
	int GetWindowWidth();
	int GetWindowHeight();
	int GetCursorX();
	int GetCursorY();
	void DisableCursor();
	void HideCursor();
	void ShowCursor();
	GLFWwindow* GetWindowPtr();
	int GetCursorScreenX();
	int GetCursorScreenY();
	bool WindowHasFocus();
	bool WindowHasNotBeenForceClosed();
	void ForceCloseWindow();

	enum WindowMode { WINDOWED, FULLSCREEN };
	enum RenderMode { WIREFRAME, NORMAL };
	void CreateWindow(WindowMode windowMode);
	void SetWindowMode(WindowMode windowMode);
	void SetRenderMode(RenderMode renderMode);
	void ToggleFullscreen();
	void ToggleWireframe();
	void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
	void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	int GetScrollWheelYOffset();
	void ResetScrollWheelYOffset();
	void processInput(GLFWwindow* window);
	void framebuffer_size_callback(GLFWwindow* window, int width, int height);
	void window_focus_callback(GLFWwindow* window, int focused);

	// camera
    inline Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
    inline float lastX = (1920 * 0.75f) / 2.0f;
    inline float lastY = (1920 * 0.75f) / 2.0f;
    inline bool firstMouse = true;

    // timing
    inline float deltaTime = 0.0f;	// time between current frame and last frame
    inline float lastFrame = 0.0f;
	
}