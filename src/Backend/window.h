#pragma once 

#include <cstdint>
#include <memory>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../input/Event.h"
#include "../input/EngineEvent.h"

struct Window 
{
	using EventCallbackFn = std::function<void(Event&)>;

  static void Init(const std::string& windowTitle, uint32_t windowWidth, uint32_t windoHeight);
  static void Terminate();
	static void Update();
	static uint32_t GetWidth();
	static uint32_t GetHeight();
	static GLFWwindow* GetWindowPtr();
	static void SetEventCallback(const EventCallbackFn& callback);
	static bool isClosed();
	static void SetVSync(bool enabled);
	static bool IsVSync();
  static void Maximize(bool maximize);
  static void SetResolution(uint32_t width, uint32_t height);
	static void SetFullscreen(bool full);
	static void SetResizable(bool enable);
  static void CenterWindowPos();
  static void SetCursorVisible(bool enable);
  static bool IsRunning();
  static bool IsMinimized();

private:

  static void SetWindowIcon(const char* iconpath, GLFWwindow* window);
  static void OnEvent(Event& e);
  static bool OnWindowClose(WindowCloseEvent& e);
  static bool OnWindowResize(WindowResizeEvent& e);
};
