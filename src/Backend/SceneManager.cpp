#include "SceneManager.h"


#include "Renderer.h"
#include "AudioManager.h"
#include "Camera.h"
#include "LightManager.h"
#include "Logger.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <limits>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Scenes.hpp"

namespace
{
  constexpr const char* SceneFilePath = "../res/scenes/scene.json";

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

  struct GenericScene : Scene
  {
    explicit GenericScene(const std::string& name) : Scene(name) {}
    void OnSceneStart() override {}
    void OnUpdate(DeltaTime& dt) override
    {
      Renderer::DrawScene(dt, []() {});
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
		case Key::R:
		{
      Renderer::SetFullscreen("select1", true);
			break;
		}
		case Key::T:
		{
      Renderer::SetFullscreen("select2", false);
			break;
		}
    case Key::Tab:
    {
      Renderer::SwitchRenderState();
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

  for(auto& model : m_Assets.static_models)
  {
    m_Assets.futureStatic.push_back(
        std::async(std::launch::async,
            Model::CreateSTATIC,
            model.path.c_str(),
            model.scale,
            model.flag,
            model.meshType));
  }

  for(auto& model : m_Assets.animated_models)
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
        GABGL_WARN("Invalid transform for model '{}': position, rotation and scale must contain 3 values", modelName);
        return;
      }

      Transform transform(
        {position[0], position[1], position[2]},
        {rotation[0], rotation[1], rotation[2]},
        {scale[0], scale[1], scale[2]});

      if (type == "controller")
      {
        ModelManager::SetInitialControllerTransform(modelName, transform, 1.0f, 1.0f, true);
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

      const std::string baseName = transformDesc.value("name", e.value("name", modelName));
      entity.name = ordinal == 0 ? baseName : baseName + " #" + std::to_string(ordinal + 1);
      ++ordinal;
      m_EditorEntities.push_back(std::move(entity));
    };

    if (e.contains("instances"))
    {
      for (const auto& instance : e["instances"])
        spawn(instance);
    }
    else
    {
      spawn(e);
    }
  }

