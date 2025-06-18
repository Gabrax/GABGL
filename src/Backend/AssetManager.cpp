#include "AssetManager.h"

#include <future>
#include "../backend/Audio.h"
#include "../backend/Texture.h"
#include "../backend/Renderer.h"
#include "../backend/Model.h"
#include "../engine.h"
#include <GLFW/glfw3.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <atomic>
#include <filesystem>

class UploadContextManager {
public:
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

UploadContextManager::UploadContextManager(GLFWwindow* sharedContext) {
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    m_UploadWindow = glfwCreateWindow(1, 1, "", nullptr, sharedContext);
    if (!m_UploadWindow) {
        std::cerr << "Failed to create upload OpenGL context window" << std::endl;
        throw std::runtime_error("Failed to create upload context");
    }

    m_Thread = std::thread(&UploadContextManager::ThreadFunc, this);
}

UploadContextManager::~UploadContextManager() {
    Shutdown();
    if (m_Thread.joinable())
        m_Thread.join();
}

void UploadContextManager::EnqueueUpload(Task&& task) {
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_TaskQueue.push(std::move(task));
        m_Idle = false;
    }
    m_CondVar.notify_one();
}

void UploadContextManager::WaitIdle() {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_CondVar.wait(lock, [this]() { return m_TaskQueue.empty() && m_Idle.load(); });
}

void UploadContextManager::Shutdown() {
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Stop = true;
    }
    m_CondVar.notify_one();
}

void UploadContextManager::ThreadFunc() {
    glfwMakeContextCurrent(m_UploadWindow);

    while (true) {
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



void UploadContextManager::ScheduleOnMainThread(std::function<void()> task) {
    std::lock_guard<std::mutex> lock(mainThreadQueueMutex);
    mainThreadTasks.push(std::move(task));
}

// Call this on main thread regularly (e.g. each frame)
void UploadContextManager::ProcessMainThreadTasks() {
    std::lock_guard<std::mutex> lock(mainThreadQueueMutex);
    while (!mainThreadTasks.empty()) {
        mainThreadTasks.front()();
        mainThreadTasks.pop();
    }
}

void AssetManager::LoadAssets()
{
    std::vector<const char*> sounds = {
        "res/audio/select1.wav",
        "res/audio/select2.wav",
    };

    std::vector<const char*> music = {
        "res/audio/menu.wav",
    };

    std::vector<std::string> skybox = {
        "res/skybox/NightSky_Right.png",
        "res/skybox/NightSky_Left.png",
        "res/skybox/NightSky_Top.png",
        "res/skybox/NightSky_Bottom.png",
        "res/skybox/NightSky_Front.png",
        "res/skybox/NightSky_Back.png"
    };

    std::vector<std::tuple<const char*, float, bool, PhysXMeshType>> static_models = {
        { "res/map/objHouse.obj", 1.0f, true, PhysXMeshType::TRIANGLEMESH },
    };

    std::vector<std::tuple<const char*, float, bool, PhysXMeshType>> animated_models = {
        { "res/lowpoly/MaleSurvivor1.glb", 1.0f, true, PhysXMeshType::TRIANGLEMESH },
        /*{ "res/guy/guy.dae", 1.0f, true, PhysXMeshType::TRIANGLEMESH }*/
    };

    std::vector<std::future<void>> m_FutureVoid;
    std::vector<std::future<std::shared_ptr<Texture>>> m_FutureTextures;
    std::vector<std::future<std::shared_ptr<Model>>> m_FutureStaticModels;
    std::vector<std::future<std::shared_ptr<Model>>> m_FutureAnimModels;

    for (auto& sound : sounds)
        m_FutureVoid.push_back(std::async(std::launch::async, AudioSystem::LoadSound, sound));
    for (auto& track : music)
        m_FutureVoid.push_back(std::async(std::launch::async, AudioSystem::LoadMusic, track));

    auto futureSkybox = std::async(std::launch::async, [skybox]() { return Texture::CreateRAWCUBEMAP(skybox); });
    m_FutureTextures.push_back(std::move(futureSkybox));
    auto skyboxTexture = m_FutureTextures.back().get();

    for (auto& [path, scale, flag, meshType] : static_models)
        m_FutureStaticModels.push_back(std::async(std::launch::async, Model::CreateSTATIC, path, scale, flag, meshType));

    for (auto& [path, scale, flag, meshType] : animated_models)
        m_FutureAnimModels.push_back(std::async(std::launch::async, Model::CreateANIMATED, path, scale, flag, meshType));

    UploadContextManager uploadManager(Engine::GetInstance().GetMainWindow().GetWindowPtr());

    uploadManager.EnqueueUpload([skyboxTexture]() {
        Renderer::BakeSkyboxTextures("night", skyboxTexture);
    });

    for (size_t i = 0; i < static_models.size(); ++i) {
        auto model = m_FutureStaticModels[i].get();
        const std::string& path = std::get<0>(static_models[i]);
        std::string name = std::filesystem::path(path).stem().string();

        uploadManager.EnqueueUpload([&uploadManager, path, model, name]() {
            Renderer::BakeModelTextures(path, model);

            uploadManager.ScheduleOnMainThread([name]() {
              Renderer::BakeModelBuffers(name);
            });
        });
    }

    for (size_t i = 0; i < animated_models.size(); ++i) {
        auto model = m_FutureAnimModels[i].get();
        const std::string& path = std::get<0>(animated_models[i]);
        std::string name = std::filesystem::path(path).stem().string();

        uploadManager.EnqueueUpload([&uploadManager, path, model, name]() {
            Renderer::BakeModelTextures(path, model);

            uploadManager.ScheduleOnMainThread([name]() {
                Renderer::BakeModelBuffers(name);
            });
        });
    }

    uploadManager.WaitIdle();
    uploadManager.ProcessMainThreadTasks();
    uploadManager.Shutdown();
}





