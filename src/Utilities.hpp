#pragma once
#include "LoadShader.h"
#include "Window.h"
#include "raudio.h"
#include <iostream>
#include <chrono>
#include <functional>
#include <thread>
#include <atomic>

namespace Utilities {

  struct Shaders 
  {
    Shader model;
    Shader skybox;
    Shader animated;
    Shader light;
    Shader mainFB;
    Shader bloom_downsample;
    Shader bloom_upsample;
    Shader bloom_final;
  };
  inline Shaders g_shaders;

  struct Sounds
  {
    Sound hotload;
    Sound fullscreen;
    Sound switchmode;
  };
  inline Sounds g_sounds;

  inline void BakeShaders()
  {
    g_shaders.model.Load("res/shaders/model.glsl");
    g_shaders.animated.Load("res/shaders/anim_model.glsl");
    g_shaders.skybox.Load("res/shaders/skybox.glsl");
    g_shaders.light.Load("res/shaders/light.glsl");
    g_shaders.mainFB.Load("res/shaders/mainFB.glsl");
    g_shaders.bloom_downsample.Load("res/shaders/bloom_downsample.glsl");
    g_shaders.bloom_upsample.Load("res/shaders/bloom_upsample.glsl");
    g_shaders.bloom_final.Load("res/shaders/bloom_final.glsl");
  }

  inline void HotReloadShaders()
  {
    std::cout << "HotReloading shaders...\n";
    BakeShaders();
    PlaySound(g_sounds.hotload);
  }

  inline void LoadSounds()
  {
    g_sounds.hotload = LoadSound("res/audio/select3.wav");
    SetSoundVolume(g_sounds.hotload, 0.5f);
    g_sounds.fullscreen = LoadSound("res/audio/select1.wav");
    SetSoundVolume(g_sounds.fullscreen, 0.5f);
    g_sounds.switchmode = LoadSound("res/audio/select2.wav");
    SetSoundVolume(g_sounds.switchmode, 0.5f);
  }

  inline void measureExecutionTime(const std::atomic<bool>& initializationComplete, const std::chrono::high_resolution_clock::time_point& start)
  {
      while (!initializationComplete) {
          auto current = std::chrono::high_resolution_clock::now();
          std::chrono::duration<double> elapsed = current - start;
          std::cout << "\rElapsed time: " << elapsed.count() << " seconds" << std::flush;
          std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
      }

      auto end = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> elapsed = end - start;
      std::cout << "\rTotal time taken: " << elapsed.count() << " seconds" << '\n';
  }

  template <typename Func>
  inline void displayTimer(Func&& func)
  {

      auto start = std::chrono::high_resolution_clock::now();
      std::atomic<bool> initializationComplete = false;

      std::thread timerThread(measureExecutionTime, std::ref(initializationComplete), start);

      func();

      initializationComplete = true;

      timerThread.join();
  }

  struct Timer
  {
      Timer()
      {
        Reset();
      }

      void Reset()
      {
        m_Start = std::chrono::high_resolution_clock::now();
      }

      float Elapsed()
      {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count() * 0.001f * 0.001f * 0.001f;
      }

      float ElapsedMillis()
      {
        return Elapsed() * 1000.0f;
      }

    private:
      std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
  };
}
