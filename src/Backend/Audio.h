#pragma once

#include <alc.h>
#include <al.h>
#include <vector>
#include <sndfile.h>

#define SD_INIT SoundDevice::Init();
#define LISTENER SoundDevice::Get()

class SoundDevice
{
public:
	static SoundDevice* Get();
	static void Init();

	void GetLocation(float &x, float& y, float& z);
	void GetOrientation(float &ori);
	float GetGain();

	void SetAttunation(int key);
	void SetLocation(const float& x, const float& y, const float& z);
	void SetOrientation(
		const float& atx, const float& aty, const float& atz,
		const float& upx, const float& upy, const float& upz);
	void SetGain(const float& val);

private:
	SoundDevice();
	~SoundDevice();

	ALCdevice* p_ALCDevice;
	ALCcontext* p_ALCContext;
};

#define SE_LOAD SoundEffectsLibrary::Get()->Load
#define SE_UNLOAD SoundEffectsLibrary::Get()->UnLoad

class SoundEffectsPlayer
{
public:
	SoundEffectsPlayer();
	~SoundEffectsPlayer();

	void Play(const ALuint& buffer_to_play);
	void Stop();
	void Pause();
	void Resume();

	void SetBufferToPlay(const ALuint& buffer_to_play);
	void SetLooping(const bool& loop);
	void SetPosition(const float& x, const float& y, const float& z);

	bool isPlaying();

private:
	ALuint p_Source;
	ALuint p_Buffer = 0;
};

class SoundEffectsLibrary
{
public:
	static SoundEffectsLibrary* Get();

	ALuint Load(const char* filename);
	bool UnLoad(const ALuint& bufferId);

private:
	SoundEffectsLibrary();
	~SoundEffectsLibrary();

	std::vector<ALuint> p_SoundEffectBuffers;
};

class MusicBuffer
{
public:
	void Play();
	void Pause();
	void Stop();
	void Resume();

	void UpdateBufferStream();

	ALint getSource();

	bool isPlaying();

	void SetGain(const float& val);

	MusicBuffer(const char* filename);
	~MusicBuffer();
private:
	ALuint p_Source;
	static const int BUFFER_SAMPLES = 8192;
	static const int NUM_BUFFERS = 4;
	ALuint p_Buffers[NUM_BUFFERS];
	SNDFILE* p_SndFile;
	SF_INFO p_Sfinfo;
	short* p_Membuf;
	ALenum p_Format;

	MusicBuffer() = delete;
};
