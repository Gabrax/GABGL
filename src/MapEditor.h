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
#include "Managers/LightManager.h"

struct MapEditor
{
  void Init()
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

  void editorlogic()
  {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImVec2 windowSize(100, 100);  
    ImVec2 windowPos(100, 100);   

    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

    ImGui::Begin("MapEditor");

    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);  
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
    ImGui::Text("Camera Properties");

    ImGui::Columns(2, nullptr, false); // 2 columns, no borders
    ImGui::SetColumnWidth(0, 50);    // Set the width of the first column

    // X Value
    ImGui::Text("X:");
    ImGui::NextColumn();
    ImGui::DragFloat("##X", &Window::_camera.Position.x, 0.1f, -FLT_MAX, FLT_MAX, "%.2f", ImGuiSliderFlags_None);

    ImGui::NextColumn(); 
    // Y Value
    ImGui::Text("Y:");
    ImGui::NextColumn(); 
    ImGui::DragFloat("##Y", &Window::_camera.Position.y, 0.1f, -FLT_MAX, FLT_MAX, "%.2f", ImGuiSliderFlags_None);

    ImGui::NextColumn();
    // Z Value
    ImGui::Text("Z:");
    ImGui::NextColumn();
    ImGui::DragFloat("##Z", &Window::_camera.Position.z, 0.1f, -FLT_MAX, FLT_MAX, "%.2f", ImGuiSliderFlags_None);

    ImGui::NextColumn(); 
    // Yaw Value
    ImGui::Text("Yaw:");
    ImGui::NextColumn();
    ImGui::DragFloat("##Yaw", &Window::_camera.Yaw, 0.5f, -180.0f, 180.0f, "%.2f", ImGuiSliderFlags_None);

    ImGui::NextColumn();
    // Pitch Value
    ImGui::Text("Pitch:");
    ImGui::NextColumn();
    ImGui::DragFloat("##Pitch", &Window::_camera.Pitch, 0.5f, -90.0f, 90.0f, "%.2f", ImGuiSliderFlags_None);

    ImGui::Columns(1); // Reset to single-column layout
    ImGui::Separator();

    ImGui::Text("Light Editor");

