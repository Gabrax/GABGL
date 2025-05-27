#pragma once

#include "../Backend/Layer.h"
#include "../input/EngineEvent.h"
#include "../input/KeyEvent.h"
#include "../backend/Windowbase.h"
// TEMPORARY
#include <GLFW/glfw3.h>
#include <glad/glad.h>


struct ImGuiLayer : Layer
{
	template<typename T>
	ImGuiLayer(T* t) : Layer("ImGuiLayer"), m_Window(t) {};
	~ImGuiLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnEvent(Event& e) override;

	void Begin();
	void End();

	void BlockEvents(bool block) { m_BlockEvents = block; }
		
	void SetDarkThemeColors();

	uint32_t GetActiveWidgetID() const;
private:
	GLFWwindow* window;
	WindowBase* m_Window;
	bool m_BlockEvents = true;
};

