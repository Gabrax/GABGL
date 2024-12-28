#pragma once

#include "../Input/KeyEvent.h"
#include "../Backend/DeltaTime.h"
#include "../Backend/Layer.h"
#include "../Backend/BackendScopeRef.h"

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

	void CenteredText(const char* text);

private:

	bool m_ViewportFocused = false, m_ViewportHovered = false;
	glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
	glm::vec2 m_ViewportBounds[2];

	// Editor resources
	//Ref<Texture2D> m_IconPlay, m_IconPause, m_IconStep, m_IconSimulate, m_IconStop;
};