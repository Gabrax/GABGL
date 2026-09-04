#include "SceneManager.h"


#include "RenderSystem.h"
#include "AudioManager.h"
#include "Camera.h"
#include <gabdebug.h>
#include "ParticleRenderer.h"
#include "RenderBackend.h"
#include "../Input/UserInput.h"
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <unordered_map>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

namespace
{
  constexpr const char* SceneFilePath = "../res/scenes/scene.json";

  std::unordered_map<std::string, SceneManager::SceneFactory> s_SceneFactories;
  SceneManager::InteractionHandlers s_InteractionHandlers;

  const char* LightTypeName(LightType type)
  {
    switch (type)
    {
      case LightType::DIRECT: return "direct";
      case LightType::SPOT: return "spot";
      default: return "point";
    }
  }

  LightType ParseLightType(const std::string& type)
  {
    if (type == "direct" || type == "directional") return LightType::DIRECT;
    if (type == "spot") return LightType::SPOT;
    return LightType::POINT;
  }

  bool ReadVec3(const json& value, glm::vec3& result)
  {
    if (!value.is_array() || value.size() != 3 ||
        !value[0].is_number() || !value[1].is_number() || !value[2].is_number())
      return false;
    result = glm::vec3(value[0].get<float>(), value[1].get<float>(), value[2].get<float>());
    return true;
  }

  RenderLight ToRenderLight(const SceneLight& light)
  {
    return {light.type, light.color, light.position, light.rotation};
  }

  struct GenericScene : Scene
  {
    explicit GenericScene(const std::string& name) : Scene(name) {}
    void OnSceneStart() override {}
    void OnUpdate(DeltaTime& dt) override
    {
      RenderSystem::DrawScene(dt, []() {});
    }
  };
}

void Scene::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT(Scene::OnKeyPressed));
	dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT(Scene::OnMouseButtonPressed));
}

bool Scene::OnKeyPressed(KeyPressedEvent& e)
{
	// Shortcuts
	if (e.IsRepeat()) return false;

	switch (e.GetKeyCode())
	{
    case Key::Tab:
    {
      RenderSystem::SwitchRenderState();
      break;
    }
	}

	return false;
}

bool Scene::OnMouseButtonPressed(MouseButtonPressedEvent& e)
{
	/*if (e.GetMouseButton() == Mouse::ButtonLeft)*/
	/*{*/
	/*	if (m_ViewportHovered && !ImGuizmo::IsOver() && !Input::IsKeyPressed(Key::LeftAlt))*/
	/*		m_SelectionContext = m_HoveredEntity;*/
	/*}*/
	return false;
}

void Scene::StartLoading()
{
  if(m_Assets.loadingStarted)
      return;

  LoadSceneFromJSON(SceneFilePath,m_Name);

  m_Assets.loadingStarted = true;
  m_Assets.loadingDone = false;

  for(auto& s : m_Assets.sounds) AudioManager::LoadSound(s.c_str());

  for(auto& m : m_Assets.music) AudioManager::LoadMusic(m.c_str());

  for(const auto& model : m_Assets.static_models)
  {
    m_Assets.futureStatic.push_back(
        std::async(std::launch::async,
            Model::CreateSTATIC,
            model.path.c_str(),
            model.scale,
            model.flag,
            model.meshType));
  }

  for(const auto& model : m_Assets.animated_models)
  {
    m_Assets.futureAnim.push_back(
        std::async(std::launch::async,
            Model::CreateANIMATED,
            model.path.c_str(),
            model.scale,
            model.flag,
            model.meshType));
  }

  m_Assets.futureTextures.push_back(
      std::async(std::launch::async,[skybox=m_Assets.skybox]()
      {
          return Texture::CreateCUBEMAP(skybox);
      }));
}

