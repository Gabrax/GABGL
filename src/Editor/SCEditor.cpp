#include "SCEditor.h"
#include "../Backend/BackendLogger.h"
#include <imgui.h>
#include <imgui_internal.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include "../Engine.h"
#include <json.hpp>

static char projectName[256] = ""; 

StartEditor::StartEditor() : Layer("StartEditor")
{
	m_ProjIcon = Texture::Create("../res/engineTextures/projfileicon.png");
}

void StartEditor::OnAttach(){}

void StartEditor::OnDetach(){}

void StartEditor::OnImGuiRender()
{
	static bool dockspaceOpen = true;
	static bool opt_fullscreen_persistant = true;
	bool opt_fullscreen = opt_fullscreen_persistant;
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_NoTabBar;

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

	style.WindowMinSize.x = minWinSizeX;

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::MenuItem("Create Project...", "Ctrl+N")) ImGui::OpenPopup("Create New Project");
			
			if (ImGui::BeginPopup("Create New Project", ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::Text("Enter the project name:");
				ImGui::InputText("##ProjectName", projectName, sizeof(projectName));

				ImGui::Separator();

				if (ImGui::Button("Create", ImVec2(120, 0))) {

					NewProject(projectName);

					projectName[0] = '\0';

					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(120, 0))) {
					projectName[0] = '\0';

					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			ImGui::EndMenuBar();
		}

		ProjectsBrowserPanel();

	ImGui::End();
}

void StartEditor::NewProject(const std::string& projectName)
{
	namespace fs = std::filesystem;
	try {
		if (fs::exists(generalProjectsFolderPath)) {
			GABGL_WARN("Project folder already exists: {}", generalProjectsFolderPath);
		}
		else {
			fs::create_directory(generalProjectsFolderPath);
			GABGL_INFO("Created project folder: {}", generalProjectsFolderPath);
		}


	}catch (const fs::filesystem_error& e) {
		GABGL_ERROR("Filesystem error: {}", e.what());
	}

	// Create the project folder
	fs::path projectPath = generalProjectsFolderPath / projectName;
	try {
		if (fs::exists(projectPath)) {
			GABGL_WARN("Project folder already exists: {}",projectPath);
			return;
		}

		fs::create_directory(projectPath);
		GABGL_INFO("Created project folder: {}", projectPath);

		fs::path projectFile = projectPath / (projectName + ".proj");

		nlohmann::json projectJson = {
			{"Project", {
				{"Name", projectName},
				{"StartScene", ""},
				{"AssetDirectory", "Assets"}
			}}
		};

		std::ofstream file(projectFile);
		if (file) {
			file << projectJson.dump(4); 
			file.close();
			GABGL_INFO("Created project file: {}", projectFile);
		}
		else {
			GABGL_ERROR("Failed to create project file: {}", projectFile);
		}

		fs::path Path = projectPath / "Assets";
		fs::create_directory(Path);
		GABGL_INFO("Created Assets folder: {}", Path);

		Path = projectPath / "Scenes";
		fs::create_directory(Path);
		GABGL_INFO("Created Scenes folder: {}", Path);
	}
	catch (const fs::filesystem_error& e) {
		GABGL_ERROR("Filesystem error: {}", e.what());
	}
	catch (const std::exception& e) {
		GABGL_ERROR("Unexpected error: {}", e.what());
	}
}

void StartEditor::OpenProject(const std::filesystem::path& projectPath)
{
}

void StartEditor::DeleteProject(const std::filesystem::path& projectPath)
{
	try
	{
		if (std::filesystem::exists(projectPath))
		{
			std::filesystem::remove_all(projectPath); // Attempt deletion
			if (std::filesystem::exists(projectPath))
			{
				throw std::runtime_error("Failed to delete folder: " + projectPath.string());
			}
			GABGL_INFO("Deleted folder: {}",projectPath);
		}
		else
		{
			GABGL_WARN("Folder not found: {}", projectPath);
		}
	}
	catch (const std::exception& e)
	{
		GABGL_ERROR("Error deleting folder: {}", e.what());
	}
}

void StartEditor::ProjectsBrowserPanel()
{
	static std::string folderToDelete; // Stores the name of the folder to delete
	static bool showConfirmationPopup = false;

	ImGui::Begin("Projects", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

	static float padding = 16.0f;
	static float thumbnailSize = 128.0f;
	float cellSize = thumbnailSize + padding;

	float panelWidth = ImGui::GetContentRegionAvail().x;
	int columnCount = (int)(panelWidth / cellSize);
	if (columnCount < 1)
		columnCount = 1;

	ImGui::Columns(columnCount, 0, false);

	for (auto& directoryEntry : std::filesystem::directory_iterator(generalProjectsFolderPath))
	{
		if (!directoryEntry.is_directory())
			continue;

		const auto& path = directoryEntry.path();
		std::string folderName = path.filename().string();

		ImGui::PushID(folderName.c_str());
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

		if (ImGui::ImageButton((ImTextureID)m_ProjIcon->GetRendererID(), { thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 }))
		{
			Engine::GetInstance().SetCurrentProject(path);
			Engine::GetInstance().GetStartWindow().Terminate();
		}

		if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
		{
			ImGui::OpenPopup("Folder Options");
			folderToDelete = path.string(); 
		}
		
		ImGui::TextWrapped(folderName.c_str());
		ImGui::PopStyleColor();

		if (ImGui::BeginPopup("Folder Options"))
		{
			if (ImGui::MenuItem("Delete"))
			{
				showConfirmationPopup = true;
			}
			ImGui::EndPopup();
		}

		ImGui::NextColumn();
		ImGui::PopID();
	}

	ImGui::Columns(1);

	ImGui::End();

	// Confirmation popup
	if (showConfirmationPopup)
	{
		ImGui::OpenPopup("Confirm Delete");
		showConfirmationPopup = false;
	}

	if (ImGui::BeginPopupModal("Confirm Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Are you sure you want to delete this folder?");
		ImGui::Separator();

		if (ImGui::Button("Yes", ImVec2(120, 0)))
		{
			DeleteProject(folderToDelete); // Delete the folder
			folderToDelete.clear();       // Clear the folder path
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("No", ImVec2(120, 0)))
		{
			folderToDelete.clear(); // Clear the folder path
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}


