#pragma once

#include "../input/KeyEvent.h"
#include "../backend/DeltaTime.h"
#include "../backend/Texture.h"
/*#include "../Scene/Scene.h"*/
#include "../backend/FrameBuffer.h"
/*#include "../Scene/Entity.hpp"*/
#include "../backend/windowbase.h"

#include <filesystem>
#include <glm/glm.hpp>

struct Editor
{
  Editor();
	~Editor();

	void OnImGuiRender(uint32_t framebufferTexture);
	void OnEvent(Event& e);
private:
	bool OnKeyPressed(KeyPressedEvent& e);
	bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
private:
	void ReloadProject();
	void SaveProject();
private:
	void ViewportPanel(uint32_t framebufferTexture);
	void SceneHierarchyPanel();
	void ComponentsPanel();
	void CenteredText(const char* text);
	/*void DrawEntityNode(Entity entity);*/
	/*void DrawComponents(Entity entity);*/
	template<typename T>
	void DisplayAddComponentEntry(const std::string& entryName);
private:

	std::shared_ptr<Framebuffer> m_Framebuffer;
	int m_GizmoType;
	/*Entity m_SelectionContext;*/
	/*Entity m_HoveredEntity;*/
	std::vector<std::string> sceneFiles;
	int selectedSceneIndex = -1;
	bool m_ViewportFocused = false, m_ViewportHovered = false;
	glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
	glm::vec2 m_ViewportBounds[2];

	void Begin();
	void End();
	void BlockEvents(bool block) { m_BlockEvents = block; }
	void SetDarkThemeColors();
	uint32_t GetActiveWidgetID() const;
private:
	WindowBase* m_WindowRef;
	bool m_BlockEvents = true;
};
