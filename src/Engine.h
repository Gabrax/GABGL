#pragma once

#include "Backend/Windowbase.h"
#include "Backend/BackendScopeRef.h"
#include "Input/EngineEvent.h"
#include "Backend/LayerStack.h"
#include "Editor/ImGuiLayer.h"
#include "Editor/SCEditor.h"
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
	inline void SetCurrentProject(const std::filesystem::path& projectPath) { m_CurrentProjectName = projectPath.filename().string(); m_CurrentProjectPath = projectPath; }
	inline const std::filesystem::path& GetCurrentProjectPath() const { return m_CurrentProjectPath; }
	inline const std::string& GetCurrentProject() const { return m_CurrentProjectName; }
private:
	bool OnWindowClose(WindowCloseEvent& e);
	bool OnWindowResize(WindowResizeEvent& e);
	void CleanupStartWindow();
	void RenderLayers(DeltaTime& dt);
	void RenderEditorLayers();
	Scope<Window> m_StartWindow;
	Scope<Window> m_MainWindow;
	ImGuiLayer* m_ImGuiLayer;
	StartEditor* m_StartEditorlayer;
	MainEditor* m_MainEditorlayer;
	LayerStack m_LayerStack;
	void PushLayer(Layer* layer);
	void PushOverlay(Layer* layer);
	float m_LastFrameTime = 0.0f;
	bool m_Minimized = false;
	bool m_isRunning = true;
	bool closed = false;
private:
	std::filesystem::path m_CurrentProjectPath;
	std::string m_CurrentProjectName;
	enum class EngineState
	{
		StartWindow, MainWindow, Exiting

	} appState;

	static Engine* s_Instance;
};