#include "AssetManager.h"

#include <future>
#include "../backend/AudioManager.h"
#include "../backend/Texture.h"
#include "../backend/Renderer.h"
#include "../backend/ModelManager.h"
#include <GLFW/glfw3.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <atomic>

struct UploadContextManager
{
  using Task = std::function<void()>;

  UploadContextManager(GLFWwindow* sharedContext = nullptr);
  ~UploadContextManager();

  // Add an upload task to the queue
  void EnqueueUpload(Task&& task);

  // Wait until all queued tasks are done
  void WaitIdle();
  void ScheduleOnMainThread(std::function<void()> task);
  void ProcessMainThreadTasks();
  // Stop the upload thread and destroy context
  void Shutdown();

private:
  void ThreadFunc();

  GLFWwindow* m_UploadWindow = nullptr;
  std::thread m_Thread;
  std::mutex m_Mutex;
  std::condition_variable m_CondVar;
  std::queue<Task> m_TaskQueue;
  std::atomic<bool> m_Stop{ false };
  std::atomic<bool> m_Idle{ true };
  std::mutex mainThreadQueueMutex;
  std::queue<std::function<void()>> mainThreadTasks;
};

UploadContextManager::UploadContextManager(GLFWwindow* sharedContext)
{
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  m_UploadWindow = glfwCreateWindow(1, 1, "", nullptr, sharedContext);
  if (!m_UploadWindow) {
      throw std::runtime_error("Failed to create upload context");
  }

  m_Thread = std::thread(&UploadContextManager::ThreadFunc, this);
}

UploadContextManager::~UploadContextManager()
{
  Shutdown();
  if (m_Thread.joinable())
      m_Thread.join();
}

void UploadContextManager::EnqueueUpload(Task&& task)
{
  {
      std::lock_guard<std::mutex> lock(m_Mutex);
      m_TaskQueue.push(std::move(task));
      m_Idle = false;
  }
  m_CondVar.notify_one();
}

void UploadContextManager::WaitIdle()
{
  std::unique_lock<std::mutex> lock(m_Mutex);
  m_CondVar.wait(lock, [this]() { return m_TaskQueue.empty() && m_Idle.load(); });
}

void UploadContextManager::Shutdown()
{
  {
      std::lock_guard<std::mutex> lock(m_Mutex);
      m_Stop = true;
  }
  m_CondVar.notify_one();
}

void UploadContextManager::ThreadFunc()
{
  glfwMakeContextCurrent(m_UploadWindow);

  while (true)
  {
    Task task;

    {
        std::unique_lock<std::mutex> lock(m_Mutex);
        m_CondVar.wait(lock, [this]() { return !m_TaskQueue.empty() || m_Stop; });

        if (m_Stop && m_TaskQueue.empty())
            break;

        task = std::move(m_TaskQueue.front());
        m_TaskQueue.pop();
    }

    if (task) {
        task();
        glFlush();
    }

    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (m_TaskQueue.empty())
            m_Idle = true;
    }
    m_CondVar.notify_all();
  }

  glFinish();
  glfwMakeContextCurrent(nullptr);
  glfwDestroyWindow(m_UploadWindow);
  m_UploadWindow = nullptr;
}

void UploadContextManager::ScheduleOnMainThread(std::function<void()> task)
{
  std::lock_guard<std::mutex> lock(mainThreadQueueMutex);
  mainThreadTasks.push(std::move(task));
}

void UploadContextManager::ProcessMainThreadTasks()
{
  std::lock_guard<std::mutex> lock(mainThreadQueueMutex);
  while (!mainThreadTasks.empty()) {
      mainThreadTasks.front()();
      mainThreadTasks.pop();
  }
}

struct AssetsData
{
  std::vector<const char*> sounds =
  {
    "res/audio/select1.wav",
    "res/audio/select2.wav",
  };

  std::vector<const char*> music =
  {
    "res/audio/menu.wav",
    "res/audio/night.wav",
  };

  std::vector<std::string> skybox =
  {
    "res/textures/NightSky_Right.png",
    "res/textures/NightSky_Left.png",
    "res/textures/NightSky_Top.png",
    "res/textures/NightSky_Bottom.png",
    "res/textures/NightSky_Front.png",
    "res/textures/NightSky_Back.png"
  };

