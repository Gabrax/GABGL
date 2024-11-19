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
	void DisableCursor();
	void HideCursor();
	void ShowCursor();
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
	void ResetScrollWheelYOffset();
	void processInput(GLFWwindow* window);
	void framebuffer_size_callback(GLFWwindow* window, int width, int height);
	void window_focus_callback(GLFWwindow* window, int focused);
	void DeltaTime();

  // GETTERS // 
  float getDeltaTime();
  float getAspectRatio();
	int GetScrollWheelYOffset();
	GLFWwindow* GetWindowPtr();
	int GetCursorScreenX();
	int GetCursorScreenY();
	int GetWindowWidth();
	int GetWindowHeight();
  int GetWindowedWidth();
  int GetWindowedHeight();
  int GetFullscreenWidth();
  int GetFullscreenHeight();
	int GetCursorX();
	int GetCursorY();

	// camera
  inline Camera _camera(glm::vec3(0.20f, 31.33f, 34.65f));
  inline float _lastX = static_cast<float>(GetWindowWidth()) / 2.0f;
  inline float _lastY = static_cast<float>(GetWindowHeight()) / 2.0f;
  inline bool _firstMouse = true;

}
