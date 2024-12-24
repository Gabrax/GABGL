#pragma once

#include <cstdint>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../Input/Event.h"
#include "BackendScopeRef.h"

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
	virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
	virtual void SetVSync(bool enabled) = 0;
	virtual bool IsVSync() const = 0;

	template<typename T>
	inline static Scope<Window> Create(const WindowDefaultData& props = WindowDefaultData())
	{
		return CreateScope<T>(props);
	}
};