void Scene::SpawnEntities()
{
  m_EditorEntities.clear();
  m_NextEntityId = 1;

  for(auto& e : m_Assets.entities)
  {
    const std::string modelName = e["model"];
    const auto sceneModel = ModelManager::GetModel(modelName);
    if (!sceneModel || sceneModel->GetPhysXMeshType() == MeshType::CONVEXMESH)
      continue;

    const std::string type = e.value("type", "static");
    uint32_t ordinal = 0;

    auto spawn = [&](const json& transformDesc)
    {
      const auto position = transformDesc.value("position", std::vector<float>{0.0f, 0.0f, 0.0f});
      const auto rotation = transformDesc.value("rotation", std::vector<float>{0.0f, 0.0f, 0.0f});
      const auto scale = transformDesc.value("scale", std::vector<float>{1.0f, 1.0f, 1.0f});

      if (position.size() != 3 || rotation.size() != 3 || scale.size() != 3)
      {
        gablog_log(LOG_WARN, __FILE__, __LINE__, "Invalid transform for model '%s': position, rotation and scale must contain 3 values",
          modelName.c_str());
        return;
      }

      Transform transform(
        {position[0], position[1], position[2]},
        {rotation[0], rotation[1], rotation[2]},
        {scale[0], scale[1], scale[2]});

      if (type == "controller")
      {
        const float radius = std::max(0.01f, transformDesc.value("controller_radius", e.value("controller_radius", 1.0f)));
        const float height = std::max(0.01f, transformDesc.value("controller_height", e.value("controller_height", 1.0f)));
        ModelManager::SetInitialControllerTransform(modelName, transform, radius, height, true);
      }
      else
      {
        const auto model = ModelManager::GetModel(modelName);
        if (!model)
          return;

        if (model->m_InstanceTransforms.empty())
          ModelManager::SetInitialModelTransform(modelName, transform.GetTransform());
        else
          ModelManager::AddModelInstance(modelName, transform.GetTransform());
      }

      const auto model = ModelManager::GetModel(modelName);
      if (!model || model->m_InstanceTransforms.empty())
        return;

      SceneEntity entity;
      entity.id = m_NextEntityId++;
      entity.model = modelName;
      entity.type = type;
      entity.transform = transform;
      entity.instanceIndex = type == "controller"
        ? 0
        : static_cast<uint32_t>(model->m_InstanceTransforms.size() - 1);
      if (type == "controller")
      {
        entity.controllerRadius = model->GetControllerRadius();
        entity.controllerHeight = model->GetControllerHeight();
      }

      const std::string baseName = transformDesc.value("name", e.value("name", modelName));
      entity.name = ordinal == 0 ? baseName : baseName + " #" + std::to_string(ordinal + 1);
      entity.itemName = transformDesc.value("item_name", e.value("item_name", baseName));
      entity.pickable = transformDesc.value("pickable", e.value("pickable", false));
      const json* dialogue = nullptr;
      if (transformDesc.contains("dialogue")) dialogue = &transformDesc["dialogue"];
      else if (e.contains("dialogue")) dialogue = &e["dialogue"];
      if (dialogue && dialogue->is_array())
      {
        for (const auto& line : *dialogue)
          if (line.is_string() && !line.get_ref<const std::string&>().empty())
            entity.dialogue.push_back(line.get<std::string>());
      }
      entity.interactable = entity.pickable || !entity.dialogue.empty() ||
        transformDesc.value("interactable", e.value("interactable", false));
      entity.player = transformDesc.value("player", e.value("player", false));
      entity.interactionRange = std::max(0.1f, transformDesc.value("interaction_range", e.value("interaction_range", 4.0f)));
      entity.labelHeight = transformDesc.value("label_height", e.value("label_height", 1.5f));
      ++ordinal;
      m_EditorEntities.push_back(std::move(entity));
    };

    if (e.contains("instances"))
    {
      for (const auto& instance : e["instances"]) spawn(instance);
    }
    else
    {
      spawn(e);
    }
  }

}

void Scene::SpawnLights()
{
  m_EditorLights.clear();
  m_NextLightId = 1;

  if (!m_Assets.lights.is_array()) return;

  for (const auto& description : m_Assets.lights)
  {
    if (!description.is_object()) continue;

    SceneLight light;
    light.type = ParseLightType(description.value("type", "point"));
    if (light.type == LightType::DIRECT &&
        std::ranges::any_of(m_EditorLights, [](const SceneLight& existing) { return existing.type == LightType::DIRECT; }))
    {
      gablog_log(LOG_WARN, __FILE__, __LINE__, "Scene '%s' contains more than one directional light; ignoring the duplicate",
        m_Name.c_str());
      continue;
    }

    const glm::vec3 defaultRotation = light.type == LightType::DIRECT
      ? glm::vec3(-1.0f, -2.0f, -1.0f)
      : glm::vec3(0.0f, -1.0f, 0.0f);
    light.rotation = defaultRotation;
    if (description.contains("color") && !ReadVec3(description["color"], light.color))
      gablog_log(LOG_WARN, __FILE__, __LINE__, "Invalid color for a light in scene '%s'; using white", m_Name.c_str());
    if (description.contains("position") && !ReadVec3(description["position"], light.position))
      gablog_log(LOG_WARN, __FILE__, __LINE__, "Invalid position for a light in scene '%s'; using origin", m_Name.c_str());
    if (description.contains("rotation") && !ReadVec3(description["rotation"], light.rotation))
      gablog_log(LOG_WARN, __FILE__, __LINE__, "Invalid rotation for a light in scene '%s'; using the default", m_Name.c_str());

    light.id = m_NextLightId++;
    light.name = description.value("name", std::string(LightTypeName(light.type)) + " light " + std::to_string(light.id));
    if (!RenderBackend::AddLight(ToRenderLight(light)))
    {
      gablog_log(LOG_WARN, __FILE__, __LINE__, "Renderer rejected light '%s'", light.name.c_str());
      continue;
    }
    m_EditorLights.push_back(std::move(light));
  }
}