    static int selectedLightIndex = 0; // Selected light index
    static glm::vec3 lightPosition(0.0f, 15.0f, 0.0f);
    static glm::vec3 lightScale(1.0f, 1.0f, 1.0f);
    static float lightRotation = 0.0f;
    static glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f); // Default to WHITE

    // Dropdown to select the light index
    if (!lightManager.lights.empty()) {
        ImGui::Text("Select Light:");
        std::vector<std::string> lightNames;
        for (int i = 0; i < lightManager.lights.size(); ++i) {
            lightNames.push_back("Light " + std::to_string(i));
        }
        std::vector<const char*> lightNamesCStr;
        for (const auto& name : lightNames) {
            lightNamesCStr.push_back(name.c_str());
        }
        ImGui::Combo("##LightIndex", &selectedLightIndex, lightNamesCStr.data(), lightNamesCStr.size());
    } else {
        ImGui::Text("No lights to edit.");
    }

    ImGui::Text("Edit Properties");

    ImGui::Text("Color");
    ImGui::ColorEdit3("Color", &lightColor[0]); 

    ImGui::Text("Position");
    ImGui::DragFloat3("Position", &lightPosition[0]); 

    ImGui::Text("Scale");
    ImGui::DragFloat3("Scale", &lightScale[0]); 

    ImGui::Text("Rotation");
    ImGui::SliderFloat("Rotation", &lightRotation, 0.0f, 360.0f); 

    if (ImGui::Button("Add Light")) {
        glm::vec4 colorWithAlpha(lightColor, 1.0f); // Convert color to glm::vec4
        lightManager.AddLight(colorWithAlpha, lightPosition, lightScale, lightRotation);
    }

    if (ImGui::Button("Edit Light")) {
        if (selectedLightIndex >= 0 && selectedLightIndex < lightManager.lights.size()) {
            glm::vec4 colorWithAlpha(lightColor, 1.0f); // Convert color to glm::vec4
            lightManager.EditLight(selectedLightIndex, colorWithAlpha, lightPosition, lightScale, lightRotation);
        } else {
            ImGui::Text("Invalid light index selected!");
        }
    }

    if (ImGui::Button("Remove Light")) {
        if (selectedLightIndex >= 0 && selectedLightIndex < lightManager.lights.size()) {
            lightManager.RemoveLight(selectedLightIndex);
        } else {
            ImGui::Text("Invalid light index selected!");
        }
    }

    ImGui::Separator();

    if (ImGui::Button("Save Scene"))
    {
        nlohmann::json sceneData;

        // Save camera data
        sceneData["Camera"]["Position"] = {
            {"x", Window::_camera.Position.x},
            {"y", Window::_camera.Position.y},
            {"z", Window::_camera.Position.z}
        };
        sceneData["Camera"]["Rotation"] = {
            {"Yaw", Window::_camera.Yaw},
            {"Pitch", Window::_camera.Pitch}
        };

        // Save lights data
        sceneData["Lights"] = nlohmann::json::array();
        for (const auto& light : lightManager.lights) {
            const auto& lightData = light.second; // Assuming lights are stored as a vector of pairs
            nlohmann::json lightJson;
            lightJson["Color"] = {
                {"r", lightData.color.r},
                {"g", lightData.color.g},
                {"b", lightData.color.b},
                {"a", lightData.color.a}
            };
            lightJson["Position"] = {
                {"x", lightData.position.x},
                {"y", lightData.position.y},
                {"z", lightData.position.z}
            };
            lightJson["Scale"] = {
                {"x", lightData.scale.x},
                {"y", lightData.scale.y},
                {"z", lightData.scale.z}
            };
            lightJson["Rotation"] = lightData.rotation;
            sceneData["Lights"].push_back(lightJson);
        }

        // Write the scene data to a JSON file
        std::ofstream outFile("scene.json");
        if (outFile.is_open()) {
            outFile << sceneData.dump(4); // Pretty print JSON
            outFile.close();
            puts("Scene data saved");
        } else {
            puts("Error: Could not create file scene.json");
        }
    }

    if (ImGui::Button("Load Scene"))
    {
       LoadScene(); 
    }

    ImGui::End();
    

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

  void Render()
  {
    if(isRendered){
      editorlogic();
      Window::ShowCursor();
    } else Window::DisableCursor(); 

    if(Input::KeyPressed(KEY_GRAVE_ACCENT)){
      isRendered = !isRendered;
      Window::DisableMovement();
    }
  }

  void RenderScene()
  {
    lightManager.RenderLights();
  }

  void LoadScene()
  {
    std::ifstream inFile("scene.json");
    if (inFile.is_open()) {
        try {
            nlohmann::json sceneData;
            inFile >> sceneData;

            // Load camera data
            if (sceneData.contains("Camera")) {
                const auto& cameraData = sceneData["Camera"];
                if (cameraData.contains("Position") && cameraData.contains("Rotation")) {
                    Window::_camera.Position.x = cameraData["Position"]["x"];
                    Window::_camera.Position.y = cameraData["Position"]["y"];
                    Window::_camera.Position.z = cameraData["Position"]["z"];
                    Window::_camera.Yaw = cameraData["Rotation"]["Yaw"];
                    Window::_camera.Pitch = cameraData["Rotation"]["Pitch"];
                    puts("Camera data loaded successfully");
                }
            }

            // Load lights data
            if (sceneData.contains("Lights")) {
                lightManager.lights.clear(); // Clear existing lights
                for (const auto& lightJson : sceneData["Lights"]) {
                    glm::vec4 color(
                        lightJson["Color"]["r"],
                        lightJson["Color"]["g"],
                        lightJson["Color"]["b"],
                        lightJson["Color"]["a"]
                    );
                    glm::vec3 position(
                        lightJson["Position"]["x"],
                        lightJson["Position"]["y"],
                        lightJson["Position"]["z"]
                    );
                    glm::vec3 scale(
                        lightJson["Scale"]["x"],
                        lightJson["Scale"]["y"],
                        lightJson["Scale"]["z"]
                    );
                    float rotation = lightJson["Rotation"];
                    lightManager.AddLight(color, position, scale, rotation); // Re-add lights
                }
                puts("Lights data loaded successfully");
            }
        } catch (const std::exception& e) {
            puts("Error: Failed to parse scene.json");
            printf("Exception: %s\n", e.what());
        }
    } else {
        puts("Error: Could not open file scene.json");
    }
  }

private:

  enum class EditorPage
  {
    Main,
    LightEditor
  };

  LightManager lightManager;
  bool isRendered = false;
};
