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
#include <cstdint>
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

                ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow;
                if (selectedModelIndex == modelManager.vec_staticModels.size() + i) {
                    nodeFlags |= ImGuiTreeNodeFlags_Selected;
                }
                
                if (ImGui::TreeNodeEx(lightNodeLabel.c_str(),nodeFlags)) {

                    if (ImGui::IsItemClicked()) {
                        selectedModelIndex = i;
                    }

                    if (selectedModelIndex == i) {
                        UpdateGizmoTransform(currentLight);
                    }

                    lightColor = glm::vec3(currentLight.second.color);
                    lightPosition = currentLight.second.position;
                    lightRotation = currentLight.second.rotation;
                    lightScale = currentLight.second.scale;

                    ImGui::PushItemWidth(200.0f);
                    if (ImGui::ColorEdit3("lightColor", &lightColor[0])) {
                        currentLight.second.color = glm::vec4(lightColor, 1.0f); 
                    }
                    ImGui::PopItemWidth();

                    DrawVec3Control("Position", lightPosition);
                    DrawVec3Control("Rotation", lightRotation);
                    DrawVec3Control("Scale", lightScale);

                    if (lightPosition != glm::vec3(currentLight.second.position)) {
                        currentLight.second.position = lightPosition;
                    }
                    if (modelRotation != currentLight.second.rotation) {
                        currentLight.second.rotation = lightRotation;
                    }
                    if (modelScale != currentLight.second.scale) {
                        currentLight.second.scale = lightScale;
                    }

                    lightManager.EditLight(i,currentLight.second.color,currentLight.second.position);

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

                  ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow;
                  if (selectedModelIndex == i) {
                      nodeFlags |= ImGuiTreeNodeFlags_Selected;
                  }

                  if (ImGui::TreeNodeEx(filename, nodeFlags)) {
                      // Set the selected model index
                      if (ImGui::IsItemClicked()) {
                          selectedModelIndex = i;
                      }

                      if (selectedModelIndex == i) {
                          UpdateGizmoTransform(model);
                      }

                      modelPosition = model.second.position;
                      modelRotation = model.second.rotation;
                      modelScale = model.second.scale;

                      DrawVec3Control("Position", modelPosition);
                      DrawVec3Control("Rotation", modelRotation);
                      DrawVec3Control("Scale", modelScale);

                      if (modelPosition != model.second.position) {
                          model.second.position = modelPosition;
                      }
                      if (modelRotation != model.second.rotation) {
                          model.second.rotation = modelRotation;
                      }
                      if (modelScale != model.second.scale) {
                          model.second.scale = modelScale;
                      }

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

                  ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow;
                  if (selectedModelIndex == modelManager.vec_staticModels.size() + i) {
                      nodeFlags |= ImGuiTreeNodeFlags_Selected;
                  }

                  if (ImGui::TreeNodeEx(filename, nodeFlags)) {
                      
                      if (ImGui::IsItemClicked()) {
                          selectedModelIndex = i;
                      }

                      if (selectedModelIndex == i) {
                          UpdateGizmoTransform(model);
                      }

                      modelPosition = model.second.position;
                      modelRotation = model.second.rotation;
                      modelScale = model.second.scale;

                      DrawVec3Control("Position", modelPosition);
                      DrawVec3Control("Rotation", modelRotation);
                      DrawVec3Control("Scale", modelScale);

                      if (modelPosition != model.second.position) {
                          model.second.position = modelPosition;
                      }
                      if (modelRotation != model.second.rotation) {
                          model.second.rotation = modelRotation;
                      }
                      if (modelScale != model.second.scale) {
                          model.second.scale = modelScale;
                      }

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
      }
    
    if(ImGui::IsMouseClicked(GLFW_MOUSE_BUTTON_RIGHT)) ImGui::OpenPopup("AddEntityPopup");
    if (ImGui::BeginPopup("AddEntityPopup",ImGuiWindowFlags_MenuBar)) {

        if (ImGui::BeginMenuBar()){
            ImGui::Text("Add Entity");
            ImGui::EndMenuBar();
        }

        
        if (ImGui::Button("Add Light")) ImGui::OpenPopup("light_add_popup");
        if (ImGui::BeginPopup("light_add_popup", ImGuiWindowFlags_MenuBar)) {

            ImGui::Text("Model Type:");
            ImGui::SameLine();
            ImGui::PushItemWidth(150.0f);
            const char* lightTypeOptions[] = { "POINT", "DIRECTIONAL", "SPOT" };
            ImGui::Combo("##LightType", &selectedLightTypeIndex, lightTypeOptions, IM_ARRAYSIZE(lightTypeOptions));
            ImGui::PopItemWidth();

            glm::vec4 colorWithAlpha(glm::vec3(0.994f,0.994f,0.994f), 1.0f);
            if(ImGui::Button("Add")){
              const char* lightType = lightTypeOptions[selectedLightTypeIndex];

              if (std::strcmp(lightType, "POINT") == 0) {
                  lightManager.AddLight(LightType::POINT, colorWithAlpha, glm::vec3(0.0f));
              }

              if (std::strcmp(lightType, "DIRECTIONAL") == 0) {
                  lightManager.AddLight(LightType::DIRECTIONAL, colorWithAlpha, glm::vec3(0.0f));
              }

              if (std::strcmp(lightType, "SPOT") == 0) {
                  lightManager.AddLight(LightType::SPOT, colorWithAlpha, glm::vec3(0.0f));
              }
            }
          ImGui::EndPopup();
        }

        if (ImGui::Button("Add Model")) ImGui::OpenPopup("model_add_popup");
        if (ImGui::BeginPopup("model_add_popup", ImGuiWindowFlags_MenuBar))
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
                      modelManager.AddModelStatic(modelPathString, modelPosition, modelRotation, modelScale);
                  }

                  if (std::strcmp(modelType, "ANIMATED") == 0) {
                      modelManager.AddModelAnimated(modelPathString, modelPosition, modelRotation, modelScale);
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
          lightJson["Type"] = light.second.type;
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
          lightJson["Rotation"] = {
              {"x", lightData.rotation.x},
              {"y", lightData.rotation.y},
              {"z", lightData.rotation.z}
          };
          lightJson["Scale"] = {
              {"x", lightData.scale.x},
              {"y", lightData.scale.y},
              {"z", lightData.scale.z}
          };
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
          modelJson["Rotation"] = {
              {"x", model.second.rotation.x},
              {"y", model.second.rotation.y},
              {"z", model.second.rotation.z}
          };
          modelJson["Scale"] = {
              {"x", model.second.scale.x},
              {"y", model.second.scale.y},
              {"z", model.second.scale.z}
          };
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
          modelJson["Rotation"] = {
              {"x", model.second.rotation.x},
              {"y", model.second.rotation.y},
              {"z", model.second.rotation.z}
          };
          modelJson["Scale"] = {
              {"x", model.second.scale.x},
              {"y", model.second.scale.y},
              {"z", model.second.scale.z}
          };
          sceneData["Models"].push_back(modelJson);
      }

      std::ofstream outFile("./scenes/scene.json");
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
    std::ifstream inFile("./scenes/scene.json");
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
                for (const auto& lightJson : sceneData["Lights"]) {
                    int32_t modelType = lightJson["Type"];
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
                    glm::vec3 rotation(
                        lightJson["Rotation"]["x"],
                        lightJson["Rotation"]["y"],
                        lightJson["Rotation"]["z"]
                    );
                    glm::vec3 scale(
                        lightJson["Scale"]["x"],
                        lightJson["Scale"]["y"],
                        lightJson["Scale"]["z"]
                    );

                    if (modelType == 0) {
                        lightManager.AddLight(LightType::POINT, color, position, rotation, scale); 
                    }

                    if (modelType == 1) {
                        lightManager.AddLight(LightType::DIRECTIONAL, color, position, rotation, scale); 
                    }

                    if (modelType == 2) {
                        lightManager.AddLight(LightType::SPOT, color, position, rotation, scale); 
                    }
                }
                puts("Lights data loaded successfully");
            }

            if (sceneData.contains("Models")) {
                modelManager.vec_staticModels.clear(); 
                modelManager.vec_animatedModels.clear(); 
                for (const auto& modelJson : sceneData["Models"]) {
                    std::string modelType = modelJson["Type"];
                    std::string modelPath = modelJson["Path"].get<std::string>();
                    glm::vec3 position(
                        modelJson["Position"]["x"],
                        modelJson["Position"]["y"],
                        modelJson["Position"]["z"]
                    );
                    glm::vec3 rotation(
                        modelJson["Rotation"]["x"],
                        modelJson["Rotation"]["y"],
                        modelJson["Rotation"]["z"]
                    );
                    glm::vec3 scale(
                        modelJson["Scale"]["x"],
                        modelJson["Scale"]["y"],
                        modelJson["Scale"]["z"]
                    );

                    if (modelType == "STATIC") {
                        modelManager.AddModelStatic(modelPath, position, rotation, scale);
                    } else if (modelType == "ANIMATED") {
                        modelManager.AddModelAnimated(modelPath, position, rotation, scale);
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
  glm::vec3 lightColor;
  glm::vec3 lightPosition;
  glm::vec3 lightRotation;
  glm::vec3 lightScale;


  int selectedModelIndex = 0;
  glm::vec3 modelPosition;
  glm::vec3 modelRotation;
  glm::vec3 modelScale;

  char modelPathBuffer[256] = "";  // Input buffer for model path
  int selectedModelTypeIndex = 0;  // 0 for static, 1 for animated
  int selectedLightTypeIndex = 0;  // 0 for static, 1 for animated

  std::vector<std::string> modelNames;
  int totalModels = 0;


  ModelManager& modelManager;
  LightManager& lightManager;
  bool isRendered = false;

  template<typename T>
  void UpdateGizmoTransform(T& entity) {
      glm::mat4 modelMatrix = glm::mat4(1.0f);

      // Generate transformation matrix
      modelMatrix = glm::translate(glm::mat4(1.0f), entity.second.position);
      modelMatrix = glm::rotate(modelMatrix, glm::radians(entity.second.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
      modelMatrix = glm::rotate(modelMatrix, glm::radians(entity.second.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
      modelMatrix = glm::rotate(modelMatrix, glm::radians(entity.second.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
      modelMatrix = glm::scale(modelMatrix, entity.second.scale);

      ImGuizmo::SetOrthographic(false);
      ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, (float)Window::GetWindowWidth(), (float)Window::GetWindowHeight());
      ImGuizmo::SetDrawlist();

      glm::mat4 projection = glm::perspective(glm::radians(45.0f), Window::getAspectRatio(), 0.001f, 2000.0f);
      glm::mat4 view = Window::_camera.GetViewMatrix();

      static auto currentManipulationMode = ImGuizmo::TRANSLATE;
      if (Input::KeyPressed(KEY_S)) {
          currentManipulationMode = ImGuizmo::SCALE;
      } else if (Input::KeyPressed(KEY_G)) {
          currentManipulationMode = ImGuizmo::TRANSLATE;
      } else if (Input::KeyPressed(KEY_R)) {
          currentManipulationMode = ImGuizmo::ROTATE;
      }

      ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), currentManipulationMode, ImGuizmo::LOCAL, glm::value_ptr(modelMatrix));

      if (ImGuizmo::IsUsing()) {
          glm::vec3 position, rotation, scale;
          Utilities::DecomposeTransform(modelMatrix, position, rotation, scale);

          entity.second.position = position;
          entity.second.rotation.x = glm::degrees(rotation.x);
          entity.second.rotation.y = glm::degrees(rotation.y);
          entity.second.rotation.z = glm::degrees(rotation.z);
          entity.second.scale = scale;
      }
  }

  void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
  {
      ImGuiIO& io = ImGui::GetIO();
      auto boldFont = io.Fonts->Fonts[0];

      ImGui::PushID(label.c_str());

      ImGui::Columns(2);
      ImGui::SetColumnWidth(0, columnWidth);
      ImGui::Text(label.c_str());
      ImGui::NextColumn();

      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

      float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
      ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

      // X
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.9f, 0.2f, 0.2f, 1.0f});
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
      ImGui::PushFont(boldFont);
      if (ImGui::Button("X", buttonSize)){}
      ImGui::PopFont();
      ImGui::PopStyleColor(3);

      ImGui::SameLine();
      ImGui::SetNextItemWidth(ImGui::CalcItemWidth() / 10.0f);
      ImGui::DragFloat("##X", &values.x, 0.1f, -FLT_MAX, FLT_MAX, "%.2f");

      // Y
      ImGui::SameLine();
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.8f, 0.3f, 1.0f});
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
      ImGui::PushFont(boldFont);
      if (ImGui::Button("Y", buttonSize)){}
      ImGui::PopFont();
      ImGui::PopStyleColor(3);

      ImGui::SameLine();
      ImGui::SetNextItemWidth(ImGui::CalcItemWidth() / 10.0f);
      ImGui::DragFloat("##Y", &values.y, 0.1f, -FLT_MAX, FLT_MAX, "%.2f");

      // Z
      ImGui::SameLine();
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.35f, 0.9f, 1.0f});
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
      ImGui::PushFont(boldFont);
      if (ImGui::Button("Z", buttonSize)){}
      ImGui::PopFont();
      ImGui::PopStyleColor(3);

      ImGui::SameLine();
      ImGui::SetNextItemWidth(ImGui::CalcItemWidth() / 10.0f);
      ImGui::DragFloat("##Z", &values.z, 0.1f, -FLT_MAX, FLT_MAX, "%.2f");

      ImGui::PopStyleVar();
      ImGui::Columns(1);
      ImGui::PopID();
  }
};