SceneEntity* Scene::FindEntity(uint64_t entityId)
{
  const auto it = std::find_if(m_EditorEntities.begin(), m_EditorEntities.end(),
    [entityId](const SceneEntity& entity) { return entity.id == entityId; });
  return it == m_EditorEntities.end() ? nullptr : &(*it);
}

SceneLight* Scene::FindLight(uint64_t lightId)
{
  const auto it = std::ranges::find_if(m_EditorLights, [lightId](const SceneLight& light) { return light.id == lightId; });
  return it == m_EditorLights.end() ? nullptr : &(*it);
}

uint64_t Scene::AddLight(LightType type)
{
  if (type == LightType::DIRECT &&
      std::ranges::any_of(m_EditorLights, [](const SceneLight& light) { return light.type == LightType::DIRECT; }))
    return 0;

  SceneLight light;
  light.id = m_NextLightId++;
  light.type = type;
  light.name = std::string(LightTypeName(type)) + " light " + std::to_string(light.id);
  light.position = Camera::GetPosition();
  light.rotation = type == LightType::DIRECT
    ? glm::vec3(-1.0f, -2.0f, -1.0f)
    : Camera::GetForwardDirection();

  if (!RenderBackend::AddLight(ToRenderLight(light))) return 0;
  m_EditorLights.push_back(std::move(light));
  return m_EditorLights.back().id;
}

bool Scene::UpdateLight(uint64_t lightId, const std::string& name, const glm::vec3& color,
  const glm::vec3& position, const glm::vec3& rotation)
{
  const auto it = std::find_if(m_EditorLights.begin(), m_EditorLights.end(),
    [lightId](const SceneLight& light) { return light.id == lightId; });
  if (it == m_EditorLights.end()) return false;

  it->name = name;
  it->color = glm::max(color, glm::vec3(0.0f));
  it->position = position;
  it->rotation = rotation;
  const auto rendererIndex = static_cast<size_t>(std::distance(m_EditorLights.begin(), it));
  return RenderBackend::UpdateLight(rendererIndex, ToRenderLight(*it));
}

bool Scene::RemoveLight(uint64_t lightId)
{
  const auto it = std::ranges::find_if(m_EditorLights, [lightId](const SceneLight& light) { return light.id == lightId; });
  if (it == m_EditorLights.end())
    return false;

  const auto rendererIndex = static_cast<size_t>(std::distance(m_EditorLights.begin(), it));
  if (!RenderBackend::RemoveLight(rendererIndex)) return false;
  m_EditorLights.erase(it);
  return true;
}

uint64_t Scene::DuplicateEntity(uint64_t entityId)
{
  const SceneEntity* sourcePtr = FindEntity(entityId);
  if (!sourcePtr || sourcePtr->type == "controller") return 0;

  const SceneEntity source = *sourcePtr;
  const uint32_t instanceIndex = ModelManager::AddModelInstance(source.model, source.transform.GetTransform());
  if (instanceIndex == std::numeric_limits<uint32_t>::max()) return 0;

  SceneEntity duplicate = source;
  duplicate.id = m_NextEntityId++;
  duplicate.name += " Copy";
  duplicate.instanceIndex = instanceIndex;
  m_EditorEntities.push_back(std::move(duplicate));
  return m_EditorEntities.back().id;
}

uint64_t Scene::AddModelEntity(const std::string& modelName)
{
  const auto model = ModelManager::GetModel(modelName);
  if (!model || model->GetPhysXMeshType() == MeshType::CONVEXMESH ||
      model->GetPhysXMeshType() == MeshType::CONTROLLER)
    return 0;

  const glm::vec3 position = Camera::GetPosition() + Camera::GetForwardDirection() * 3.0f;
  const Transform transform(position, glm::vec3(0.0f), glm::vec3(1.0f));
  const uint32_t instanceIndex = ModelManager::AddModelInstance(modelName, transform.GetTransform());
  if (instanceIndex == std::numeric_limits<uint32_t>::max()) return 0;

  if (!model->m_IsRendered) ModelManager::SetRender(modelName, true);

  const auto existingCount = std::ranges::count_if(m_EditorEntities,
    [&modelName](const SceneEntity& entity) { return entity.model == modelName; });
  SceneEntity entity;
  entity.id = m_NextEntityId++;
  entity.name = existingCount == 0
    ? modelName
    : modelName + " #" + std::to_string(existingCount + 1);
  entity.model = modelName;
  entity.itemName = entity.name;
  entity.transform = transform;
  entity.instanceIndex = instanceIndex;
  m_EditorEntities.push_back(std::move(entity));
  return m_EditorEntities.back().id;
}

