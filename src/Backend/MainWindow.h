#pragma once 

#include "Windowbase.h"

struct MainWindow : Window
{
	MainWindow(const WindowDefaultData& data);
	virtual ~MainWindow();
	void Update() override;
	inline uint32_t GetWidth() const override { return m_Data.Width; }
	inline uint32_t GetHeight() const override { return m_Data.Height; }
	inline void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
	void SetVSync(bool enabled) override;
	bool IsVSync() const override;

private:
	virtual void Init(const WindowDefaultData& data);
	virtual void Terminate() override;
private:

  GLFWwindow* m_Window;

  struct WindowSpecificData
  {
    std::string title;
    uint32_t Width, Height;
    bool VSync;

    EventCallbackFn EventCallback;
  };

  WindowSpecificData m_Data;
};
