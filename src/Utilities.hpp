#pragma once
#include "LoadShader.h"
#include "Window.h"
#include "raudio.h"
#include <iostream>
#include <chrono>
#include <functional>
#include <thread>
#include <atomic>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#include "PxPhysicsAPI.h"


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

  inline 	bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale)
	{
		// From glm::decompose in matrix_decompose.inl

		using namespace glm;
		using T = float;

		mat4 LocalMatrix(transform);

		// Normalize the matrix.
		if (epsilonEqual(LocalMatrix[3][3], static_cast<float>(0), epsilon<T>()))
			return false;

		// First, isolate perspective.  This is the messiest.
		if (
			epsilonNotEqual(LocalMatrix[0][3], static_cast<T>(0), epsilon<T>()) ||
			epsilonNotEqual(LocalMatrix[1][3], static_cast<T>(0), epsilon<T>()) ||
			epsilonNotEqual(LocalMatrix[2][3], static_cast<T>(0), epsilon<T>()))
		{
			// Clear the perspective partition
			LocalMatrix[0][3] = LocalMatrix[1][3] = LocalMatrix[2][3] = static_cast<T>(0);
			LocalMatrix[3][3] = static_cast<T>(1);
		}

		// Next take care of translation (easy).
		translation = vec3(LocalMatrix[3]);
		LocalMatrix[3] = vec4(0, 0, 0, LocalMatrix[3].w);

		vec3 Row[3], Pdum3;

		// Now get scale and shear.
		for (length_t i = 0; i < 3; ++i)
			for (length_t j = 0; j < 3; ++j)
				Row[i][j] = LocalMatrix[i][j];

		// Compute X scale factor and normalize first row.
		scale.x = length(Row[0]);
		Row[0] = detail::scale(Row[0], static_cast<T>(1));
		scale.y = length(Row[1]);
		Row[1] = detail::scale(Row[1], static_cast<T>(1));
		scale.z = length(Row[2]);
		Row[2] = detail::scale(Row[2], static_cast<T>(1));

		// At this point, the matrix (in rows[]) is orthonormal.
		// Check for a coordinate system flip.  If the determinant
		// is -1, then negate the matrix and the scaling factors.
#if 0
		Pdum3 = cross(Row[1], Row[2]); // v3Cross(row[1], row[2], Pdum3);
		if (dot(Row[0], Pdum3) < 0)
		{
			for (length_t i = 0; i < 3; i++)
			{
				scale[i] *= static_cast<T>(-1);
				Row[i] *= static_cast<T>(-1);
			}
		}
#endif

		rotation.y = asin(-Row[0][2]);
		if (cos(rotation.y) != 0) {
			rotation.x = atan2(Row[1][2], Row[2][2]);
			rotation.z = atan2(Row[0][1], Row[0][0]);
		}
		else {
			rotation.x = atan2(-Row[2][0], Row[1][1]);
			rotation.z = 0;
		}


		return true;
	}

  inline glm::mat4 PxMat44ToGlmMat4(physx::PxMat44 pxMatrix) {
    glm::mat4 matrix;
    for (int x = 0; x < 4; x++)
        for (int y = 0; y < 4; y++)
            matrix[x][y] = pxMatrix[x][y];
    return matrix;
  }

  inline physx::PxMat44 GlmMat4ToPxMat44(glm::mat4 glmMatrix) {
      physx::PxMat44 matrix;
      for (int x = 0; x < 4; x++)
          for (int y = 0; y < 4; y++)
              matrix[x][y] = glmMatrix[x][y];
      return matrix;
  }
  struct Transform {
      glm::vec3 position = glm::vec3(0);
      glm::vec3 rotation = glm::vec3(0);
      glm::vec3 scale = glm::vec3(1);
      glm::mat4 to_mat4() {
          glm::mat4 m = glm::translate(glm::mat4(1), position);
          m *= glm::mat4_cast(glm::quat(rotation));
          m = glm::scale(m, scale);
          return m;
      };
      glm::vec3 to_forward_vector() {
          glm::quat q = glm::quat(rotation);
          return glm::normalize(q * glm::vec3(0.0f, 0.0f, 1.0f));
      }
      glm::vec3 to_right_vector() {
          glm::quat q = glm::quat(rotation);
          return glm::normalize(q * glm::vec3(1.0f, 0.0f, 0.0f));
      }
  };
}