bool Scene::RemoveEntity(uint64_t entityId)
{
  const auto entityIt = std::ranges::find_if(m_EditorEntities,
    [entityId](const SceneEntity& entity) { return entity.id == entityId; });
  if (entityIt == m_EditorEntities.end()) return false;

  const std::string modelName = entityIt->model;
  const uint32_t removedInstance = entityIt->instanceIndex;
  if (entityIt->active)
  {
    if (!ModelManager::RemoveModelInstance(modelName, removedInstance)) return false;
    for (auto& entity : m_EditorEntities)
    {
      if (entity.id != entityId && entity.active && entity.model == modelName &&
          entity.instanceIndex > removedInstance)
        --entity.instanceIndex;
    }
  }

  if (m_FocusedEntityId == entityId) m_FocusedEntityId = 0;
  m_EditorEntities.erase(entityIt);
  return true;
}

bool Scene::UpdateEntityTransform(uint64_t entityId, const Transform& transform)
{
  SceneEntity* entity = FindEntity(entityId);
  if (!entity) return false;

  entity->transform = transform;
  if (entity->type == "controller")
  {
    ModelManager::SetControllerTransform(entity->model, transform);
  }
  else if (entity->instanceIndex == 0)
  {
    ModelManager::SetInitialModelTransform(entity->model, transform.GetTransform());
  }
  else
  {
    ModelManager::SetModelInstanceTransform(entity->model, entity->instanceIndex, transform.GetTransform());
  }
  return true;
}

bool Scene::UpdateControllerSize(uint64_t entityId, float radius, float height)
{
  SceneEntity* entity = FindEntity(entityId);
  if (!entity || entity->type != "controller") return false;

  radius = std::max(0.01f, radius);
  height = std::max(0.01f, height);
  if (!ModelManager::SetControllerSize(entity->model, radius, height)) return false;

  entity->controllerRadius = radius;
  entity->controllerHeight = height;
  return true;
}

void Scene::SyncEditorEntityTransforms()
{
  for (auto& entity : m_EditorEntities)
  {
    const auto model = ModelManager::GetModel(entity.model);
    if (!model || entity.instanceIndex >= model->m_InstanceTransforms.size()) continue;

    glm::quat orientation;
    glm::vec3 translation;
    glm::vec3 skew;
    glm::vec4 perspective;
    if (glm::vec3 scale; glm::decompose(model->m_InstanceTransforms[entity.instanceIndex], scale, orientation, translation, skew, perspective))
      entity.transform = Transform(translation, glm::degrees(glm::eulerAngles(orientation)), scale);
  }
}

bool Scene::GetPlayerPosition(glm::vec3& position) const
{
  const auto player = std::ranges::find_if(m_EditorEntities, [](const SceneEntity& entity) { return entity.active && entity.player; });
  if (player == m_EditorEntities.end()) return false;

  const auto model = ModelManager::GetModel(player->model);
  if (model && model->GetPhysXMeshType() == MeshType::CONTROLLER) position = model->GetControllerTransform().GetPosition();
  else position = player->transform.GetPosition();
  return true;
}

void Scene::UpdateInteractions()
{
  SyncEditorEntityTransforms();

  glm::vec3 playerPosition;
  if (!GetPlayerPosition(playerPosition))
  {
    m_FocusedEntityId = 0;
    return;
  }

  SceneEntity* focused = nullptr;
  float closestDistanceSquared = std::numeric_limits<float>::max();
  for (auto& entity : m_EditorEntities)
  {
    if (!entity.active || !entity.interactable || entity.player) continue;

    const glm::vec3 offset = entity.transform.GetPosition() - playerPosition;
    const float distanceSquared = glm::dot(offset, offset);
    if (distanceSquared <= entity.interactionRange * entity.interactionRange &&
        distanceSquared < closestDistanceSquared)
    {
      closestDistanceSquared = distanceSquared;
      focused = &entity;
    }
  }

  m_FocusedEntityId = focused ? focused->id : 0;
  const bool interactDown = Input::IsKeyPressed(Key::E) || Input::IsGamepadButtonPressed(Gamepad::X);
  const bool interactPressed = interactDown && !m_PreviousInteract;
  m_PreviousInteract = interactDown;
  if (!focused || !interactPressed) return;

  const std::string& displayName = focused->itemName.empty() ? focused->name : focused->itemName;
  gablog_log(LOG_INFO, __FILE__, __LINE__, "Interacted with '%s'", displayName.c_str());
  if (!focused->pickable)
  {
    if (!focused->dialogue.empty() && s_InteractionHandlers.startDialogue)
      s_InteractionHandlers.startDialogue(displayName, focused->dialogue);
    return;
  }

  if (!s_InteractionHandlers.tryPickUp || !s_InteractionHandlers.tryPickUp(*focused))
  {
    gablog_log(LOG_WARN, __FILE__, __LINE__, "Could not pick up '%s'", displayName.c_str());
    return;
  }

  const std::string modelName = focused->model;
  const uint32_t removedInstance = focused->instanceIndex;
  if (!ModelManager::RemoveModelInstance(modelName, removedInstance))
  {
    if (s_InteractionHandlers.rollbackPickUp) s_InteractionHandlers.rollbackPickUp(*focused);
    return;
  }

  focused->active = false;
  m_FocusedEntityId = 0;
  for (auto& entity : m_EditorEntities)
  {
    if (entity.active && entity.model == modelName && entity.instanceIndex > removedInstance)
      --entity.instanceIndex;
  }
  gablog_log(LOG_INFO, __FILE__, __LINE__, "Picked up '%s'", displayName.c_str());
}

