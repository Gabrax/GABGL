#pragma once

#include "Windowbase.h"
#include "BackendScopeRef.h"
#include "../Input/EngineEvent.h"
#include "LayerStack.h"
#include "../Editor/ImGuiLayer.h"

struct Engine
{
	Engine();
	virtual ~Engine();
	void Run();
	void OnEvent(Event& e);
	inline static Engine& GetInstance() { return *s_Instance; }
	Window& GetStartWindow() { return *m_StartWindow; }
	Window& GetMainWindow() { return *m_MainWindow; }
private:
	bool OnWindowClose(WindowCloseEvent& e);
	bool OnWindowResize(WindowResizeEvent& e);
	Scope<Window> m_StartWindow;
	Scope<Window> m_MainWindow;
	ImGuiLayer* m_ImGuiLayer;
	LayerStack m_LayerStack;
	void PushLayer(Layer* layer);
	void PushOverlay(Layer* layer);
	float m_LastFrameTime = 0.0f;
	bool m_Minimized = false;
	bool m_isRunning = true;
private:
	static Engine* s_Instance;
};