#pragma once 

#include "Windowbase.h"
#include "../Renderer/GraphicsContext.h"

struct MainWindow : Window
{
	MainWindow(const WindowDefaultData& data);
	virtual ~MainWindow();
	void Update() override;
	inline uint32_t GetWidth() const override { return m_Data.Width; }
	inline uint32_t GetHeight() const override { return m_Data.Height; }
	inline GLFWwindow* GetWindowPtr() const override { return m_Window; }
	inline void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
	void SetVSync(bool enabled) override;
	bool IsVSync() const override;
	inline bool isClosed() override { return m_WindowClosed; }

private:
	virtual void Init(const WindowDefaultData& data);
	virtual void Terminate() override;
private:

	GLFWwindow* m_Window;
	bool m_WindowClosed = false;
	Scope<GraphicsContext> m_Context;

	struct WindowSpecificData
	{
		std::string title;
		uint32_t Width, Height;
		bool VSync;

		EventCallbackFn EventCallback;
	} m_Data;
};