bool Scene::SaveToJSON(const std::string& path) const
{
  std::ifstream input(path);
  if (!input)
  {
    gablog_log(LOG_ERROR, __FILE__, __LINE__, "Could not open scene file for reading: %s", path.c_str());
    return false;
  }

  json data;
  try
  {
    input >> data;
  }
  catch (const json::exception& exception)
  {
    gablog_log(LOG_ERROR, __FILE__, __LINE__, "Could not parse scene file '%s': %s", path.c_str(), exception.what());
    return false;
  }

  json entities = json::array();
  for (const auto& entity : m_EditorEntities)
  {
    const glm::vec3 position = entity.transform.GetPosition();
    const glm::vec3 rotation = entity.transform.GetRotation();
    const glm::vec3 scale = entity.transform.GetScale();

    json serialized = {
      {"name", entity.name},
      {"model", entity.model},
      {"position", {position.x, position.y, position.z}},
      {"rotation", {rotation.x, rotation.y, rotation.z}},
      {"scale", {scale.x, scale.y, scale.z}}
    };
    if (entity.type != "static") serialized["type"] = entity.type;
    if (entity.type == "controller")
    {
      serialized["controller_radius"] = entity.controllerRadius;
      serialized["controller_height"] = entity.controllerHeight;
    }
    if (entity.interactable) serialized["interactable"] = true;
    if (entity.pickable) serialized["pickable"] = true;
    if (entity.player) serialized["player"] = true;
    if (!entity.dialogue.empty()) serialized["dialogue"] = entity.dialogue;
    if (entity.interactable)
    {
      serialized["item_name"] = entity.itemName;
      serialized["interaction_range"] = entity.interactionRange;
      serialized["label_height"] = entity.labelHeight;
    }
    entities.push_back(std::move(serialized));
  }

  data["scenes"][m_Name]["entities"] = std::move(entities);

  const auto saveCullingScales = [](json& descriptions)
  {
    if (!descriptions.is_array()) return;
    for (auto& description : descriptions)
    {
      if (!description.is_object() || !description.contains("path")) continue;
      const std::string modelName = std::filesystem::path(description["path"].get<std::string>()).stem().string();
      const auto model = ModelManager::GetModel(modelName);
      if (model) description["culling_bounds_scale"] = model->GetCullingBoundsScale();
    }
  };
  auto& sceneData = data["scenes"][m_Name];
  if (sceneData.contains("static_models")) saveCullingScales(sceneData["static_models"]);
  if (sceneData.contains("animated_models")) saveCullingScales(sceneData["animated_models"]);

  json lights = json::array();
  for (const auto& light : m_EditorLights)
  {
    lights.push_back({
      {"name", light.name},
      {"type", LightTypeName(light.type)},
      {"color", {light.color.x, light.color.y, light.color.z}},
      {"position", {light.position.x, light.position.y, light.position.z}},
      {"rotation", {light.rotation.x, light.rotation.y, light.rotation.z}}
    });
  }
  data["scenes"][m_Name]["lights"] = std::move(lights);

  std::ofstream output(path, std::ios::trunc);
  if (!output)
  {
    gablog_log(LOG_ERROR, __FILE__, __LINE__, "Could not open scene file for writing: %s", path.c_str());
    return false;
  }
  output << std::setw(2) << data << '\n';
  return output.good();
}

