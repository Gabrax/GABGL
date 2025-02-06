#pragma once

#include "../Input/KeyEvent.h"
#include "../Backend/DeltaTime.h"
#include "../Backend/Layer.h"
#include "../Backend/BackendScopeRef.h"
#include "../Renderer/Texture.h"
#include "../Scene/Scene.h"
#include "../Renderer/FrameBuffer.h"
#include "../Scene/Entity.hpp"

#include <filesystem>
#include <glm/glm.hpp>

struct MainEditor : Layer
{
	MainEditor();
	virtual ~MainEditor() = default;

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
	void ContentBrowserPanel();
	bool IsProtectedFolder(const std::filesystem::path& path);
	void DeleteFileOrFolder(const std::filesystem::path& path);
	void DebugProfilerPanel();
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

	Ref<Framebuffer> m_Framebuffer;
	int m_GizmoType;
	Entity m_SelectionContext;
	Entity m_HoveredEntity;
	Ref<Scene> m_ActiveScene;
	Ref<Scene> m_EditorScene;
	bool m_PrimaryCamera = true;
	EditorCamera m_EditorCamera;
	std::filesystem::path m_BaseDirectory;
	std::filesystem::path m_CurrentDirectory;
	std::vector<std::string> sceneFiles;
	int selectedSceneIndex = -1;
	bool m_ViewportFocused = false, m_ViewportHovered = false;
	glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
	glm::vec2 m_ViewportBounds[2];

	Ref<Texture> m_FolderIcon, m_FileIcon;
};