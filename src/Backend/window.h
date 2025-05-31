#pragma once 

#include "windowbase.h"

struct Window : WindowBase
{
	Window(const WindowDefaultData& data);
	virtual ~Window();
	void Update() override;
	inline uint32_t GetWidth() const override { return m_Data.Width; }
	inline uint32_t GetHeight() const override { return m_Data.Height; }
	inline GLFWwindow* GetWindowPtr() const override { return m_Window; }
	inline void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
	void SetVSync(bool enabled) override;
	bool IsVSync() const override;
	inline bool isClosed() override { return m_WindowClosed; }
  void Maximize(bool maximize) override;
  void SetResolution(uint32_t width, uint32_t height) override;
	void SetFullscreen(bool full) override;
	void SetResizable(bool enable);
  void CenterWindowPos() override;

private:
	virtual void Init(const WindowDefaultData& data);
	virtual void Terminate() override;
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
};
