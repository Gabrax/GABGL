#pragma once 

#include <cstdint>
#include <memory>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../input/Event.h"


struct WindowDefaultData
{
	std::string title = "";
	uint32_t Width, Height;
	WindowDefaultData(const std::string& windowTitle = "def",
		uint32_t width = 800,
		uint32_t height = 600)
		: title(windowTitle), Width(width), Height(height) {}
};

struct Window 
{
	using EventCallbackFn = std::function<void(Event&)>;

	Window(const WindowDefaultData& data);
	~Window();
	void Update();
	inline uint32_t GetWidth() const { return m_Data.Width; }
	inline uint32_t GetHeight() const { return m_Data.Height; }
	inline GLFWwindow* GetWindowPtr() const { return m_Window; }
	inline void SetEventCallback(const EventCallbackFn& callback) { m_Data.EventCallback = callback; }
	inline bool isClosed() { return m_WindowClosed; }
	void SetVSync(bool enabled);
	bool IsVSync() const;
  void Maximize(bool maximize);
  void SetResolution(uint32_t width, uint32_t height);
	void SetFullscreen(bool full);
	void SetResizable(bool enable);
  void CenterWindowPos();
  void SetCursorVisible(bool enable);

	static std::unique_ptr<Window> Create(const WindowDefaultData& props = WindowDefaultData());

private:
	void Init(const WindowDefaultData& data);
	void Terminate();
private:

	GLFWwindow* m_Window;
  GLFWmonitor* m_Monitor;
  const GLFWvidmode* m_Mode;

  int32_t currWidth, currHeight;
	bool m_WindowClosed = false;

	struct WindowSpecificData
	{
		std::string title;
		uint32_t Width, Height;
		bool VSync;

		EventCallbackFn EventCallback;
	} m_Data;

  void SetWindowIcon(const char* iconpath, GLFWwindow* window);
};
