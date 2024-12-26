#pragma once

#include <cstdint>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../Input/Event.h"
#include "BackendScopeRef.h"
#include <stb_image.h>

struct WindowDefaultData
{
	std::string title = "";
	uint32_t Width, Height;
	WindowDefaultData(const std::string& windowTitle = "GABGL",
		uint32_t width = 1280,
		uint32_t height = 720)
		: title(windowTitle), Width(width), Height(height) {}
};

struct Window
{
	using EventCallbackFn = std::function<void(Event&)>;

	virtual ~Window() = default;
	virtual void Terminate() = 0;
	virtual void Update() = 0;
	virtual uint32_t GetWidth() const = 0;
	virtual uint32_t GetHeight() const = 0;
	virtual void* GetNativeWindow() const = 0;
	virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
	virtual bool IsVSync() const = 0;
	virtual void SetVSync(bool enabled) = 0;

	inline void SetWindowIcon(const char* iconpath, GLFWwindow* window)
	{
		GLFWimage images[1];
		images[0].pixels = stbi_load(iconpath, &images[0].width, &images[0].height, 0, 4);
		if (images[0].pixels) {
			glfwSetWindowIcon(window, 1, images);
			stbi_image_free(images[0].pixels);
		}
	}
	template<typename T>
	inline static Scope<Window> Create(const WindowDefaultData& props = WindowDefaultData())
	{
		return CreateScope<T>(props);
	}
};
