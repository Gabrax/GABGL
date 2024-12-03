#pragma once
#include "Input/Input.h"
#include "Input/Key_Values.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"
#include "imguiThemes.h"
#include "Window.h"
#include <json.hpp>
#include <fstream>

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
        ImGui::Text("Camera Properties");
        ImGui::Columns(2, nullptr, false); // 2 columns, no borders
        ImGui::SetColumnWidth(0, 50);    // Set the width of the first column

        
        ImGui::Text("X:");
        ImGui::NextColumn(); 
        ImGui::InputFloat("##X", &Window::_camera.Position.x, 0.1f, 1.0f, "%.2f"); // Hide label in the field

        ImGui::NextColumn(); 
        ImGui::Text("Y:");
        ImGui::NextColumn(); 
        ImGui::InputFloat("##Y", &Window::_camera.Position.y, 0.1f, 1.0f, "%.2f");

        ImGui::NextColumn();
        ImGui::Text("Z:");
        ImGui::NextColumn();
        ImGui::InputFloat("##Z", &Window::_camera.Position.z, 0.1f, 1.0f, "%.2f");

        ImGui::NextColumn(); 
        ImGui::Text("Yaw:");
        ImGui::NextColumn();
        ImGui::SliderFloat("##Yaw", &Window::_camera.Yaw, -180.0f, 180.0f, "%.2f");

        ImGui::NextColumn();
        ImGui::Text("Pitch:");
        ImGui::NextColumn();
        ImGui::SliderFloat("##Pitch", &Window::_camera.Pitch, -90.0f, 90.0f, "%.2f");

        ImGui::Columns(1); // Reset to single-column layout
        ImGui::Separator();

        if (ImGui::Button("Light Editor"))
        {
            currentPage = EditorPage::LightEditor; 
        }

        if (ImGui::Button("Save Scene"))
        {
          nlohmann::json cameraData;

          cameraData["Position"] = {
              {"x", Window::_camera.Position.x},
              {"y", Window::_camera.Position.y},
              {"z", Window::_camera.Position.z}
          };
          cameraData["Rotation"] = {
              {"Yaw", Window::_camera.Yaw},
              {"Pitch", Window::_camera.Pitch}
          };

          // Attempt to create and write to the file
          std::ofstream outFile("scene.json");
          if (outFile.is_open()) {
              outFile << cameraData.dump(4); // Pretty print JSON
              outFile.close();
              puts("Camera data saved");
          } else {
              puts("Error: Could not create file ");
          }
        }

        if (ImGui::Button("Load Scene"))
        {
            std::ifstream inFile("scene.json");
            if (inFile.is_open()) {
                try {
                    nlohmann::json cameraData;
                    inFile >> cameraData;

                    if (cameraData.contains("Position") && cameraData.contains("Rotation")) {
                        Window::_camera.Position.x = cameraData["Position"]["x"];
                        Window::_camera.Position.y = cameraData["Position"]["y"];
                        Window::_camera.Position.z = cameraData["Position"]["z"];
                        Window::_camera.Yaw = cameraData["Rotation"]["Yaw"];
                        Window::_camera.Pitch = cameraData["Rotation"]["Pitch"];

                        puts("Camera data loaded successfully");
                    } else {
                        puts("Error: Invalid JSON structure in scene.json");
                    }
                } catch (const std::exception& e) {
                    puts("Error: Failed to parse scene.json");
                    printf("Exception: %s\n", e.what());
                }
            } else {
                puts("Error: Could not open file scene.json");
            }
        }

        ImGui::End();
    }
    else if (currentPage == EditorPage::LightEditor) {
        
        ImGui::Begin("Light Editor");

        ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);  
        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);    
        ImGui::Text("skibidi...");
        

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
