#pragma once
#include "Input/Input.h"
#include "Input/Key_Values.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imguiThemes.h"
#include "Window.h"

namespace MapEditor {

  inline bool isRendered = false;
  
  enum class EditorPage
  {
    Main,
    LightEditor
  };

  static EditorPage currentPage = EditorPage::Main; // Track current page

  inline void Init()
  {

    ImGui::CreateContext();

    imguiThemes::embraceTheDarkness();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

    io.FontGlobalScale = 1.0f; 

    ImGuiStyle &style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
      style.Colors[ImGuiCol_WindowBg].w = 0.f;  // Set window background to be transparent
      style.Colors[ImGuiCol_DockingEmptyBg].w = 0.f; // Transparent docking area
    }

    ImGui_ImplGlfw_InitForOpenGL(Window::GetWindowPtr(), true);
    ImGui_ImplOpenGL3_Init("#version 330");
  }

  inline void editorlogic()
  {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImVec2 windowSize(100, 100);  
    ImVec2 windowPos(100, 100);   

    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());


    if (currentPage == EditorPage::Main) {
        
        ImGui::Begin("MapEditor");

        ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);  
        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);    
        ImGui::Text("Camera Position");
        ImGui::Text("X: %.2f, Y: %.2f, Z: %.2f", 
            Window::_camera.Position.x, 
            Window::_camera.Position.y, 
            Window::_camera.Position.z);
        ImGui::Text("Camera Rotation");
        ImGui::Text("X: %.2f, Y: %.2f", 
            Window::_camera.Yaw, 
            Window::_camera.Pitch);

        if (ImGui::Button("Light Editor")) {
            currentPage = EditorPage::LightEditor; 
        }

        ImGui::End();
    }
    else if (currentPage == EditorPage::LightEditor) {
        
        ImGui::Begin("Light Editor");

        ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);  
        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);    
        ImGui::Text("Light settings go here...");
        

        if (ImGui::Button("Return to Main")) {
            currentPage = EditorPage::Main; 
        }

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
  }

  inline void Render()
  {
    if(isRendered){
      editorlogic();
      Window::ShowCursor();
    } else Window::DisableCursor(); 

    if(Input::KeyPressed(KEY_GRAVE_ACCENT)){
      isRendered = !isRendered;
    }
  }

}
