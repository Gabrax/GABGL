#pragma once 

#include "Windowbase.h"

struct StartWindow : Window
{
	StartWindow(const WindowDefaultData& data);
	virtual ~StartWindow();
	void Update() override;
	inline uint32_t GetWidth() const override  { return m_Data.Width; }
	inline uint32_t GetHeight() const override { return m_Data.Height; }
	inline void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
	void SetVSync(bool enabled) override;
	bool IsVSync() const override;
	inline static bool isGLFWInit() { return m_GLFWInitialized; }
	inline static bool isGLADInit() { return m_GLADInitialized; }
	inline static bool isClosed()   { return m_WindowClosed; }

private:
	virtual void Init(const WindowDefaultData& data);
	virtual void Terminate() override;
private:

	GLFWwindow* m_Window;
	static bool m_GLFWInitialized;
	static bool m_GLADInitialized;
	static bool m_WindowClosed;

	struct WindowSpecificData
	{
		std::string title;
		uint32_t Width, Height;
		bool VSync;

		EventCallbackFn EventCallback;
	};

	WindowSpecificData m_Data;
};
