#include "Editor.h"
#include "../backend/BackendLogger.h"
#include <imgui.h>
#include <imgui_internal.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include "ImGuizmo.h"
#include <glm/gtc/type_ptr.hpp>
/*#include "../Engine.h"*/
/*#include "../Renderer/Renderer2D.h"*/
/*#include "../Input/UserInput.h"*/
/*#include "../Backend/Utils.hpp"*/
/*#include "../Renderer/RendererAPI.h"*/
/*#include "json.hpp"*/
/**/
/*Editor::Editor() : Layer("MainEditor"), m_GizmoType(ImGuizmo::OPERATION::TRANSLATE)*/
/*{*/
/*	m_FolderIcon = Texture::Create("../res/engineTextures/foldericon.png");*/
/*	m_FileIcon = Texture::Create("../res/engineTextures/projfileicon.png");*/
/*}*/
/**/
/*void Editor::OnAttach()*/
/*{*/
/*	FramebufferSpecification fbSpec;*/
/*	fbSpec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };*/
/*	fbSpec.Width = Engine::GetInstance().GetMainWindow().GetWidth();*/
/*	fbSpec.Height = Engine::GetInstance().GetMainWindow().GetHeight();*/
/*	m_Framebuffer = Framebuffer::Create(fbSpec);*/
/**/
/*	m_EditorScene = CreateRef<Scene>();*/
/*	m_ActiveScene = m_EditorScene;*/
/*	m_EditorCamera = EditorCamera(30.0f, 1.778f, 0.1f, 1000.0f);*/
/*	Renderer2D::SetLineWidth(4.0f);*/
/*}*/
/**/
/*void Editor::OnDetach()*/
/*{*/
/**/
/*}*/
/**/
/*void Editor::OnUpdate(DeltaTime dt)*/
/*{*/
/*	m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);*/
/**/
/*	// Resize*/
/*	if (FramebufferSpecification spec = m_Framebuffer->GetSpecification();*/
/*		m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f && // zero sized framebuffer is invalid*/
/*		(spec.Width != m_ViewportSize.x || spec.Height != m_ViewportSize.y))*/
/*	{*/
/*		m_Framebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);*/
/*		//m_CameraController.OnResize(m_ViewportSize.x, m_ViewportSize.y);*/
/*		m_EditorCamera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);*/
/*	}*/
/*	Renderer2D::ResetStats();*/
/*	m_Framebuffer->Bind();*/
/*	RendererAPI::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });*/
/*	RendererAPI::Clear();*/
/*	m_Framebuffer->ClearAttachment(1, -1);*/
/**/
/*	switch (m_SceneState)*/
/*	{*/
/*		case SceneState::Edit:*/
/*		{*/
/*			//if (m_ViewportFocused)*/
/*				//m_CameraController.OnUpdate(ts);*/
/**/
/*			m_EditorCamera.OnUpdate(dt);*/
/**/
/*			m_ActiveScene->OnUpdateEditor(dt, m_EditorCamera);*/
/*			break;*/
/*		}*/
/*		case SceneState::Simulate:*/
/*		{*/
/*			m_EditorCamera.OnUpdate(dt);*/
/**/
/*			m_ActiveScene->OnUpdateSimulation(dt, m_EditorCamera);*/
/*			break;*/
/*		}*/
/*		case SceneState::Play:*/
/*		{*/
/*			m_ActiveScene->OnUpdateRuntime(dt);*/
/*			break;*/
/*		}*/
/*	}*/
/**/
/*	auto [mx, my] = ImGui::GetMousePos();*/
/*	mx -= m_ViewportBounds[0].x;*/
/*	my -= m_ViewportBounds[0].y;*/
/*	glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];*/
/*	my = viewportSize.y - my;*/
/*	int mouseX = (int)mx;*/
/*	int mouseY = (int)my;*/
/**/
/*	if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y)*/
/*	{*/
/*		int pixelData = m_Framebuffer->ReadPixel(1, mouseX, mouseY);*/
/*		m_HoveredEntity = pixelData == -1 ? Entity() : Entity((entt::entity)pixelData, m_ActiveScene.get());*/
/*	}*/
/**/
/*	m_Framebuffer->Unbind();*/
/*}*/
/**/
/*void Editor::OnImGuiRender()*/
/*{*/
/*	// Note: Switch this to true to enable dockspace*/
/*	static bool dockspaceOpen = true;*/
/*	static bool opt_fullscreen_persistant = true;*/
/*	bool opt_fullscreen = opt_fullscreen_persistant;*/
/*	static ImGuiDockNodeFlags dockspace_flags = ImGuiWindowFlags_NoCollapse | ImGuiDockNodeFlags_NoTabBar;*/
/**/
/*	// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,*/
/*	// because it would be confusing to have two docking targets within each others.*/
/*	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;*/
/*	if (opt_fullscreen)*/
/*	{*/
/*		ImGuiViewport* viewport = ImGui::GetMainViewport();*/
/*		ImGui::SetNextWindowPos(viewport->Pos);*/
/*		ImGui::SetNextWindowSize(viewport->Size);*/
/*		ImGui::SetNextWindowViewport(viewport->ID);*/
/*		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);*/
/*		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);*/
/*		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;*/
/*		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;*/
/*	}*/
/**/
/*	// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.*/
/*	if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)*/
/*		window_flags |= ImGuiWindowFlags_NoBackground;*/
/**/
/*	// Important: note that we proceed even if Begin() returns false (aka window is collapsed).*/
/*	// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, */
/*	// all active windows docked into it will lose their parent and become undocked.*/
/*	// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise */
/*	// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.*/
/*	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));*/
/*	ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);*/
/*	ImGui::PopStyleVar();*/
/*	if (opt_fullscreen)*/
/*		ImGui::PopStyleVar(2);*/
/**/
/*	// DockSpace*/
/*	ImGuiIO& io = ImGui::GetIO();*/
/*	ImGuiStyle& style = ImGui::GetStyle();*/
/*	float minWinSizeX = style.WindowMinSize.x;*/
/*	style.WindowMinSize.x = 370.0f;*/
/*	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)*/
/*	{*/
/*		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");*/
/*		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);*/
/*	}*/
/**/
/*	style.WindowMinSize.x = minWinSizeX;*/
/**/
/*	// PANELS //*/
/*	SceneHierarchyPanel();*/
/**/
/*	ComponentsPanel();*/
/**/
/*	ViewportPanel();*/
/*	// PANELS //*/
/**/
/*	ImGui::End();*/
/*}*/
/**/
/*void Editor::OnEvent(Event& e)*/
/*{*/
/*	//m_CameraController.OnEvent(e);*/
/*	if (m_SceneState == SceneState::Edit)*/
/*	{*/
/*		m_EditorCamera.OnEvent(e);*/
/*	}*/
/**/
/*	EventDispatcher dispatcher(e);*/
/*	dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT(Editor::OnKeyPressed));*/
/*	dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT(Editor::OnMouseButtonPressed));*/
/*}*/
/**/
/*bool Editor::OnKeyPressed(KeyPressedEvent& e)*/
/*{*/
/*	// Shortcuts*/
/*	if (e.IsRepeat())*/
/*		return false;*/
/**/
/*	bool control = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);*/
/*	bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);*/
/**/
/*	switch (e.GetKeyCode())*/
/*	{*/
/*		case Key::N:*/
/*		{*/
/*			if (control)*/
/*				//NewScene();*/
/**/
/*			break;*/
/*		}*/
/*		case Key::O:*/
/*		{*/
/*			if (control)*/
/*				//OpenProject();*/
/**/
/*			break;*/
/*		}*/
/*		case Key::S:*/
/*		{*/
/*			if (control)*/
/*			{*/
/*				/*if (shift)*/
/*					SaveSceneAs();*/
/*				else*/
/*					SaveScene();*/
/*			}*/
/**/
/*			break;*/
/*		}*/
/**/
/*		// Scene Commands*/
/*		case Key::D:*/
/*		{*/
/*			if (control)*/
/*				//OnDuplicateEntity();*/
/**/
/*			break;*/
/*		}*/
/**/
/*		// Gizmos*/
/*		case Key::Q:*/
/*		{*/
/*			if (!ImGuizmo::IsUsing())*/
/*				m_GizmoType = -1;*/
/*			break;*/
/*		}*/
/*		case Key::W:*/
/*		{*/
/*			if (!ImGuizmo::IsUsing())*/
/*				m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;*/
/*			break;*/
/*		}*/
/*		case Key::E:*/
/*		{*/
/*			if (!ImGuizmo::IsUsing())*/
/*				m_GizmoType = ImGuizmo::OPERATION::ROTATE;*/
/*			break;*/
/*		}*/
/*		case Key::R:*/
/*		{*/
/**/
/*			if (!ImGuizmo::IsUsing())*/
/*				m_GizmoType = ImGuizmo::OPERATION::SCALE;*/
/**/
/*			break;*/
/*		}*/
/*		case Key::Delete:*/
/*		{*/
/*			if (Engine::GetInstance().GetImGuiLayer()->GetActiveWidgetID() == 0)*/
/*			{*/
/*				Entity selectedEntity = m_SelectionContext;*/
/*				if (selectedEntity)*/
/*				{*/
/*					m_SelectionContext = {};*/
/*					m_ActiveScene->DestroyEntity(selectedEntity);*/
/*				}*/
/*			}*/
/*			break;*/
/*		}*/
/*	}*/
/**/
/*	return false;*/
/*}*/
/**/
/*bool Editor::OnMouseButtonPressed(MouseButtonPressedEvent& e)*/
/*{*/
/*	if (e.GetMouseButton() == Mouse::ButtonLeft)*/
/*	{*/
/*		if (m_ViewportHovered && !ImGuizmo::IsOver() && !Input::IsKeyPressed(Key::LeftAlt))*/
/*			m_SelectionContext = m_HoveredEntity;*/
/*	}*/
/*	return false;*/
/*}*/
/**/
/*void Editor::ReloadProject()*/
/*{*/
/**/
/*}*/
/**/
/*void Editor::SaveProject()*/
/*{*/
/**/
/*}*/
/**/
/*void Editor::ViewportPanel()*/
/*{*/
/*	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });*/
/*	ImGui::Begin("Viewport", nullptr, NULL);*/
/*	auto viewportMinRegion = ImGui::GetWindowContentRegionMin();*/
/*	auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();*/
/*	auto viewportOffset = ImGui::GetWindowPos();*/
/*	m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };*/
/*	m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };*/
/**/
/*	m_ViewportFocused = ImGui::IsWindowFocused();*/
/*	m_ViewportHovered = ImGui::IsWindowHovered();*/
/**/
/*	Engine::GetInstance().GetImGuiLayer()->BlockEvents(!m_ViewportHovered);*/
/*	ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();*/
/*	m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };*/
/**/
/*	uint64_t textureID = m_Framebuffer->GetColorAttachmentRendererID();*/
/*	ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });*/
/**/
/*	if (ImGui::BeginDragDropTarget())*/
/*	{*/
/*		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))*/
/*		{*/
/*			const wchar_t* path = (const wchar_t*)payload->Data;*/
/*			//OpenScene(path);*/
/*		}*/
/*		ImGui::EndDragDropTarget();*/
/*	}*/
/**/
/*	Entity selectedEntity = m_SelectionContext;*/
/*	if (selectedEntity && m_GizmoType != -1)*/
/*	{*/
/*		ImGuizmo::SetOrthographic(false);*/
/*		ImGuizmo::SetDrawlist();*/
/**/
/*		ImGuizmo::SetRect(m_ViewportBounds[0].x, m_ViewportBounds[0].y, m_ViewportBounds[1].x - m_ViewportBounds[0].x, m_ViewportBounds[1].y - m_ViewportBounds[0].y);*/
/**/
/*		// Editor camera*/
/*		const glm::mat4& cameraProjection = m_EditorCamera.GetProjection();*/
/*		glm::mat4 cameraView = m_EditorCamera.GetViewMatrix();*/
/**/
/*		// Entity transform*/
/*		auto& tc = selectedEntity.GetComponent<TransformComponent>();*/
/*		glm::mat4 transform = tc.GetTransform();*/
/**/
/*		// Snapping*/
/*		bool snap = Input::IsKeyPressed(Key::LeftControl);*/
/*		float snapValue = 0.5f; // Snap to 0.5m for translation/scale*/
/*		// Snap to 45 degrees for rotation*/
/*		if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)*/
/*			snapValue = 45.0f;*/
/**/
/*		float snapValues[3] = { snapValue, snapValue, snapValue };*/
/**/
/*		ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),*/
/*			(ImGuizmo::OPERATION)m_GizmoType, ImGuizmo::LOCAL, glm::value_ptr(transform),*/
/*			nullptr, snap ? snapValues : nullptr);*/
/**/
/*		if (ImGuizmo::IsUsing())*/
/*		{*/
/*			glm::vec3 position, rotation, scale;*/
/*			Utils::DecomposeTransform(transform, position, rotation, scale);*/
/**/
/*			glm::vec3 deltaRotation = rotation - tc.Rotation;*/
/*			tc.Position = position;*/
/*			tc.Rotation += deltaRotation;*/
/*			tc.Scale = scale;*/
/*		}*/
/*	}*/
/**/
/*	ImGui::End();*/
/*	ImGui::PopStyleVar();*/
/*}*/
/**/
/*void Editor::SceneHierarchyPanel()*/
/*{*/
/*	ImGui::Begin("Scene Hierarchy", nullptr, ImGuiWindowFlags_NoCollapse);*/
/*	CenteredText("Scene Hierarchy");*/
/**/
/*	if (m_ActiveScene)*/
/*	{*/
/*		m_ActiveScene->m_Registry.each([&](auto entityID)*/
/*			{*/
/*				Entity entity{ entityID, m_ActiveScene.get() };*/
/*				DrawEntityNode(entity);*/
/*			});*/
/**/
/*		// Clear selection if left-click on blank space*/
/*		if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())*/
/*			m_SelectionContext = {};*/
/**/
/*		// Right-click on blank space only*/
/*		if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))*/
/*		{*/
/*			ImGui::OpenPopup("BlankSpaceContextMenu");*/
/*		}*/
/**/
/*		if (ImGui::BeginPopup("BlankSpaceContextMenu"))*/
/*		{*/
/*			if (ImGui::MenuItem("Create Empty Entity"))*/
/*				m_ActiveScene->CreateEntity("Empty Entity");*/
/**/
/*			ImGui::EndPopup();*/
/*		}*/
/*	}*/
/**/
/*	ImGui::End();*/
/*}*/
/**/
/*void Editor::ComponentsPanel()*/
/*{*/
/*	ImGui::Begin("Components", nullptr, ImGuiWindowFlags_NoCollapse);*/
/*	CenteredText("Components");*/
/*	if (m_SelectionContext)*/
/*	{*/
/*		DrawComponents(m_SelectionContext);*/
/*	}*/
/**/
/*	ImGui::End();*/
/*}*/
/**/
/*void Editor::DrawEntityNode(Entity entity)*/
/*{*/
/*	auto& tag = entity.GetComponent<TagComponent>().Tag;*/
/**/
/*	ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;*/
/*	flags |= ImGuiTreeNodeFlags_SpanAvailWidth;*/
/*	bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.c_str());*/
/*	if (ImGui::IsItemClicked())*/
/*	{*/
/*		m_SelectionContext = entity;*/
/*	}*/
/**/
/*	bool entityDeleted = false;*/
/*	if (ImGui::BeginPopupContextItem())*/
/*	{*/
/*		if (ImGui::MenuItem("Delete Entity"))*/
/*			entityDeleted = true;*/
/**/
/*		ImGui::EndPopup();*/
/*	}*/
/**/
/*	if (opened)*/
/*	{*/
/*		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;*/
/*		bool opened = ImGui::TreeNodeEx((void*)9817239, flags, tag.c_str());*/
/*		if (opened)*/
/*			ImGui::TreePop();*/
/*		ImGui::TreePop();*/
/*	}*/
/**/
/*	if (entityDeleted)*/
/*	{*/
/*		m_ActiveScene->DestroyEntity(entity);*/
/*		if (m_SelectionContext == entity)*/
/*			m_SelectionContext = {};*/
/*	}*/
/*}*/
/**/
/*static void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)*/
/*{*/
/*	ImGuiIO& io = ImGui::GetIO();*/
/*	auto boldFont = io.Fonts->Fonts[0];*/
/**/
/*	ImGui::PushID(label.c_str());*/
/**/
/*	ImGui::Columns(2);*/
/*	ImGui::SetColumnWidth(0, columnWidth);*/
/*	ImGui::Text(label.c_str());*/
/*	ImGui::NextColumn();*/
/**/
/*	ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());*/
/*	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });*/
/**/
/*	float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;*/
/*	ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };*/
/**/
/*	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });*/
/*	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });*/
/*	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });*/
/*	ImGui::PushFont(boldFont);*/
/*	if (ImGui::Button("X", buttonSize))*/
/*		values.x = resetValue;*/
/*	ImGui::PopFont();*/
/*	ImGui::PopStyleColor(3);*/
/**/
/*	ImGui::SameLine();*/
/*	ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");*/
/*	ImGui::PopItemWidth();*/
/*	ImGui::SameLine();*/
/**/
/*	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });*/
/*	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });*/
/*	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });*/
/*	ImGui::PushFont(boldFont);*/
/*	if (ImGui::Button("Y", buttonSize))*/
/*		values.y = resetValue;*/
/*	ImGui::PopFont();*/
/*	ImGui::PopStyleColor(3);*/
/**/
/*	ImGui::SameLine();*/
/*	ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");*/
/*	ImGui::PopItemWidth();*/
/*	ImGui::SameLine();*/
/**/
/*	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });*/
/*	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });*/
/*	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });*/
/*	ImGui::PushFont(boldFont);*/
/*	if (ImGui::Button("Z", buttonSize))*/
/*		values.z = resetValue;*/
/*	ImGui::PopFont();*/
/*	ImGui::PopStyleColor(3);*/
/**/
/*	ImGui::SameLine();*/
/*	ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");*/
/*	ImGui::PopItemWidth();*/
/**/
/*	ImGui::PopStyleVar();*/
/**/
/*	ImGui::Columns(1);*/
/**/
/*	ImGui::PopID();*/
/*}*/
/**/
/*template<typename T, typename UIFunction>*/
/*static void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction)*/
/*{*/
/*	const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;*/
/*	if (entity.HasComponent<T>())*/
/*	{*/
/*		auto& component = entity.GetComponent<T>();*/
/*		ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();*/
/**/
/*		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });*/
/*		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;*/
/*		ImGui::Separator();*/
/*		bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());*/
/*		ImGui::PopStyleVar(*/
/*		);*/
/*		ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);*/
/*		if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))*/
/*		{*/
/*			ImGui::OpenPopup("ComponentSettings");*/
/*		}*/
/**/
/*		bool removeComponent = false;*/
/*		if (ImGui::BeginPopup("ComponentSettings"))*/
/*		{*/
/*			if (ImGui::MenuItem("Remove component"))*/
/*				removeComponent = true;*/
/**/
/*			ImGui::EndPopup();*/
/*		}*/
/**/
/*		if (open)*/
/*		{*/
/*			uiFunction(component);*/
/*			ImGui::TreePop();*/
/*		}*/
/**/
/*		if (removeComponent)*/
/*			entity.RemoveComponent<T>();*/
/*	}*/
/*}*/
/**/
/*void Editor::DrawComponents(Entity entity)*/
/*{*/
/*	if (entity.HasComponent<TagComponent>())*/
/*	{*/
/*		auto& tag = entity.GetComponent<TagComponent>().Tag;*/
/**/
/*		char buffer[256];*/
/*		memset(buffer, 0, sizeof(buffer));*/
/*		strncpy_s(buffer, sizeof(buffer), tag.c_str(), sizeof(buffer));*/
/*		if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))*/
/*		{*/
/*			tag = std::string(buffer);*/
/*		}*/
/*	}*/
/**/
/*	ImGui::SameLine();*/
/*	ImGui::PushItemWidth(-1);*/
/**/
/*	if (ImGui::Button("Add Component")) ImGui::OpenPopup("AddComponent");*/
/**/
/*	if (ImGui::BeginPopup("AddComponent"))*/
/*	{*/
/*		DisplayAddComponentEntry<TransformComponent>("Transform");*/
/*		DisplayAddComponentEntry<CameraComponent>("Camera");*/
/*		DisplayAddComponentEntry<TextureComponent>("Texture");*/
/*		DisplayAddComponentEntry<TextComponent>("Text");*/
/**/
/*		ImGui::EndPopup();*/
/*	}*/
/**/
/*	ImGui::PopItemWidth();*/
/**/
/*	DrawComponent<TransformComponent>("Transform", entity, [](auto& component)*/
/*		{*/
/*			DrawVec3Control("Translation", component.Position);*/
/*			glm::vec3 rotation = glm::degrees(component.Rotation);*/
/*			DrawVec3Control("Rotation", rotation);*/
/*			component.Rotation = glm::radians(rotation);*/
/*			DrawVec3Control("Scale", component.Scale, 1.0f);*/
/*		});*/
/**/
/*	DrawComponent<CameraComponent>("Camera", entity, [](auto& component)*/
/*		{*/
/*			auto& camera = component.Camera;*/
/**/
/*			ImGui::Checkbox("Primary", &component.Primary);*/
/**/
/*			const char* projectionTypeStrings[] = { "Perspective", "Orthographic" };*/
/*			const char* currentProjectionTypeString = projectionTypeStrings[(int)camera.GetProjectionType()];*/
/*			if (ImGui::BeginCombo("Projection", currentProjectionTypeString))*/
/*			{*/
/*				for (int i = 0; i < 2; i++)*/
/*				{*/
/*					bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];*/
/*					if (ImGui::Selectable(projectionTypeStrings[i], isSelected))*/
/*					{*/
/*						currentProjectionTypeString = projectionTypeStrings[i];*/
/*						camera.SetProjectionType((SceneCamera::ProjectionType)i);*/
/*					}*/
/**/
/*					if (isSelected)*/
/*						ImGui::SetItemDefaultFocus();*/
/*				}*/
/**/
/*				ImGui::EndCombo();*/
/*			}*/
/**/
/*			if (camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)*/
/*			{*/
/*				float perspectiveVerticalFov = glm::degrees(camera.GetPerspectiveVerticalFOV());*/
/*				if (ImGui::DragFloat("Vertical FOV", &perspectiveVerticalFov))*/
/*					camera.SetPerspectiveVerticalFOV(glm::radians(perspectiveVerticalFov));*/
/**/
/*				float perspectiveNear = camera.GetPerspectiveNearClip();*/
/*				if (ImGui::DragFloat("Near", &perspectiveNear))*/
/*					camera.SetPerspectiveNearClip(perspectiveNear);*/
/**/
/*				float perspectiveFar = camera.GetPerspectiveFarClip();*/
/*				if (ImGui::DragFloat("Far", &perspectiveFar))*/
/*					camera.SetPerspectiveFarClip(perspectiveFar);*/
/*			}*/
/**/
/*			if (camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)*/
/*			{*/
/*				float orthoSize = camera.GetOrthographicSize();*/
/*				if (ImGui::DragFloat("Size", &orthoSize))*/
/*					camera.SetOrthographicSize(orthoSize);*/
/**/
/*				float orthoNear = camera.GetOrthographicNearClip();*/
/*				if (ImGui::DragFloat("Near", &orthoNear))*/
/*					camera.SetOrthographicNearClip(orthoNear);*/
/**/
/*				float orthoFar = camera.GetOrthographicFarClip();*/
/*				if (ImGui::DragFloat("Far", &orthoFar))*/
/*					camera.SetOrthographicFarClip(orthoFar);*/
/**/
/*				ImGui::Checkbox("Fixed Aspect Ratio", &component.FixedAspectRatio);*/
/*			}*/
/*		});*/
/**/
/*	DrawComponent<TextureComponent>("Texture", entity, [](auto& component)*/
/*		{*/
/*			ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));*/
/**/
/*			if (component.Texture)*/
/*			{*/
/*				// Flip the Y axis by changing UV coordinates*/
/*				ImVec2 uv0 = { 0.0f, 1.0f }; // Bottom-left*/
/*				ImVec2 uv1 = { 1.0f, 0.0f }; // Top-right*/
/*				ImGui::Image((ImTextureID)component.Texture->GetRendererID(), ImVec2(100.0f, 100.0f), uv0, uv1);*/
/*			}*/
/*			else*/
/*			{*/
/*				ImGui::Text("Drop Texture here");*/
/*			}*/
/**/
/*			// Handle drag and drop for the texture*/
/*			if (ImGui::BeginDragDropTarget())*/
/*			{*/
/*				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))*/
/*				{*/
/*					const wchar_t* path = (const wchar_t*)payload->Data;*/
/*					std::filesystem::path texturePath(path);*/
/*					Ref<Texture> texture = Texture::Create(texturePath.string());*/
/*					if (texture->IsLoaded())*/
/*						component.Texture = texture;*/
/*					else*/
/*						GABGL_WARN("Could not load texture {0}", texturePath.filename().string());*/
/*				}*/
/*				ImGui::EndDragDropTarget();*/
/*			}*/
/*		});*/
/**/
/*	DrawComponent<TextComponent>("Text Renderer", entity, [](auto& component)*/
/*		{*/
/*			//ImGui::InputTextMultiline("Text String", &component.TextString);*/
/*			ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));*/
/*			ImGui::DragFloat("Kerning", &component.Kerning, 0.025f);*/
/*			ImGui::DragFloat("Line Spacing", &component.LineSpacing, 0.025f);*/
/*		});*/
/**/
/*}*/
/**/
/*template<typename T>*/
/*void Editor::DisplayAddComponentEntry(const std::string& entryName) {*/
/*	if (!m_SelectionContext.HasComponent<T>())*/
/*	{*/
/*		if (ImGui::MenuItem(entryName.c_str()))*/
/*		{*/
/*			m_SelectionContext.AddComponent<T>();*/
/*			ImGui::CloseCurrentPopup();*/
/*		}*/
/*	}*/
/*}*/
/**/
/*void Editor::CenteredText(const char* text) {*/
/*	ImVec2 windowSize = ImGui::GetWindowSize();*/
/*	ImVec2 textSize = ImGui::CalcTextSize(text);*/
/**/
/*	float centeredX = (windowSize.x - textSize.x) / 2.0f;*/
/*	ImGui::SetCursorPosX(centeredX);*/
/*	ImGui::Text("%s", text);*/
/*}*/
/**/
