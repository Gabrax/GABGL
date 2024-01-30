#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>


namespace Window
{
	void Init(int width, int height);
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
	void CreateWindow(WindowMode windowMode);
	void SetWindowMode(WindowMode windowMode);
	void ToggleFullscreen();
	int GetScrollWheelYOffset();
	void ResetScrollWheelYOffset();
	void processInput(GLFWwindow* window);
	void framebuffer_size_callback(GLFWwindow* window, int width, int height);
	void window_focus_callback(GLFWwindow* window, int focused);
	void scroll_callback(GLFWwindow* window, double /*xoffset*/, double yoffset);
}

