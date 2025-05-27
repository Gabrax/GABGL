#pragma once

#include "../input/KeyEvent.h"
#include "../backend/DeltaTime.h"
#include "../backend/Layer.h"
#include "../backend/Texture.h"
#include "../Scene/Scene.h"
#include "../backend/FrameBuffer.h"
#include "../Scene/Entity.hpp"

#include <filesystem>
#include <glm/glm.hpp>

struct Editor : Layer
{
	Editor();
	virtual ~Editor() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(DeltaTime dt) override;
	virtual void OnImGuiRender() override;
	void OnEvent(Event& e) override;
private:
	bool OnKeyPressed(KeyPressedEvent& e);
	bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
private:
	void ReloadProject();
	void SaveProject();
private:
	void ViewportPanel();
	void SceneHierarchyPanel();
	void ComponentsPanel();
	void CenteredText(const char* text);
	void DrawEntityNode(Entity entity);
	void DrawComponents(Entity entity);
	template<typename T>
	void DisplayAddComponentEntry(const std::string& entryName);
private:

	enum class SceneState
	{
		Edit = 0, Play = 1, Simulate = 2
	};
	SceneState m_SceneState = SceneState::Edit;

	std::shared_ptr<Framebuffer> m_Framebuffer;
	int m_GizmoType;
	Entity m_SelectionContext;
	Entity m_HoveredEntity;
	std::shared_ptr<Scene> m_ActiveScene;
	std::shared_ptr<Scene> m_EditorScene;
	bool m_PrimaryCamera = true;
	Camera m_EditorCamera;
	std::vector<std::string> sceneFiles;
	int selectedSceneIndex = -1;
	bool m_ViewportFocused = false, m_ViewportHovered = false;
	glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
	glm::vec2 m_ViewportBounds[2];

	std::shared_ptr<Texture> m_FolderIcon, m_FileIcon;
};