void Scene::UpdateLoading()
{
  if(!m_Assets.loadingStarted || m_Assets.loadingDone) return;

  auto ready = [](auto& vec)
  {
    return std::all_of(vec.begin(),vec.end(),
    [](auto& f)
    {
        return f.wait_for(std::chrono::seconds(0))
            == std::future_status::ready;
    });
  };

  if(!ready(m_Assets.futureStatic) ||
     !ready(m_Assets.futureAnim) ||
     !ready(m_Assets.futureTextures))
      return;

  if(!m_Assets.uploadStarted)
  {
    m_Assets.uploadStarted = true;

    for(size_t i=0;i<m_Assets.static_models.size();i++)
    {
        auto model = m_Assets.futureStatic[i].get();
        model->SetCullingBoundsScale(m_Assets.static_models[i].cullingBoundsScale);
        ModelManager::BakeModel(
            m_Assets.static_models[i].path,
            model);
    }

    for(size_t i=0;i<m_Assets.animated_models.size();i++)
    {
        auto model = m_Assets.futureAnim[i].get();
        model->SetCullingBoundsScale(m_Assets.animated_models[i].cullingBoundsScale);
        ModelManager::BakeModel(
            m_Assets.animated_models[i].path,
            model);
    }

    auto skyboxTex = m_Assets.futureTextures[0].get();

    RenderSystem::UploadSkybox(skyboxTex);

    SpawnEntities();
    SpawnLights();

    ModelManager::UploadToGPU();
    RenderSystem::FinalizeModelUpload();

    m_Assets.futureStatic.clear();
    m_Assets.futureAnim.clear();
    m_Assets.futureTextures.clear();

    OnSceneStart();

    m_Assets.loadingDone = true;
    m_Assets.loadingStarted = false;
    m_Assets.uploadStarted = false;
  }
}

bool Scene::IsLoadingComplete() const
{
  return m_Assets.loadingDone;
}

void Scene::LoadSceneFromJSON(const std::string& path, const std::string& sceneName)
{
    std::ifstream file(path);
    json data;
    file >> data;

    auto& scene = data["scenes"][sceneName];

    if(scene.contains("sounds"))
        m_Assets.sounds = scene["sounds"].get<std::vector<std::string>>();

    if(scene.contains("music"))
        m_Assets.music = scene["music"].get<std::vector<std::string>>();

    if(scene.contains("skybox"))
        m_Assets.skybox = scene["skybox"].get<std::vector<std::string>>();

    if(scene.contains("static_models"))
    {
        for(auto& m : scene["static_models"])
        {
            SceneAssets::ModelDesc desc;

            desc.path = m["path"];
            desc.scale = m.value("scale",1.0f);
            desc.cullingBoundsScale = std::max(0.01f, m.value("culling_bounds_scale", 1.0f));
            desc.flag = false;

            if(std::string mesh = m.value("mesh","none"); mesh == "trianglemesh") desc.meshType = MeshType::TRIANGLEMESH;
            else if(mesh == "convex")  desc.meshType = MeshType::CONVEXMESH;
            else desc.meshType = MeshType::NONE;

            m_Assets.static_models.push_back(desc);
        }
    }

    if(scene.contains("animated_models"))
    {
        for(auto& m : scene["animated_models"])
        {
            SceneAssets::ModelDesc desc;

            desc.path = m["path"];
            desc.scale = m.value("scale",1.0f);
            desc.cullingBoundsScale = std::max(0.01f, m.value("culling_bounds_scale", 1.0f));
            desc.flag = false;
            desc.meshType = MeshType::CONTROLLER;

            m_Assets.animated_models.push_back(desc);
        }
    }

    if(scene.contains("entities"))
        m_Assets.entities = scene["entities"];

    if(scene.contains("lights"))
        m_Assets.lights = scene["lights"];
}

std::unique_ptr<Scene> SceneManager::s_ActiveScene = nullptr;
std::unique_ptr<Scene> SceneManager::s_PendingScene = nullptr;
bool SceneManager::s_Loading = false;
SceneManager::TransitionState SceneManager::s_TransitionState = SceneManager::TransitionState::None;
std::string SceneManager::s_RequestedScene;
float SceneManager::s_TransitionProgress = 0.0f;

namespace
{
  constexpr float SceneFadeDuration = 0.45f;

  float SmoothStep(float value)
  {
    value = std::clamp(value, 0.0f, 1.0f);
    return value * value * (3.0f - 2.0f * value);
  }
}

void SceneManager::LoadScene(const std::string& name)
{
  if (name.empty() || s_Loading || s_TransitionState != TransitionState::None) return;

  s_RequestedScene = name;
  s_TransitionProgress = 0.0f;

  if (s_ActiveScene)
  {
    s_TransitionState = TransitionState::FadingOut;
    return;
  }

  BeginLoadingScene(name);
}

void SceneManager::RegisterScene(const std::string& name, SceneFactory factory)
{
  if (name.empty() || !factory) return;
  s_SceneFactories.insert_or_assign(name, std::move(factory));
}

void SceneManager::SetInteractionHandlers(InteractionHandlers handlers)
{
  s_InteractionHandlers = std::move(handlers);
}

