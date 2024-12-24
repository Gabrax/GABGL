#pragma once

#include <cstdint>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

struct WindowProps
{
	std::string title = "";
	uint32_t Width, Height;
	WindowProps(const std::string& windowTitle = "GABGL",
		uint32_t width = 1280,
		uint32_t height = 720)
		: title(windowTitle), Width(width), Height(height) {}
};


struct Window
{
	virtual ~Window() = default;
	virtual void Update() = 0;
	virtual uint32_t GetWidth() const = 0;
	virtual uint32_t GetHeight() const = 0;
	//virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
	virtual void SetVSync(bool enabled) = 0;
	virtual bool IsVSync() const = 0;

	static Window* Create(const WindowProps& props = WindowProps());
};
