#pragma once

#include "../Input/KeyEvent.h"
#include "../Backend/DeltaTime.h"
#include "../Backend/Layer.h"
#include "../Backend/BackendScopeRef.h"
#include "../Renderer/Texture.h"

#include <filesystem>
#include <glm/glm.hpp>

struct StartEditor : Layer
{
	StartEditor();
	virtual ~StartEditor() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	virtual void OnImGuiRender() override;
private:

	void NewProject(const std::string& projectName);
	void OpenProject(const std::filesystem::path& projectPath);
	void DeleteProject(const std::filesystem::path& projectPath);
	std::filesystem::path generalProjectsFolderPath = std::filesystem::current_path() / "Projects";
	Ref<Texture> m_ProjIcon;
private:
	void ProjectsBrowserPanel();
};