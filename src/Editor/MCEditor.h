#pragma once

#include "../Input/KeyEvent.h"
#include "../Backend/DeltaTime.h"
#include "../Backend/Layer.h"
#include "../Backend/BackendScopeRef.h"
#include "../Renderer/Texture.h"

#include <filesystem>
#include <glm/glm.hpp>

struct MainEditor : Layer
{
	MainEditor();
	virtual ~MainEditor() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	virtual void OnImGuiRender() override;
	void OnEvent(Event& e) override;
private:
	void ReloadProject();
	void SaveProject();
private:
	void ViewportPanel();
	void SceneHierarchyPanel();
	void ComponentsPanel();
	void ContentBrowserPanel();
	void DebugProfilerPanel();
	void CenteredText(const char* text);
private:

	std::filesystem::path m_BaseDirectory;
	std::filesystem::path m_CurrentDirectory;
	bool m_ViewportFocused = false, m_ViewportHovered = false;
	glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
	glm::vec2 m_ViewportBounds[2];

	Ref<Texture> m_FolderIcon, m_FileIcon;
};