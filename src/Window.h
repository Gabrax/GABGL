#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Camera.h"


namespace Window {

	void Init();
	void ShowFPS();
	void ProcessInput();
	void BeginFrame();
	void EndFrame();
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
	void DeltaTime();
  float getDeltaTime();
  float getAspectRatio();

	// camera
  inline Camera _camera(glm::vec3(15.61f, 3.98f, 16.21f));
  inline float _lastX = static_cast<float>(GetWindowWidth()) / 2.0f;
  inline float _lastY = static_cast<float>(GetWindowHeight()) / 2.0f;
  inline bool _firstMouse = true;

}
