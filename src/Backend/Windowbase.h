#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../input/Event.h"
#include <stb_image.h>

struct WindowDefaultData
{
	std::string title = "";
	uint32_t Width, Height;
	WindowDefaultData(const std::string& windowTitle = "def",
		uint32_t width = 800,
		uint32_t height = 600)
		: title(windowTitle), Width(width), Height(height) {}
};

struct WindowBase
{
	using EventCallbackFn = std::function<void(Event&)>;

	virtual ~WindowBase() = default;
	virtual void Terminate() = 0;
	virtual void Update() = 0;
	virtual uint32_t GetWidth() const = 0;
	virtual uint32_t GetHeight() const = 0;
	virtual void SetResolution(uint32_t width, uint32_t height) = 0;
	virtual void SetFullscreen(bool full) = 0;
	virtual GLFWwindow* GetWindowPtr() const = 0;
	virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
	virtual bool IsVSync() const = 0;
	virtual void SetVSync(bool enabled) = 0;
  virtual void Maximize(bool maximize) = 0;
  virtual void CenterWindowPos() = 0;
	virtual bool isClosed() = 0;

	inline void SetWindowIcon(const char* iconpath, GLFWwindow* window)
	{
		stbi_set_flip_vertically_on_load(0);
		GLFWimage images[1];
		images[0].pixels = stbi_load(iconpath, &images[0].width, &images[0].height, 0, 4);
		if (images[0].pixels) {
			glfwSetWindowIcon(window, 1, images);
			stbi_image_free(images[0].pixels);
		}
	}

	template<typename T>
	inline static std::unique_ptr<WindowBase> Create(const WindowDefaultData& props = WindowDefaultData())
	{
		return std::make_unique<T>(props);
	}
};
