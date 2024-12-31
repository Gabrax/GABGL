#include "MCEditor.h"
#include "../Backend/BackendLogger.h"
#include <imgui.h>
#include <imgui_internal.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include "../Engine.h"

MainEditor::MainEditor() : Layer("MainEditor"), m_BaseDirectory(Engine::GetInstance().GetCurrentProjectPath()), m_CurrentDirectory(m_BaseDirectory)
{
	m_FolderIcon = Texture::Create("../res/engineTextures/foldericon.png");
	m_FileIcon = Texture::Create("../res/engineTextures/projfileicon.png");
}

void MainEditor::OnAttach()
{

}

void MainEditor::OnDetach()
{

}

void MainEditor::OnImGuiRender()
{
	GABGL_PROFILE_SCOPE("GAB");
	// Note: Switch this to true to enable dockspace
	static bool dockspaceOpen = true;
	static bool opt_fullscreen_persistant = true;
	bool opt_fullscreen = opt_fullscreen_persistant;
	static ImGuiDockNodeFlags dockspace_flags = ImGuiWindowFlags_NoCollapse | ImGuiDockNodeFlags_NoTabBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

	// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
	// because it would be confusing to have two docking targets within each others.
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	if (opt_fullscreen)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	}

	// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
	if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
		window_flags |= ImGuiWindowFlags_NoBackground;

	// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
	// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
	// all active windows docked into it will lose their parent and become undocked.
	// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
	// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
	ImGui::PopStyleVar();
	if (opt_fullscreen)
		ImGui::PopStyleVar(2);

	// DockSpace
	ImGuiIO& io = ImGui::GetIO();
	ImGuiStyle& style = ImGui::GetStyle();
	float minWinSizeX = style.WindowMinSize.x;
	style.WindowMinSize.x = 370.0f;
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	}

	Engine& instance = Engine::GetInstance();

	style.WindowMinSize.x = minWinSizeX;

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("ReloadProject", "Ctrl+R")) ReloadProject();
			if (ImGui::MenuItem("SaveProject", "Ctrl+S")) SaveProject();

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
	
	SceneHierarchyPanel();

	ComponentsPanel();

	ImGui::ShowDebugLogWindow();

	ContentBrowserPanel();

	DebugProfilerPanel();

	ViewportPanel();

	ImGui::End();
}

void MainEditor::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);
	//dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT(MainEditor::OnKeyPressed));
	//dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT(MainEditor::OnMouseButtonPressed));
}

void MainEditor::ReloadProject()
{

}

void MainEditor::SaveProject()
{

}

void MainEditor::ViewportPanel()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
	ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoMove);
	auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
	auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
	auto viewportOffset = ImGui::GetWindowPos();
	m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
	m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

	m_ViewportFocused = ImGui::IsWindowFocused();
	m_ViewportHovered = ImGui::IsWindowHovered();

	Engine::GetInstance().GetImGuiLayer()->BlockEvents(!m_ViewportHovered);

	ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
	m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

	ImGui::End();
	ImGui::PopStyleVar();
}

void MainEditor::SceneHierarchyPanel()
{
	ImGui::Begin("Scene Hierarchy", nullptr, ImGuiWindowFlags_NoCollapse);

	CenteredText("Scene Hierarchy");

	ImGui::End();
}

void MainEditor::ComponentsPanel()
{
	ImGui::Begin("Components", nullptr, ImGuiWindowFlags_NoCollapse);

	CenteredText("Components");

	ImGui::End();
}


void MainEditor::ContentBrowserPanel()
{
	ImGui::Begin("Content Browser",nullptr, ImGuiWindowFlags_NoCollapse);
	ImGui::BeginGroup();
	if (m_CurrentDirectory != std::filesystem::path(m_BaseDirectory))
	{
		if (ImGui::Button("<"))
		{
			m_CurrentDirectory = m_CurrentDirectory.parent_path();
		}
	}
	ImGui::SameLine();
	CenteredText("Content Browser");
	ImGui::EndGroup();

	static float padding = 16.0f;
	static float thumbnailSize = 128.0f;
	float cellSize = thumbnailSize + padding;

	float panelWidth = ImGui::GetContentRegionAvail().x;
	int columnCount = (int)(panelWidth / cellSize);
	if (columnCount < 1)
		columnCount = 1;

	ImGui::Columns(columnCount, 0, false);

	for (auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
	{
		const auto& path = directoryEntry.path();
		std::string filenameString = path.filename().string();

		if (!directoryEntry.is_directory() && path.extension() == ".proj")
			continue;

		ImGui::PushID(filenameString.c_str());
		Ref<Texture> icon = directoryEntry.is_directory() ? m_FolderIcon : m_FileIcon;
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::ImageButton((ImTextureID)icon->GetRendererID(), { thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 });

		if (ImGui::BeginDragDropSource())
		{
			std::filesystem::path relativePath(path);
			const wchar_t* itemPath = relativePath.c_str();
			ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath, (wcslen(itemPath) + 1) * sizeof(wchar_t));
			ImGui::EndDragDropSource();
		}

		ImGui::PopStyleColor();
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			if (directoryEntry.is_directory())
				m_CurrentDirectory /= path.filename();

		}
		ImGui::TextWrapped(filenameString.c_str());

		ImGui::NextColumn();

		ImGui::PopID();
	}

	ImGui::Columns(1);

	ImGui::End();
}

void MainEditor::DebugProfilerPanel()
{
	ImGui::Begin("Debug Instrumentation", nullptr, ImGuiWindowFlags_NoCollapse);

	CenteredText("Debug Instrumentation");
	for (auto& result : s_ProfileResults)
	{
		char label[50];
		std::strcpy(label, result.Name);
		std::strcat(label, " %.3fms");
		ImGui::Text(label, result.Time);
	}
	s_ProfileResults.clear();

	ImGui::End();
}

void MainEditor::CenteredText(const char* text) {
	ImVec2 windowSize = ImGui::GetWindowSize();
	ImVec2 textSize = ImGui::CalcTextSize(text);

	float centeredX = (windowSize.x - textSize.x) / 2.0f;
	ImGui::SetCursorPosX(centeredX);
	ImGui::Text("%s", text);
}

