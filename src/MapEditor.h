#pragma once
#include "GLFW/glfw3.h"
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

#include "ImGuizmo.h"
#include "Utilities.hpp"

struct SceneEditor
{
  SceneEditor(ModelManager& model, LightManager& light) : modelManager(model), lightManager(light){}

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
    ImGuizmo::BeginFrame();

    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

    ImGui::Begin("Scene Editor");

    if (ImGui::BeginMainMenuBar()) {

        if (ImGui::Button("Save Scene"))
        {
          SaveScene();
        }

        if (ImGui::Button("Load Scene"))
        {
           LoadScene(); 
        }

        ImGui::EndMainMenuBar();
    }

      if(ImGui::TreeNode("Camera Properties"))
      {
        ImGui::Columns(2, nullptr, false); // 2 columns, no borders
        ImGui::SetColumnWidth(0, 50);      // Set the width of the first column

        // X Value
        ImGui::Text("X:");
        ImGui::NextColumn();
        ImGui::PushItemWidth(150.0f);      // Set custom width for DragFloat
        ImGui::DragFloat("##X", &Window::_camera.Position.x, 0.1f, -FLT_MAX, FLT_MAX, "%.2f", ImGuiSliderFlags_None);
        ImGui::PopItemWidth();

        ImGui::NextColumn();
        // Y Value
        ImGui::Text("Y:");
        ImGui::NextColumn();
        ImGui::PushItemWidth(150.0f);
        ImGui::DragFloat("##Y", &Window::_camera.Position.y, 0.1f, -FLT_MAX, FLT_MAX, "%.2f", ImGuiSliderFlags_None);
        ImGui::PopItemWidth();

        ImGui::NextColumn();
        // Z Value
        ImGui::Text("Z:");
        ImGui::NextColumn();
        ImGui::PushItemWidth(150.0f);
        ImGui::DragFloat("##Z", &Window::_camera.Position.z, 0.1f, -FLT_MAX, FLT_MAX, "%.2f", ImGuiSliderFlags_None);
        ImGui::PopItemWidth();

        ImGui::NextColumn();
        // Yaw Value
        ImGui::Text("Yaw:");
        ImGui::NextColumn();
        ImGui::PushItemWidth(150.0f);
        ImGui::DragFloat("##Yaw", &Window::_camera.Yaw, 0.5f, -180.0f, 180.0f, "%.2f", ImGuiSliderFlags_None);
        ImGui::PopItemWidth();

        ImGui::NextColumn();
        // Pitch Value
        ImGui::Text("Pitch:");
        ImGui::NextColumn();
        ImGui::PushItemWidth(150.0f);
        ImGui::DragFloat("##Pitch", &Window::_camera.Pitch, 0.5f, -90.0f, 90.0f, "%.2f", ImGuiSliderFlags_None);
        ImGui::PopItemWidth();

        ImGui::Columns(1); // Reset to one column
        ImGui::TreePop();
      }

        if (!lightManager.lights.empty()) {


            for (int i = 0; i < lightManager.lights.size(); ++i) {
                auto& currentLight = lightManager.lights[i];
                std::string lightNodeLabel = "Light " + std::to_string(i);
                
                if (ImGui::TreeNode(lightNodeLabel.c_str())) {
                    // Display and edit light properties inside this tree node
                    lightColor = glm::vec3(currentLight.second.color);
                    lightPosition = currentLight.second.position;
                    lightScale = currentLight.second.scale;
                    lightRotation = currentLight.second.rotation;

                    // Edit Color
                    ImGui::PushItemWidth(200.0f);
                    if (ImGui::ColorEdit3("lightColor", &lightColor[0])) {
                        currentLight.second.color = glm::vec4(lightColor, 1.0f); 
                    }
                    ImGui::PopItemWidth();

                    // Edit Position
                    ImGui::PushItemWidth(200.0f);
                    if (ImGui::DragFloat3("lightPosition", &lightPosition[0])) {
                        currentLight.second.position = glm::vec4(lightPosition, currentLight.second.position.w);
                    }
                    ImGui::PopItemWidth();

                    // Edit Scale
                    ImGui::PushItemWidth(200.0f);
                    if (ImGui::DragFloat3("lightScale", &lightScale[0])) {
                        currentLight.second.scale = lightScale; 
                    }
                    ImGui::PopItemWidth();

                    // Edit Rotation
                    ImGui::PushItemWidth(200.0f);
                    if (ImGui::SliderFloat("lightRotation", &lightRotation, 0.0f, 360.0f)) {
                        currentLight.second.rotation = lightRotation; 
                    }
                    ImGui::PopItemWidth();

                    ImGui::PushItemWidth(200.0f);
                    if (ImGui::Button("Edit Light")) {
                        glm::vec4 colorWithAlpha(lightColor, 1.0f);
                        lightManager.EditLight(i, colorWithAlpha, lightPosition, lightScale, lightRotation);
                    }
                    ImGui::PopItemWidth();

                    ImGui::PushItemWidth(200.0f);
                    if (ImGui::Button("Remove Light")) {
                        lightManager.RemoveLight(i);
                    } 
                    ImGui::PopItemWidth();

          
                    ImGui::TreePop(); // End current light node
                }
            }

        }


