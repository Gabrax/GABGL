#pragma once

#include "Backend/Windowbase.h"
#include "Backend/BackendScopeRef.h"
#include "Input/EngineEvent.h"
#include "Backend/LayerStack.h"
#include "Editor/ImGuiLayer.h"
#include "Editor/MCEditor.h"

struct Engine
{
	Engine();
	virtual ~Engine();
	void Run();
	void OnEvent(Event& e);
	inline static Engine& GetInstance() { return *s_Instance; }
	inline ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }
	inline Window& GetStartWindow() { return *m_StartWindow; }
	inline Window& GetMainWindow() { return *m_MainWindow; }
private:
	bool OnWindowClose(WindowCloseEvent& e);
	bool OnWindowResize(WindowResizeEvent& e);
	void SetupMainWindow();
	void RenderLayers(DeltaTime& dt);
	void RenderEditorLayers();
	Scope<Window> m_StartWindow;
	Scope<Window> m_MainWindow;
	ImGuiLayer* m_ImGuiLayer;
	MainEditor* m_MainEditorlayer;
	LayerStack m_LayerStack;
	void PushLayer(Layer* layer);
	void PushOverlay(Layer* layer);
	float m_LastFrameTime = 0.0f;
	bool m_Minimized = false;
	bool m_isRunning = true;
	bool closed = false;
private:
	enum class EngineState
	{
		StartWindow, MainWindow, Exiting

	} appState;

	static Engine* s_Instance;
};