void SceneManager::BeginLoadingScene(const std::string& name)
{
  if (name.empty()) return;

  if (s_InteractionHandlers.reset) s_InteractionHandlers.reset();
  RenderSystem::ResetModelDrawCommands();
  ModelManager::Reset();
  RenderBackend::ClearLights();
  ParticleRenderer::Clear();
  AudioManager::StopAllSounds();
  AudioManager::StopAllMusic();

  if (const auto factory = s_SceneFactories.find(name); factory != s_SceneFactories.end())
    s_PendingScene = factory->second();
  else
    s_PendingScene = std::make_unique<GenericScene>(name);

  if (!s_PendingScene)
  {
    gablog_log(LOG_ERROR, __FILE__, __LINE__, "Scene factory for '%s' returned no scene", name.c_str());
    return;
  }

  s_PendingScene->StartLoading();
  s_Loading = true;
  s_TransitionState = TransitionState::Loading;
}

void SceneManager::Shutdown()
{
  if (s_InteractionHandlers.reset) s_InteractionHandlers.reset();
  s_Loading = false;
  s_TransitionState = TransitionState::None;
  s_RequestedScene.clear();
  s_TransitionProgress = 0.0f;
  s_PendingScene.reset();
  s_ActiveScene.reset();
}

void SceneManager::Update(DeltaTime& dt)
{
  const float frameTime = std::clamp(dt.GetSeconds(), 0.0f, 0.1f);

  if (s_TransitionState == TransitionState::FadingOut)
  {
    if (s_ActiveScene) s_ActiveScene->OnUpdate(dt);

    s_TransitionProgress = std::min(1.0f, s_TransitionProgress + frameTime / SceneFadeDuration);
    RenderSystem::DrawScreenOverlay(SmoothStep(s_TransitionProgress));

    if (s_TransitionProgress >= 1.0f)
    {
      const std::string requestedScene = std::move(s_RequestedScene);
      s_TransitionProgress = 0.0f;
      BeginLoadingScene(requestedScene);
    }
    return;
  }

  if (s_TransitionState == TransitionState::Loading)
  {
    s_PendingScene->UpdateLoading();

    RenderSystem::DrawLoadingScreen();

    if (s_PendingScene->IsLoadingComplete())
    {
      s_ActiveScene = std::move(s_PendingScene);
      s_Loading = false;
      s_TransitionState = TransitionState::FadingIn;
      s_TransitionProgress = 0.0f;
      s_RequestedScene.clear();
      RenderSystem::DrawScreenOverlay(1.0f);
    }

    return;
  }

  if (s_TransitionState == TransitionState::FadingIn)
  {
    if (s_ActiveScene) s_ActiveScene->OnUpdate(dt);

    s_TransitionProgress = std::min(1.0f, s_TransitionProgress + frameTime / SceneFadeDuration);
    RenderSystem::DrawScreenOverlay(1.0f - SmoothStep(s_TransitionProgress));
    if (s_TransitionProgress >= 1.0f)
    {
      s_TransitionState = TransitionState::None;
      s_TransitionProgress = 0.0f;
    }
    return;
  }

  if (s_ActiveScene) s_ActiveScene->OnUpdate(dt);
}

Scene* SceneManager::GetActiveScene()
{
  return s_ActiveScene.get();
}

bool SceneManager::IsLoading()
{
  return s_Loading;
}

std::vector<std::string> SceneManager::GetAvailableSceneNames()
{
  std::ifstream file(SceneFilePath);
  if (!file)
    return {};

  json data;
  try
  {
    file >> data;
  }
  catch (const json::exception&)
  {
    return {};
  }

  std::vector<std::string> names;
  if (data.contains("scenes") && data["scenes"].is_object())
  {
    names.reserve(data["scenes"].size());
    for (const auto& [name, scene] : data["scenes"].items())
      names.push_back(name);
  }
  std::sort(names.begin(), names.end());
  return names;
}

const std::vector<SceneEntity>& SceneManager::GetEntities()
{
  static const std::vector<SceneEntity> empty;
  return s_ActiveScene ? s_ActiveScene->GetEntities() : empty;
}

const std::vector<SceneLight>& SceneManager::GetLights()
{
  static const std::vector<SceneLight> empty;
  return s_ActiveScene ? s_ActiveScene->GetLights() : empty;
}

SceneEntity* SceneManager::FindEntity(uint64_t entityId)
{
  return s_ActiveScene ? s_ActiveScene->FindEntity(entityId) : nullptr;
}

SceneLight* SceneManager::FindLight(uint64_t lightId)
{
  return s_ActiveScene ? s_ActiveScene->FindLight(lightId) : nullptr;
}

uint64_t SceneManager::DuplicateEntity(uint64_t entityId)
{
  return s_ActiveScene ? s_ActiveScene->DuplicateEntity(entityId) : 0;
}

uint64_t SceneManager::AddModelEntity(const std::string& modelName)
{
  return s_ActiveScene ? s_ActiveScene->AddModelEntity(modelName) : 0;
}

bool SceneManager::RemoveEntity(uint64_t entityId)
{
  return s_ActiveScene && s_ActiveScene->RemoveEntity(entityId);
}

