#pragma once

#include <alc.h>
#include <al.h>
#include <vector>
#include <sndfile.h>
#include <glm/glm.hpp>

struct AudioSystem
{
  static void Init();
  static void Terminate();
	static void GetLocation(float &x, float& y, float& z);
	static void GetOrientation(float &ori);
	static float GetVolume();

	static void SetAttunation(int key);
	static void SetLocation(const glm::vec3& position);
	static void SetOrientation(const glm::vec3& forward, const glm::vec3& up);
	static void SetVolume(const float& val);
};

struct SoundPlayer
{
  static void Init();
  static void Terminate();
	static void Play(const ALuint& buffer_to_play);
  static void Play(const ALuint& buffer_to_play, const glm::vec3& position);
  static void Stop(ALuint buffer);
	static void StopAll();
  static void Pause(ALuint buffer);
	static void PauseAll();
  static void Resume(ALuint buffer);
	static void ResumeAll();
  static void SetLoop(ALuint buffer, bool loop);
	static void SetLoopALL(bool loop);

	static bool isAnyPlaying();
  static bool IsPlaying(ALuint buffer);
	static ALuint Load(const char* filename);
	static bool UnLoad(const ALuint& bufferId);
};

struct MusicSource
{
	MusicSource(const char* filename);
	~MusicSource();
	void Play();
	void Play(const glm::vec3& position);
	void Pause();
	void Stop();
	void Resume();
  void SetLoop(bool loop);
	bool isPlaying();

	void UpdateBufferStream();
	ALint getSource();
	void SetVolume(const float& val);

private:
	ALuint p_Source;
	static const int BUFFER_SAMPLES = 8192;
	static const int NUM_BUFFERS = 4;
	ALuint p_Buffers[NUM_BUFFERS];
	SNDFILE* p_SndFile;
	SF_INFO p_Sfinfo;
	short* p_Membuf;
	ALenum p_Format;
};

struct MusicPlayer
{
  static void Load(const char* filename);
  static void Play(size_t index);
  static void Play(size_t index, const glm::vec3& position);
  static void Pause(size_t index);
  static void Stop(size_t index);
  static void SetLoop(size_t index, bool loop);
  static void Resume(size_t index);
  static void UpdateAll();
  static size_t GetNumTracks();
};
