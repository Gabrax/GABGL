#pragma once

#include <memory>

#include "backend/window.h"
#include "input/EngineEvent.h"
#include "backend/LayerStack.h"
#include "backend/DeltaTime.h"
#include "game/game.h"
#include "game/Editor.h"

struct Engine
{
  Engine(); 
	virtual ~Engine();
	void Run();
  void OnEvent(Event& e);
	inline static Engine& GetInstance() { return *s_Instance; }
	inline WindowBase& GetMainWindow() { return *m_Window; }
private:

  std::unique_ptr<WindowBase> m_Window;
  bool OnWindowClose(WindowCloseEvent& e);
	bool OnWindowResize(WindowResizeEvent& e);

  LayerStack m_LayerStack;
	void PushLayer(Layer* layer);
	void PushOverlay(Layer* layer);
  void RenderLayers(DeltaTime& dt);
  void RenderEditorLayers();

  GAME* m_Game;

	float m_LastFrameTime = 0.0f;
	bool m_Minimized = false;
	bool m_isRunning = true;
	bool m_closed = false;
private:

	static Engine* s_Instance;
};