        // Collect model names from static and animated models
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
            // Tree nodes for static models
            if (!modelManager.vec_staticModels.empty()) {
                for (size_t i = 0; i < modelManager.vec_staticModels.size(); ++i) {
                    auto& model = modelManager.vec_staticModels[i];
                    const char* filename = strrchr(model.second.modelpath.c_str(), '/');
                    filename = (filename == nullptr) ? model.second.modelpath.c_str() : filename + 1;
                    if (ImGui::TreeNode(filename)) {

                        modelPosition = model.second.position;
                        modelScale = model.second.scale;
                        modelRotation = model.second.rotation;

                        // Properties of the model
                        ImGui::PushItemWidth(200.0f);
                        if(ImGui::DragFloat3("Position", &modelPosition[0])){
                            model.second.position = modelPosition;
                        }
                        ImGui::PopItemWidth();
                        
                        ImGui::PushItemWidth(200.0f);
                        if(ImGui::DragFloat3("Scale", &modelScale[0])){
                            model.second.scale = modelScale;
                        }
                        ImGui::PopItemWidth();

                        ImGui::PushItemWidth(200.0f);
                        if(ImGui::SliderFloat("Rotation", &modelRotation, 0.0f, 360.0f)){
                            model.second.rotation = modelRotation;
                        }
                        ImGui::PopItemWidth();

                        ImGui::Text("Type: STATIC");

                        ImGui::PushItemWidth(200.0f);
                        if (ImGui::Button("Remove Model")) {
                            modelManager.RemoveStaticModel(i);
                        }
                        ImGui::PopItemWidth();

                        ImGui::TreePop();
                    }
                }
            }

            // Tree nodes for animated models
            if (!modelManager.vec_animatedModels.empty()) {
                for (size_t i = 0; i < modelManager.vec_animatedModels.size(); ++i) {
                    auto& model = modelManager.vec_animatedModels[i];
                    const char* filename = strrchr(model.second.modelpath.c_str(), '/');
                    filename = (filename == nullptr) ? model.second.modelpath.c_str() : filename + 1;
                    if (ImGui::TreeNode(filename)) {

                        modelPosition = model.second.position;
                        modelScale = model.second.scale;
                        modelRotation = model.second.rotation;
                        // Properties of the model
                        ImGui::PushItemWidth(200.0f);
                        if(ImGui::DragFloat3("Position", &modelPosition[0])){
                            model.second.position = modelPosition;
                        }
                        ImGui::PopItemWidth();
                        
                        ImGui::PushItemWidth(200.0f);
                        if(ImGui::DragFloat3("Scale", &modelScale[0])){
                            model.second.scale = modelScale;
                        }
                        ImGui::PopItemWidth();

                        ImGui::PushItemWidth(200.0f);
                        if(ImGui::SliderFloat("Rotation", &modelRotation, 0.0f, 360.0f)){
                            model.second.rotation = modelRotation;
                        }
                        ImGui::PopItemWidth();

                        ImGui::Text("Type: ANIMATED");

                        ImGui::PushItemWidth(200.0f);
                        if (ImGui::Button("Remove Model")) {
                            modelManager.RemoveAnimatedModel(i);
                        }
                        ImGui::PopItemWidth();

                        ImGui::TreePop();
                    }
                }
            }
        