  std::vector<std::tuple<const char*, float, bool, MeshType>> static_models =
  {
    { "res/map/objHouse.obj", 0.7f, false, MeshType::TRIANGLEMESH },
    { "res/models/aidkit.glb", 1.0f, false, MeshType::NONE },
    { "res/models/aidkit_convex.glb", 1.0f, false, MeshType::CONVEXMESH },
    { "res/models/pistolammo.glb", 1.0f, false, MeshType::NONE },
    { "res/models/pistolammo_convex.glb", 1.0f, false, MeshType::CONVEXMESH },
    { "res/models/shotgunammo.glb", 1.0f, false, MeshType::NONE },
    { "res/models/shotgunammo_convex.glb", 1.0f, false, MeshType::CONVEXMESH },
    { "res/models/pistol.glb", 1.0f, false, MeshType::NONE },
    { "res/models/pistol_convex.glb", 1.0f, false, MeshType::CONVEXMESH },
    { "res/models/shotgun.glb", 1.0f, false, MeshType::NONE },
    { "res/models/shotgun_convex.glb", 1.0f, false, MeshType::CONVEXMESH },
    { "res/models/sphere.glb", 1.0f, false, MeshType::NONE },
    { "res/models/outer_terrain.glb", 1.0f, false, MeshType::NONE },
  };

  std::vector<std::tuple<const char*, float, bool, MeshType>> animated_models =
  {
    { "res/zombie/zombie.glb", 0.5f, false, MeshType::CONTROLLER },
    { "res/models/harry.glb", 1.0f, false, MeshType::CONTROLLER },
  };

  std::vector<std::future<void>> m_FutureVoid;
  std::vector<std::future<std::shared_ptr<Texture>>> m_FutureTextures;
  std::vector<std::future<std::shared_ptr<Model>>> m_FutureStaticModels;
  std::vector<std::future<std::shared_ptr<Model>>> m_FutureAnimModels;

  bool m_UploadStarted = false;
  bool m_LoadingStarted = false;
  bool m_LoadingDone = false;

} s_Data;

void AssetManager::Init()
{
  if (s_Data.m_LoadingStarted) return; // Already started

  s_Data.m_LoadingStarted = true;
  s_Data.m_LoadingDone = false;
  s_Data.m_UploadStarted = false;

  for (auto& sound : s_Data.sounds) AudioManager::LoadSound(sound);

  for (auto& track : s_Data.music) AudioManager::LoadMusic(track);

  for (auto& [path, scale, flag, meshType] : s_Data.static_models)
      s_Data.m_FutureStaticModels.push_back(std::async(std::launch::async, Model::CreateSTATIC, path, scale, flag, meshType));

  for (auto& [path, scale, flag, meshType] : s_Data.animated_models)
      s_Data.m_FutureAnimModels.push_back(std::async(std::launch::async, Model::CreateANIMATED, path, scale, flag, meshType));

  s_Data.m_FutureTextures.push_back(
      std::async(std::launch::async, [skybox = s_Data.skybox]() {
          return Texture::CreateRAWCUBEMAP(skybox);
      })
  );
}

void AssetManager::Update()
{
  if (!s_Data.m_LoadingStarted || s_Data.m_LoadingDone)
      return;

  bool allReady = std::all_of(s_Data.m_FutureVoid.begin(), s_Data.m_FutureVoid.end(),
      [](auto& f) { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }) &&
      std::all_of(s_Data.m_FutureTextures.begin(), s_Data.m_FutureTextures.end(),
      [](auto& f) { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }) &&
      std::all_of(s_Data.m_FutureStaticModels.begin(), s_Data.m_FutureStaticModels.end(),
      [](auto& f) { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }) &&
      std::all_of(s_Data.m_FutureAnimModels.begin(), s_Data.m_FutureAnimModels.end(),
      [](auto& f) { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; });

  if (!allReady)
      return;

  if (!s_Data.m_UploadStarted)
  {
    s_Data.m_UploadStarted = true;

    for (size_t i = 0; i < s_Data.static_models.size(); ++i)
    {
      ModelManager::BakeModel(std::get<0>(s_Data.static_models[i]), s_Data.m_FutureStaticModels[i].get());
    }

    for (size_t i = 0; i < s_Data.animated_models.size(); ++i)
    {
      ModelManager::BakeModel(std::get<0>(s_Data.animated_models[i]), s_Data.m_FutureAnimModels[i].get());
    }

    auto skyboxTexture = s_Data.m_FutureTextures[0].get();
    Renderer::BakeSkyboxTextures("night", skyboxTexture);

    s_Data.m_FutureVoid.clear();
    s_Data.m_FutureTextures.clear();
    s_Data.m_FutureStaticModels.clear();
    s_Data.m_FutureAnimModels.clear();

    ModelManager::UploadToGPU();
    Renderer::InitDrawCommandBuffer();

    s_Data.m_LoadingDone = true;
    s_Data.m_LoadingStarted = false;
    s_Data.m_UploadStarted = false;
  }
}

bool AssetManager::LoadingComplete()
{
    return s_Data.m_LoadingDone;
}