bool SceneManager::ImportExternalModel(const std::string& path, bool animated, float optimizerStrength,
  MeshType meshType, float cullingBoundsScale)
{
  if (!s_ActiveScene || path.empty()) return false;

  const std::filesystem::path modelPath(path);
  if (!std::filesystem::is_regular_file(modelPath))
  {
    gablog_log(LOG_ERROR, __FILE__, __LINE__, "External model does not exist: %s", path.c_str());
    return false;
  }

  const std::string modelName = modelPath.stem().string();
  if (modelName.empty()) return false;

  std::ifstream input(SceneFilePath);
  if (!input) return false;

  json data;
  try
  {
    input >> data;
  }
  catch (const json::exception& exception)
  {
    gablog_log(LOG_ERROR, __FILE__, __LINE__, "Could not parse scene file '%s': %s", SceneFilePath, exception.what());
    return false;
  }

  auto& scene = data["scenes"][s_ActiveScene->GetName()];
  const auto containsModelName = [&modelName](const json& descriptions)
  {
    if (!descriptions.is_array()) return false;
    return std::ranges::any_of(descriptions, [&modelName](const json& description)
    {
      if (!description.is_object() || !description.contains("path")) return false;
      return std::filesystem::path(description["path"].get<std::string>()).stem().string() == modelName;
    });
  };

  if (containsModelName(scene.value("static_models", json::array())) ||
      containsModelName(scene.value("animated_models", json::array())))
  {
    gablog_log(LOG_ERROR, __FILE__, __LINE__, "A model named '%s' is already registered in scene '%s'",
      modelName.c_str(), s_ActiveScene->GetName().c_str());
    return false;
  }

  json description = {
    {"path", modelPath.generic_string()},
    {"scale", std::max(0.0f, optimizerStrength)},
    {"culling_bounds_scale", std::max(0.01f, cullingBoundsScale)}
  };
  if (animated)
  {
    scene["animated_models"].push_back(std::move(description));
  }
  else
  {
    if (meshType == MeshType::TRIANGLEMESH) description["mesh"] = "trianglemesh";
    else if (meshType == MeshType::CONVEXMESH) description["mesh"] = "convex";
    scene["static_models"].push_back(std::move(description));
  }

  // A convex asset is a physics helper. Visual assets are also placed in front
  // of the editor camera so the result is visible immediately after reloading.
  if (meshType != MeshType::CONVEXMESH)
  {
    const glm::vec3 position = Camera::GetPosition() + Camera::GetForwardDirection() * 3.0f;
    json entity = {
      {"name", modelName},
      {"model", modelName},
      {"position", {position.x, position.y, position.z}},
      {"rotation", {0.0f, 0.0f, 0.0f}},
      {"scale", {1.0f, 1.0f, 1.0f}}
    };
    if (animated) entity["type"] = "controller";
    scene["entities"].push_back(std::move(entity));
  }

  std::ofstream output(SceneFilePath, std::ios::trunc);
  if (!output) return false;
  output << std::setw(2) << data << '\n';
  return output.good();
}

bool SceneManager::UpdateEntityTransform(uint64_t entityId, const Transform& transform)
{
  return s_ActiveScene && s_ActiveScene->UpdateEntityTransform(entityId, transform);
}

bool SceneManager::UpdateControllerSize(uint64_t entityId, float radius, float height)
{
  return s_ActiveScene && s_ActiveScene->UpdateControllerSize(entityId, radius, height);
}

uint64_t SceneManager::AddLight(LightType type)
{
  return s_ActiveScene ? s_ActiveScene->AddLight(type) : 0;
}

bool SceneManager::UpdateLight(uint64_t lightId, const std::string& name, const glm::vec3& color,
  const glm::vec3& position, const glm::vec3& rotation)
{
  return s_ActiveScene && s_ActiveScene->UpdateLight(lightId, name, color, position, rotation);
}

bool SceneManager::RemoveLight(uint64_t lightId)
{
  return s_ActiveScene && s_ActiveScene->RemoveLight(lightId);
}

void SceneManager::SyncEditorEntityTransforms()
{
  if (s_ActiveScene) s_ActiveScene->SyncEditorEntityTransforms();
}

bool SceneManager::GetPlayerPosition(glm::vec3& position)
{
  return s_ActiveScene && s_ActiveScene->GetPlayerPosition(position);
}

uint64_t SceneManager::GetFocusedEntityID()
{
  return s_ActiveScene ? s_ActiveScene->GetFocusedEntityID() : 0;
}

bool SceneManager::SaveActiveScene()
{
  return s_ActiveScene && s_ActiveScene->SaveToJSON(SceneFilePath);
}

std::string SceneManager::GetActiveSceneName()
{
  return s_ActiveScene ? s_ActiveScene->GetName() : std::string();
}
