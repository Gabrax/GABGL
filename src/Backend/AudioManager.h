#pragma once

#include <alc.h>
#include <al.h>
#include <glm/glm.hpp>
#include <string>

struct AudioManager
{
  // LISTENER
  static void Init();
  static void Terminate();
	static void SetListenerLocation(const glm::vec3& position);
	static void GetListenerLocation(float &x, float& y, float& z);
	static void SetListenerOrientation(const glm::vec3& forward, const glm::vec3& up);
	static void GetListenerOrientation(float &ori);
	static void SetListenerVolume(const float& val);
	static float GetListenerVolume();
	static void SetAttunation(int key);

  // SOUND
	static void LoadSound(const char* filename);
	static bool UnLoadSound(const std::string& name);
	static void PlaySound(const std::string& name, const float& volume = 1.0f);
  static void PlaySound(const std::string& name, const glm::vec3& position, const float& volume = 1.0f);
  static void StopSound(const std::string& name);
	static void StopAllSounds();
  static void PauseSound(const std::string& name);
	static void PauseAllSounds();
  static void ResumeSound(const std::string& name);
	static void ResumeAllSounds();
  static void SetSoundLoop(const std::string& name, bool loop);
	static bool isAnySoundsPlaying();
  static bool IsSoundPlaying(const std::string& name);

  // MUSIC
  static void LoadMusic(const char* filename);
  static void PlayMusic(const std::string& name, bool loop = false, const float& volume = 1.0f);
  static void PlayMusic(const std::string& name, const glm::vec3& position, const float& volume = 1.0f);
  static void PauseMusic(const std::string& name);
  static void StopMusic(const std::string& name);
  static void SetMusicLoop(const std::string& name, bool loop);
  static void ResumeMusic(const std::string& name);
  static void UpdateAllMusic();
  static size_t GetMusicNumTracks();
};
