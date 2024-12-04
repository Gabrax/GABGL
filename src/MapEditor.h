#pragma once
#include "Input/Input.h"
#include "Input/Key_Values.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "glm/fwd.hpp"
#include "imgui.h"
#include "imguiThemes.h"
#include "Window.h"
#include <json.hpp>
#include <fstream>
#include "Managers/LightManager.h"
#include "Managers/ModelManager.h"

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

    ImGui::Columns(1); 
    ImGui::Separator();

    
    ImGui::Text("Light Editor");

    static int selectedLightIndex = 0; 
    static glm::vec3 lightPosition;
    static glm::vec3 lightScale;
    static float lightRotation;
    static glm::vec3 lightColor;

    // Ensure selectedLightIndex is valid
    if (!lightManager.lights.empty()) {
        // Set default selected light at the start
        if (selectedLightIndex == 0 && lightColor == glm::vec3(0.0f)) {
            const auto& selectedLight = lightManager.lights[selectedLightIndex];
            lightColor = glm::vec3(selectedLight.second.color);
            lightPosition = selectedLight.second.position;
            lightScale = selectedLight.second.scale;
            lightRotation = selectedLight.second.rotation;
        }

        // Ensure selectedLightIndex is within range
        selectedLightIndex = std::clamp(selectedLightIndex, 0, static_cast<int>(lightManager.lights.size()) - 1);
        auto& selectedLight = lightManager.lights[selectedLightIndex];

        ImGui::Text("Select Light:");
        std::vector<std::string> lightNames;
        for (int i = 0; i < lightManager.lights.size(); ++i) {
            lightNames.push_back("Light " + std::to_string(i));
        }
        std::vector<const char*> lightNamesCStr;
        for (const auto& name : lightNames) {
            lightNamesCStr.push_back(name.c_str());
        }

        // Light selection
        if (ImGui::Combo("##LightIndex", &selectedLightIndex, lightNamesCStr.data(), lightNamesCStr.size())) {
            // Update local variables when a new light is selected
            const auto& newSelectedLight = lightManager.lights[selectedLightIndex];
            lightColor = glm::vec3(newSelectedLight.second.color);
            lightPosition = newSelectedLight.second.position;
            lightScale = newSelectedLight.second.scale;
            lightRotation = newSelectedLight.second.rotation;
        }

        ImGui::Text("Edit Properties");

        // Update light properties interactively
        if (ImGui::ColorEdit3("lightColor", &lightColor[0])) {
            selectedLight.second.color = glm::vec4(lightColor, 1.0f); 
        }

        if (ImGui::DragFloat3("lightPosition", &lightPosition[0])) {
            selectedLight.second.position = glm::vec4(lightPosition, selectedLight.second.position.w);
        }

        if (ImGui::DragFloat3("lightScale", &lightScale[0])) {
            selectedLight.second.scale = lightScale; 
        }

        if (ImGui::SliderFloat("lightRotation", &lightRotation, 0.0f, 360.0f)) {
            selectedLight.second.rotation = lightRotation; 
        }

        
        if (ImGui::Button("Add Light")) {
            glm::vec4 colorWithAlpha(lightColor, 1.0f);
            lightManager.AddLight(colorWithAlpha, lightPosition, lightScale, lightRotation);

            // Update selected index to the newly added light
            selectedLightIndex = static_cast<int>(lightManager.lights.size()) - 1;
            const auto& newSelectedLight = lightManager.lights[selectedLightIndex];
            lightColor = glm::vec3(newSelectedLight.second.color);
            lightPosition = newSelectedLight.second.position;
            lightScale = newSelectedLight.second.scale;
            lightRotation = newSelectedLight.second.rotation;
        }

        if (ImGui::Button("Edit Light")) {
            if (selectedLightIndex >= 0 && selectedLightIndex < lightManager.lights.size()) {
                glm::vec4 colorWithAlpha(lightColor, 1.0f);
                lightManager.EditLight(selectedLightIndex, colorWithAlpha, lightPosition, lightScale, lightRotation);
            } else {
                ImGui::Text("Invalid light index selected!");
            }
        }

        if (ImGui::Button("Remove Light")) {
            if (selectedLightIndex >= 0 && selectedLightIndex < lightManager.lights.size()) {
                lightManager.RemoveLight(selectedLightIndex);

                // Adjust selected index after removal
                selectedLightIndex = std::max(0, selectedLightIndex - 1);

                // Update local variables if there are remaining lights
                if (!lightManager.lights.empty()) {
                    const auto& remainingLight = lightManager.lights[selectedLightIndex];
                    lightColor = glm::vec3(remainingLight.second.color);
                    lightPosition = remainingLight.second.position;
                    lightScale = remainingLight.second.scale;
                    lightRotation = remainingLight.second.rotation;
                }
            } else {
                ImGui::Text("Invalid light index selected!");
            }
        }

    } else {
        ImGui::Text("No lights to edit.");
    }

    ImGui::Separator();


    ImGui::Text("Model Editor");

    static int selectedModelIndex = 0;
    static glm::vec3 modelPosition;
    static glm::vec3 modelScale;
    static float modelRotation;

    static char modelPathBuffer[256] = "";  // Input buffer for model path
    static int selectedModelTypeIndex = 0;  // 0 for static, 1 for animated

    std::vector<std::string> modelNames;
    int totalModels = 0;

    if (!modelManager.vec_staticModels.empty()) {
        for (const auto& model : modelManager.vec_staticModels) {
            modelNames.push_back(model.second.modelpath);
        }
        totalModels += modelManager.vec_staticModels.size();
    }

    if (!modelManager.vec_animatedModels.empty()) {
        for (const auto& model : modelManager.vec_animatedModels) {
            modelNames.push_back(model.second.modelpath);
        }
        totalModels += modelManager.vec_animatedModels.size();
    }

    if (totalModels > 0) {
        // Set default selected model at the start
        if (selectedModelIndex == 0 && modelPosition == glm::vec3(0.0f)) {
            if (selectedModelIndex < modelManager.vec_staticModels.size()) {
                const auto& selectedModel = modelManager.vec_staticModels[selectedModelIndex];
                modelPosition = selectedModel.second.position;
                modelScale = selectedModel.second.scale;
                modelRotation = selectedModel.second.rotation;
            } else {
                int animatedModelIndex = selectedModelIndex - modelManager.vec_staticModels.size();
                const auto& selectedModel = modelManager.vec_animatedModels[animatedModelIndex];
                modelPosition = selectedModel.second.position;
                modelScale = selectedModel.second.scale;
                modelRotation = selectedModel.second.rotation;
            }
        }

        ImGui::Text("Select Model:");

        std::vector<const char*> modelNamesCStr;
        for (const auto& name : modelNames) {
            const char* filename = strrchr(name.c_str(), '/');  
            filename = (filename == nullptr) ? name.c_str() : filename + 1;  
            modelNamesCStr.push_back(filename);
        }

        // Ensure selectedModelIndex stays valid
        selectedModelIndex = std::clamp(selectedModelIndex, 0, totalModels - 1);

        if (ImGui::Combo("##ModelIndex", &selectedModelIndex, modelNamesCStr.data(), modelNamesCStr.size())) {
            int modelIndex = selectedModelIndex;
            if (modelIndex < modelManager.vec_staticModels.size()) {
                const auto& selectedModel = modelManager.vec_staticModels[modelIndex];
                modelPosition = selectedModel.second.position;
                modelScale = selectedModel.second.scale;
                modelRotation = selectedModel.second.rotation;
            } else {
                modelIndex -= modelManager.vec_staticModels.size();
                const auto& selectedModel = modelManager.vec_animatedModels[modelIndex];
                modelPosition = selectedModel.second.position;
                modelScale = selectedModel.second.scale;
                modelRotation = selectedModel.second.rotation;
            }
        }

        ImGui::Text("Edit Properties");

        // Position
        if (ImGui::DragFloat3("modelPosition", &modelPosition[0])) {
            if (selectedModelIndex < modelManager.vec_staticModels.size()) {
                auto& selectedModel = modelManager.vec_staticModels[selectedModelIndex];
                selectedModel.second.position = modelPosition;  
            } else {
                int animatedModelIndex = selectedModelIndex - modelManager.vec_staticModels.size();
                auto& selectedModel = modelManager.vec_animatedModels[animatedModelIndex];
                selectedModel.second.position = modelPosition;  
            }
        }

        // Scale
        if (ImGui::DragFloat3("modelScale", &modelScale[0])) {
            if (selectedModelIndex < modelManager.vec_staticModels.size()) {
                auto& selectedModel = modelManager.vec_staticModels[selectedModelIndex];
                selectedModel.second.scale = modelScale;  
            } else {
                int animatedModelIndex = selectedModelIndex - modelManager.vec_staticModels.size();
                auto& selectedModel = modelManager.vec_animatedModels[animatedModelIndex];
                selectedModel.second.scale = modelScale;  
            }
        }

        // Rotation
        if (ImGui::SliderFloat("modelRotation", &modelRotation, 0.0f, 360.0f)) {
            if (selectedModelIndex < modelManager.vec_staticModels.size()) {
                auto& selectedModel = modelManager.vec_staticModels[selectedModelIndex];
                selectedModel.second.rotation = modelRotation;  
            } else {
                int animatedModelIndex = selectedModelIndex - modelManager.vec_staticModels.size();
                auto& selectedModel = modelManager.vec_animatedModels[animatedModelIndex];
                selectedModel.second.rotation = modelRotation;  
            }
        }
    } else {
        ImGui::Text("No models to edit.");
    }

    ImGui::Text("Model Path:");
    ImGui::SameLine();
    ImGui::InputText("##ModelPath", modelPathBuffer, sizeof(modelPathBuffer));

    ImGui::Text("Model Type:");
    ImGui::SameLine();
    const char* modelTypeOptions[] = { "STATIC", "ANIMATED" };
    ImGui::Combo("##ModelType", &selectedModelTypeIndex, modelTypeOptions, IM_ARRAYSIZE(modelTypeOptions));

    if (ImGui::Button("Add Model")) {
      if (strlen(modelPathBuffer) > 0) {
          std::string modelPathString = modelPathBuffer;
          const char* modelType = modelTypeOptions[selectedModelTypeIndex];

          if (std::strcmp(modelType, "STATIC") == 0) {
              modelManager.AddModelStatic(modelPathString, modelPosition, modelScale, modelRotation);
              selectedModelIndex = static_cast<int>(modelManager.vec_staticModels.size()) - 1;
              const auto& newSelectedModel = modelManager.vec_staticModels[selectedModelIndex];
              modelPosition = newSelectedModel.second.position;
              modelScale = newSelectedModel.second.scale;
              modelRotation = newSelectedModel.second.rotation;
          }

          if (std::strcmp(modelType, "ANIMATED") == 0) {
              modelManager.AddModelAnimated(modelPathString, modelPosition, modelScale, modelRotation);
              selectedModelIndex = static_cast<int>(modelManager.vec_animatedModels.size()) - 1;
              const auto& newSelectedModel = modelManager.vec_animatedModels[selectedModelIndex];
              modelPosition = newSelectedModel.second.position;
              modelScale = newSelectedModel.second.scale;
              modelRotation = newSelectedModel.second.rotation;
          }

          modelPathBuffer[0] = '\0';  
      }
  }

    if (ImGui::Button("Edit Model")) {
        if (selectedModelIndex >= 0) {
            if (selectedModelIndex < modelManager.vec_staticModels.size()) {
                modelManager.EditStaticModel(selectedModelIndex, modelPosition, modelScale, modelRotation);
            } else {
                int animatedModelIndex = selectedModelIndex - modelManager.vec_staticModels.size();
                modelManager.EditAnimatedModel(animatedModelIndex, modelPosition, modelScale, modelRotation);
            }
        } else {
            ImGui::Text("Invalid model index selected!");
        }
    }

    if (ImGui::Button("Remove Model")) {
        if (selectedModelIndex >= 0) {
            if (selectedModelIndex < modelManager.vec_staticModels.size()) {
                modelManager.RemoveStaticModel(selectedModelIndex);
                selectedModelIndex = std::max(0, selectedModelIndex - 1);  // Adjust if necessary
            } else {
                int animatedModelIndex = selectedModelIndex - modelManager.vec_staticModels.size();
                modelManager.RemoveAnimatedModel(animatedModelIndex);
                selectedModelIndex = std::max(0, selectedModelIndex - 1);  // Adjust if necessary
            }
        } else {
            ImGui::Text("Invalid model index selected!");
        }
    }

    ImGui::Separator();

    if (ImGui::Button("Save Scene"))
    {
        nlohmann::json sceneData;

        sceneData["Camera"]["Position"] = {
            {"x", Window::_camera.Position.x},
            {"y", Window::_camera.Position.y},
            {"z", Window::_camera.Position.z}
        };
        sceneData["Camera"]["Rotation"] = {
            {"Yaw", Window::_camera.Yaw},
            {"Pitch", Window::_camera.Pitch}
        };

        sceneData["Lights"] = nlohmann::json::array();
        for (const auto& light : lightManager.lights) {
            const auto& lightData = light.second; 
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
        // Save Models Data
        sceneData["Models"] = nlohmann::json::array();
        for (const auto& model : modelManager.vec_staticModels) {
            nlohmann::json modelJson;
            modelJson["Type"] = "STATIC";
            modelJson["Path"] = model.second.modelpath;
            modelJson["Position"] = {
                {"x", model.second.position.x},
                {"y", model.second.position.y},
                {"z", model.second.position.z}
            };
            modelJson["Scale"] = {
                {"x", model.second.scale.x},
                {"y", model.second.scale.y},
                {"z", model.second.scale.z}
            };
            modelJson["Rotation"] = model.second.rotation;
            sceneData["Models"].push_back(modelJson);
        }
        for (const auto& model : modelManager.vec_animatedModels) {
            nlohmann::json modelJson;
            modelJson["Type"] = "ANIMATED";
            modelJson["Path"] = model.second.modelpath;
            modelJson["Position"] = {
                {"x", model.second.position.x},
                {"y", model.second.position.y},
                {"z", model.second.position.z}
            };
            modelJson["Scale"] = {
                {"x", model.second.scale.x},
                {"y", model.second.scale.y},
                {"z", model.second.scale.z}
            };
            modelJson["Rotation"] = model.second.rotation;
            sceneData["Models"].push_back(modelJson);
        }

        std::ofstream outFile("scene.json");
        if (outFile.is_open()) {
            outFile << sceneData.dump(4); 
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
    modelManager.RenderModels();
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

            // Load Models Data
            if (sceneData.contains("Models")) {
                for (const auto& modelJson : sceneData["Models"]) {
                    std::string modelType = modelJson["Type"];
                    std::string modelPath = modelJson["Path"].get<std::string>();
                    glm::vec3 position(
                        modelJson["Position"]["x"],
                        modelJson["Position"]["y"],
                        modelJson["Position"]["z"]
                    );
                    glm::vec3 scale(
                        modelJson["Scale"]["x"],
                        modelJson["Scale"]["y"],
                        modelJson["Scale"]["z"]
                    );
                    float rotation = modelJson["Rotation"];

                    if (modelType == "STATIC") {
                        modelManager.AddModelStatic(modelPath.c_str(), position, scale, rotation);
                    } else if (modelType == "ANIMATED") {
                        modelManager.AddModelAnimated(modelPath.c_str(), position, scale, rotation);
                    }
                }
                puts("Models data loaded successfully");
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

  ModelManager modelManager;
  LightManager lightManager;
  bool isRendered = false;
};