  for (const auto& modelName : ModelManager::GetModelNames())
  {
    const auto model = ModelManager::GetModel(modelName);
    if (!model || model->GetPhysXMeshType() == MeshType::CONVEXMESH || !model->m_InstanceTransforms.empty())
      continue;

    Transform transform;
    const bool isController = model->GetPhysXMeshType() == MeshType::CONTROLLER;
    if (isController)
      ModelManager::SetInitialControllerTransform(modelName, transform, 1.0f, 1.0f, true);
    else
      ModelManager::SetInitialModelTransform(modelName, transform.GetTransform());

    SceneEntity entity;
    entity.id = m_NextEntityId++;
    entity.name = modelName;
    entity.model = modelName;
    entity.type = isController ? "controller" : "static";
    entity.transform = transform;
    entity.instanceIndex = 0;
    m_EditorEntities.push_back(std::move(entity));
  }
}

void Scene::SpawnLights()
{
  m_EditorLights.clear();
  m_NextLightId = 1;

  if (!m_Assets.lights.is_array())
    return;

  for (const auto& description : m_Assets.lights)
  {
    if (!description.is_object())
      continue;

    SceneLight light;
    light.type = ParseLightType(description.value("type", "point"));
    if (light.type == LightType::DIRECT &&
        std::any_of(m_EditorLights.begin(), m_EditorLights.end(),
          [](const SceneLight& existing) { return existing.type == LightType::DIRECT; }))
    {
      GABGL_WARN("Scene '{}' contains more than one directional light; ignoring the duplicate", m_Name);
      continue;
    }

    const glm::vec3 defaultRotation = light.type == LightType::DIRECT
      ? glm::vec3(-1.0f, -2.0f, -1.0f)
      : glm::vec3(0.0f, -1.0f, 0.0f);
    light.rotation = defaultRotation;
    if (description.contains("color") && !ReadVec3(description["color"], light.color))
      GABGL_WARN("Invalid color for a light in scene '{}'; using white", m_Name);
    if (description.contains("position") && !ReadVec3(description["position"], light.position))
      GABGL_WARN("Invalid position for a light in scene '{}'; using origin", m_Name);
    if (description.contains("rotation") && !ReadVec3(description["rotation"], light.rotation))
      GABGL_WARN("Invalid rotation for a light in scene '{}'; using the default", m_Name);

    light.id = m_NextLightId++;
    light.name = description.value("name", std::string(LightTypeName(light.type)) + " light " + std::to_string(light.id));
    LightManager::AddLight(light.type, light.color, light.position, light.rotation);
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
  const auto it = std::find_if(m_EditorLights.begin(), m_EditorLights.end(),
    [lightId](const SceneLight& light) { return light.id == lightId; });
  return it == m_EditorLights.end() ? nullptr : &(*it);
}

uint64_t Scene::AddLight(LightType type)
{
  if (type == LightType::DIRECT &&
      std::any_of(m_EditorLights.begin(), m_EditorLights.end(),
        [](const SceneLight& light) { return light.type == LightType::DIRECT; }))
    return 0;

  SceneLight light;
  light.id = m_NextLightId++;
  light.type = type;
  light.name = std::string(LightTypeName(type)) + " light " + std::to_string(light.id);
  light.position = Camera::GetPosition();
  light.rotation = type == LightType::DIRECT
    ? glm::vec3(-1.0f, -2.0f, -1.0f)
    : Camera::GetForwardDirection();

  LightManager::AddLight(light.type, light.color, light.position, light.rotation);
  m_EditorLights.push_back(std::move(light));
  return m_EditorLights.back().id;
}

bool Scene::UpdateLight(uint64_t lightId, const std::string& name, const glm::vec3& color,
  const glm::vec3& position, const glm::vec3& rotation)
{
  const auto it = std::find_if(m_EditorLights.begin(), m_EditorLights.end(),
    [lightId](const SceneLight& light) { return light.id == lightId; });
  if (it == m_EditorLights.end())
    return false;

  it->name = name;
  it->color = glm::max(color, glm::vec3(0.0f));
  it->position = position;
  it->rotation = rotation;
  const int32_t managerIndex = static_cast<int32_t>(std::distance(m_EditorLights.begin(), it));
  LightManager::EditLight(managerIndex, it->color, it->position, it->rotation);
  return true;
}

bool Scene::RemoveLight(uint64_t lightId)
{
  const auto it = std::find_if(m_EditorLights.begin(), m_EditorLights.end(),
    [lightId](const SceneLight& light) { return light.id == lightId; });
  if (it == m_EditorLights.end())
    return false;

  const int32_t managerIndex = static_cast<int32_t>(std::distance(m_EditorLights.begin(), it));
  LightManager::RemoveLight(managerIndex);
  m_EditorLights.erase(it);
  return true;
}

uint64_t Scene::DuplicateEntity(uint64_t entityId)
{
  const SceneEntity* sourcePtr = FindEntity(entityId);
  if (!sourcePtr || sourcePtr->type == "controller")
    return 0;

  const SceneEntity source = *sourcePtr;
  const uint32_t instanceIndex = ModelManager::AddModelInstance(source.model, source.transform.GetTransform());
  if (instanceIndex == std::numeric_limits<uint32_t>::max())
    return 0;

  SceneEntity duplicate = source;
  duplicate.id = m_NextEntityId++;
  duplicate.name += " Copy";
  duplicate.instanceIndex = instanceIndex;
  m_EditorEntities.push_back(std::move(duplicate));
  return m_EditorEntities.back().id;
}

bool Scene::UpdateEntityTransform(uint64_t entityId, const Transform& transform)
{
  SceneEntity* entity = FindEntity(entityId);
  if (!entity)
    return false;

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

void Scene::SyncEditorEntityTransforms()
{
  for (auto& entity : m_EditorEntities)
  {
    const auto model = ModelManager::GetModel(entity.model);
    if (!model || entity.instanceIndex >= model->m_InstanceTransforms.size())
      continue;

    glm::vec3 scale;
    glm::quat orientation;
    glm::vec3 translation;
    glm::vec3 skew;
    glm::vec4 perspective;
    if (glm::decompose(model->m_InstanceTransforms[entity.instanceIndex], scale, orientation, translation, skew, perspective))
      entity.transform = Transform(translation, glm::degrees(glm::eulerAngles(orientation)), scale);
  }
}

bool Scene::SaveToJSON(const std::string& path) const
{
  std::ifstream input(path);
  if (!input)
  {
    GABGL_ERROR("Could not open scene file for reading: {}", path);
    return false;
  }

  json data;
  try
  {
    input >> data;
  }
  catch (const json::exception& exception)
  {
    GABGL_ERROR("Could not parse scene file '{}': {}", path, exception.what());
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
    if (entity.type != "static")
      serialized["type"] = entity.type;
    entities.push_back(std::move(serialized));
  }

  data["scenes"][m_Name]["entities"] = std::move(entities);

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
    GABGL_ERROR("Could not open scene file for writing: {}", path);
    return false;
  }
  output << std::setw(2) << data << '\n';
  return output.good();
}

void Scene::UpdateLoading()
{
  if(!m_Assets.loadingStarted || m_Assets.loadingDone)
      return;

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
        ModelManager::BakeModel(
            m_Assets.static_models[i].path,
            m_Assets.futureStatic[i].get());
    }

    for(size_t i=0;i<m_Assets.animated_models.size();i++)
    {
        ModelManager::BakeModel(
            m_Assets.animated_models[i].path,
            m_Assets.futureAnim[i].get());
    }

    auto skyboxTex = m_Assets.futureTextures[0].get();

    Renderer::BakeSkyboxTextures("night",skyboxTex);

    SpawnEntities();
    SpawnLights();

    ModelManager::UploadToGPU();
    Renderer::InitDrawCommandBuffer();

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
            desc.flag = false;

            std::string mesh = m.value("mesh","none");

            if(mesh == "trianglemesh") desc.meshType = MeshType::TRIANGLEMESH;
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

void SceneManager::LoadScene(const std::string& name)
{
  if (name.empty() || s_Loading)
    return;

  Renderer::ResetModelDrawCommands();
  ModelManager::Reset();
  LightManager::Clear();

  if(name == "game") s_PendingScene = std::make_unique<GameScene>();
  else if(name == "menu") s_PendingScene = std::make_unique<MenuScene>();
  else s_PendingScene = std::make_unique<GenericScene>(name);

  s_PendingScene->StartLoading();
  s_Loading = true;
}

void SceneManager::Update(DeltaTime& dt)
{
  if (s_Loading)
  {
    s_PendingScene->UpdateLoading();

    Renderer::DrawLoadingScreen();

    if (s_PendingScene->IsLoadingComplete())
    {
      s_ActiveScene = std::move(s_PendingScene);
      s_Loading = false;
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

bool SceneManager::UpdateEntityTransform(uint64_t entityId, const Transform& transform)
{
  return s_ActiveScene && s_ActiveScene->UpdateEntityTransform(entityId, transform);
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
  if (s_ActiveScene)
    s_ActiveScene->SyncEditorEntityTransforms();
}

bool SceneManager::SaveActiveScene()
{
  return s_ActiveScene && s_ActiveScene->SaveToJSON(SceneFilePath);
}

std::string SceneManager::GetActiveSceneName()
{
  return s_ActiveScene ? s_ActiveScene->GetName() : std::string();
}

