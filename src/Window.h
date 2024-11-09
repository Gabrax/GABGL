#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Camera.h"


namespace Window {
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
	void DeltaTime();

	// window attributes
	inline GLFWwindow* _window;
    inline GLFWmonitor* _monitor;
    inline const GLFWvidmode* _mode;
    inline int _currentWidth = 0;
    inline int _currentHeight = 0;
    inline int _windowedWidth = 1920 * 1.5f;
    inline int _windowedHeight = 1080 * 1.5f;
    inline int _fullscreenWidth = 0;
    inline int _fullscreenHeight = 0;
    inline int _mouseScreenX = 0;
    inline int _mouseScreenY = 0;
    inline int _windowHasFocus = true;
    inline bool _forceCloseWindow = false;
    inline int _scrollWheelYOffset = 0;
    inline enum WindowMode _windowMode = WINDOWED;// FULLSCREEN;
    inline enum RenderMode _renderMode = WIREFRAME;
    inline double prevTime = 0.0;
    inline double crntTime = 0.0;
    inline double timeDiff;
    inline unsigned int counter = 0;
    inline int windowPosX = (_windowedWidth - _windowedWidth) / 2;
    inline int windowPosY = (_windowedHeight - _windowedHeight) / 2;
	inline float _aspectRatio = static_cast<float>(_windowedWidth) / static_cast<float>(_windowedHeight);

	// camera
    inline Camera _camera(glm::vec3(15.61f, 3.98f, 16.21f));
    inline float _lastX = static_cast<float>(_currentWidth) / 2.0f;
    inline float _lastY = static_cast<float>(_currentHeight) / 2.0f;
    inline bool _firstMouse = true;

    // timing
    inline float _deltaTime = 0.0f;	// time between current frame and last frame
    inline float _lastFrame = 0.0f;
}