          if (totalModels > 0) {
              glm::mat4 modelMatrix = glm::mat4(1.0f);

              // Get the selected model's matrix
              if (selectedModelIndex < modelManager.vec_staticModels.size()) {
                  const auto& selectedModel = modelManager.vec_staticModels[selectedModelIndex];
                  modelMatrix = glm::translate(glm::mat4(1.0f), selectedModel.second.position);
                  modelMatrix = glm::rotate(modelMatrix, glm::radians(selectedModel.second.rotation), glm::vec3(0.0f, 1.0f, 0.0f));
                  modelMatrix = glm::scale(modelMatrix, selectedModel.second.scale);
              } else {
                  int animatedModelIndex = selectedModelIndex - modelManager.vec_staticModels.size();
                  const auto& selectedModel = modelManager.vec_animatedModels[animatedModelIndex];
                  modelMatrix = glm::translate(glm::mat4(1.0f), selectedModel.second.position);
                  modelMatrix = glm::rotate(modelMatrix, glm::radians(selectedModel.second.rotation), glm::vec3(0.0f, 1.0f, 0.0f));
                  modelMatrix = glm::scale(modelMatrix, selectedModel.second.scale);
              }

              static auto currentManipulationMode = ImGuizmo::TRANSLATE;  

              if (Input::KeyPressed(KEY_S)) {
                  currentManipulationMode = ImGuizmo::SCALE;
              } else if (Input::KeyPressed(KEY_G)) {
                  currentManipulationMode = ImGuizmo::TRANSLATE;
              } else if (Input::KeyPressed(KEY_R)) {
                  currentManipulationMode = ImGuizmo::ROTATE;
              }

              ImGuizmo::SetOrthographic(false);
              ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, (float)Window::GetWindowWidth(), (float)Window::GetWindowHeight());
              ImGuizmo::SetDrawlist();
              glm::mat4 projection = glm::perspective(glm::radians(45.0f), Window::getAspectRatio(), 0.001f, 2000.0f);
              glm::mat4 view = Window::_camera.GetViewMatrix();

              ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), currentManipulationMode, ImGuizmo::WORLD, glm::value_ptr(modelMatrix));
             
              if(ImGuizmo::IsUsing())
              {
                  glm::vec3 position, rotation, scale;
                  Utilities::DecomposeTransform(modelMatrix, position, rotation, scale);

                  glm::vec3 newPosition(modelMatrix[3]);
                  glm::quat newRotation = glm::quat_cast(modelMatrix);
                  glm::vec3 newScale(glm::length(modelMatrix[0]), glm::length(modelMatrix[1]), glm::length(modelMatrix[2]));

                  // Update the model's transformation after manipulation
                  if (selectedModelIndex < modelManager.vec_staticModels.size()) {
                      auto& selectedModel = modelManager.vec_staticModels[selectedModelIndex];
                      selectedModel.second.position = newPosition;
                      selectedModel.second.rotation = glm::degrees(glm::eulerAngles(newRotation).y);
                      selectedModel.second.scale = newScale;
                  } else {
                      int animatedModelIndex = selectedModelIndex - modelManager.vec_staticModels.size();
                      auto& selectedModel = modelManager.vec_animatedModels[animatedModelIndex];
                      selectedModel.second.position = newPosition;
                      selectedModel.second.rotation = glm::degrees(glm::eulerAngles(newRotation).y);
                      selectedModel.second.scale = newScale;
                  }
              }
          }
      } 
    
    if(ImGui::IsMouseClicked(GLFW_MOUSE_BUTTON_RIGHT)) ImGui::OpenPopup("AddEntityPopup");
    if (ImGui::BeginPopup("AddEntityPopup",ImGuiWindowFlags_MenuBar)) {

        if (ImGui::BeginMenuBar()){
            ImGui::Text("Add Entity");
            ImGui::EndMenuBar();
        }

        if (ImGui::Button("Add Light")) {
            glm::vec4 colorWithAlpha(glm::vec3(0.994f,0.994f,0.994f), 1.0f);
            lightManager.AddLight(colorWithAlpha, glm::vec3(0.0f));
        }

        if (ImGui::Button("Add Model")) ImGui::OpenPopup("my_file_popup");
        if (ImGui::BeginPopup("my_file_popup", ImGuiWindowFlags_MenuBar))
        {
            // Model Path
            ImGui::Text("Model Path:");
            ImGui::SameLine();
            ImGui::PushItemWidth(150.0f);
            ImGui::InputText("##ModelPath", modelPathBuffer, sizeof(modelPathBuffer));
            ImGui::PopItemWidth();

            // Model Type
            ImGui::Text("Model Type:");
            ImGui::SameLine();
            ImGui::PushItemWidth(150.0f);
            const char* modelTypeOptions[] = { "STATIC", "ANIMATED" };
            ImGui::Combo("##ModelType", &selectedModelTypeIndex, modelTypeOptions, IM_ARRAYSIZE(modelTypeOptions));
            ImGui::PopItemWidth();

            if(ImGui::Button("Add")){
              if (strlen(modelPathBuffer) > 0) {
                  std::string modelPathString = modelPathBuffer;
                  const char* modelType = modelTypeOptions[selectedModelTypeIndex];

                  if (std::strcmp(modelType, "STATIC") == 0) {
                      modelManager.AddModelStatic(modelPathString, modelPosition, modelScale, modelRotation);
                  }

                  if (std::strcmp(modelType, "ANIMATED") == 0) {
                      modelManager.AddModelAnimated(modelPathString, modelPosition, modelScale, modelRotation);
                  }

                  modelPathBuffer[0] = '\0';  
              }
            }
            ImGui::EndPopup();
        }
      ImGui::EndPopup();
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

  void SaveScene()
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

  int selectedLightIndex = 0; 
  glm::vec3 lightPosition;
  glm::vec3 lightScale;
  float lightRotation;
  glm::vec3 lightColor;


  int selectedModelIndex = 0;
  glm::vec3 modelPosition;
  glm::vec3 modelScale;
  float modelRotation;

  char modelPathBuffer[256] = "";  // Input buffer for model path
  int selectedModelTypeIndex = 0;  // 0 for static, 1 for animated

  std::vector<std::string> modelNames;
  int totalModels = 0;


  ModelManager& modelManager;
  LightManager& lightManager;
  bool isRendered = false;
};
