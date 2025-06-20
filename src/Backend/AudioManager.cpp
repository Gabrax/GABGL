#include "AudioManager.h"

#include <stdio.h>
#include <vector>
#include <sndfile.h>
#include <inttypes.h>
#include <AL\alext.h>
#include <memory>
#include <iostream>
#include <unordered_map>
#include <filesystem>
#include <mutex>
#include "../backend/BackendLogger.h"

static void ALC_CheckAndThrow(ALCdevice* device)
{
  ALCenum err = alcGetError(device);
  if (err != ALC_NO_ERROR)
  {
      const char* msg = alcGetString(device, err);
      throw std::runtime_error(std::string("ALC error: ") + (msg ? msg : "Unknown ALC error"));
  }
}

static void AL_CheckAndThrow()
{
  ALenum err = alGetError();
  if (err != AL_NO_ERROR)
  {
      const char* msg = alGetString(err);
      throw std::runtime_error(std::string("AL error: ") + (msg ? msg : "Unknown AL error"));
  }
}

struct MusicSource
{
	MusicSource(const char* filename);
	~MusicSource();
	void Play(const float& volume);
	void Play(const glm::vec3& position, const float& volume);
	void Pause();
	void Stop();
	void Resume();
  void SetLoop(bool loop);
	bool isPlaying();

	void UpdateBufferStream();
	ALint getSource();
	void SetVolume(const float& val);

private:
  bool loop = false;
	ALuint p_Source;
	static const int BUFFER_SAMPLES = 8192;
	static const int NUM_BUFFERS = 4;
	ALuint p_Buffers[NUM_BUFFERS];
	SNDFILE* p_SndFile;
	SF_INFO p_Sfinfo;
	short* p_Membuf;
	ALenum p_Format;
};

struct AudioSystemData
{
	ALCdevice* p_ALCDevice;
	ALCcontext* p_ALCContext;

  std::unordered_map<std::string, ALuint> p_SoundEffectBuffers;
	std::vector<ALuint> m_Sources;
	ALuint m_LastUsedBuffer = 0;

  std::unordered_map<std::string, std::unique_ptr<MusicSource>> players;

  std::mutex s_AudioMutex;

} s_Data;

void AudioManager::Init()
{
	s_Data.p_ALCDevice = alcOpenDevice(nullptr); // nullptr = get default device
	if (!s_Data.p_ALCDevice)
		throw("failed to get sound device");

	s_Data.p_ALCContext = alcCreateContext(s_Data.p_ALCDevice, nullptr);  // create context
	if (!s_Data.p_ALCContext)
		throw("Failed to set sound context");

	if (!alcMakeContextCurrent(s_Data.p_ALCContext))   // make context current
		throw("failed to make context current");

	const ALCchar* name = nullptr;
	if (alcIsExtensionPresent(s_Data.p_ALCDevice, "ALC_ENUMERATE_ALL_EXT"))
		name = alcGetString(s_Data.p_ALCDevice, ALC_ALL_DEVICES_SPECIFIER);
	if (!name || alcGetError(s_Data.p_ALCDevice) != AL_NO_ERROR)
		name = alcGetString(s_Data.p_ALCDevice, ALC_DEVICE_SPECIFIER);
	/*printf("Opened \"%s\"\n", name);*/
  GABGL_INFO((const char*)name);

  constexpr int SOURCE_POOL_SIZE = 16;

	ALCcontext* currentCtx = alcGetCurrentContext();
	if (!currentCtx)
		std::cerr << "ERROR: No OpenAL context is active!\n";

	s_Data.m_Sources.resize(SOURCE_POOL_SIZE);
	alGenSources(SOURCE_POOL_SIZE, s_Data.m_Sources.data());
	AL_CheckAndThrow();

	s_Data.p_SoundEffectBuffers.clear();

}

void AudioManager::Terminate()
{
  alcMakeContextCurrent(nullptr);
  alcDestroyContext(s_Data.p_ALCContext);
  alcCloseDevice(s_Data.p_ALCDevice);

  alDeleteSources((ALsizei)s_Data.m_Sources.size(), s_Data.m_Sources.data());

  std::vector<ALuint> allBuffers;
  for (auto& [name, buffer] : s_Data.p_SoundEffectBuffers)
      allBuffers.push_back(buffer);
  alDeleteBuffers((ALsizei)allBuffers.size(), allBuffers.data());

  s_Data.p_SoundEffectBuffers.clear();
  s_Data.m_Sources.clear();
}

void AudioManager::GetListenerLocation(float& x, float& y, float& z)
{
	alGetListener3f(AL_POSITION, &x, &y, &z);
	AL_CheckAndThrow();
}

void AudioManager::GetListenerOrientation(float& ori)
{
	alGetListenerfv(AL_ORIENTATION, &ori);
	AL_CheckAndThrow();
}

float AudioManager::GetListenerVolume()
{
	float curr_gain;
	alGetListenerf(AL_GAIN, &curr_gain);
	AL_CheckAndThrow();
	return curr_gain;
}

void AudioManager::SetAttunation(int key)
{
	if (key < 0xD001 || key > 0xD006)
		throw("bad attunation key");

	alDistanceModel(key);
	AL_CheckAndThrow();
}

void AudioManager::SetListenerLocation(const glm::vec3& position)
{
	alListener3f(AL_POSITION, position.x, position.y, position.z);
	AL_CheckAndThrow();
}

void AudioManager::SetListenerOrientation(const glm::vec3& forward, const glm::vec3& up)
{
	std::vector<float> ori;
	ori.push_back(forward.x);
	ori.push_back(forward.y);
	ori.push_back(forward.z);
	ori.push_back(up.x);
	ori.push_back(up.y);
	ori.push_back(up.z);
	alListenerfv(AL_ORIENTATION, ori.data());
	AL_CheckAndThrow();
}

void AudioManager::SetListenerVolume(const float& val)
{
	// clamp between 0 and 5
	float newVol = val;
	if (newVol < 0.f) newVol = 0.f;
	else if (newVol > 5.f) newVol = 5.f;

	alListenerf(AL_GAIN, newVol);
	AL_CheckAndThrow();
}

void AudioManager::PlaySound(const std::string& name, const float& volume)
{
  auto it = s_Data.p_SoundEffectBuffers.find(name);
  if (it == s_Data.p_SoundEffectBuffers.end())
      return;

  ALuint buffer = it->second;

  for (ALuint source : s_Data.m_Sources) {
      ALint state;
      alGetSourcei(source, AL_SOURCE_STATE, &state);
      if (state != AL_PLAYING) {
          alSourcei(source, AL_BUFFER, buffer);
          alSourcef(source, AL_GAIN, volume);  // Set volume before playing
          alSourcePlay(source);
          AL_CheckAndThrow();
          s_Data.m_LastUsedBuffer = buffer;
          return;
      }
  }

  std::cerr << "No available source to play sound.\n";
}


void AudioManager::PlaySound(const std::string& name, const glm::vec3& position, const float& volume)
{
  auto it = s_Data.p_SoundEffectBuffers.find(name);
  if (it == s_Data.p_SoundEffectBuffers.end())
      return;

  ALuint buffer = it->second;

  for (ALuint source : s_Data.m_Sources) {
      ALint state;
      alGetSourcei(source, AL_SOURCE_STATE, &state);
      if (state != AL_PLAYING) {
          alSourcei(source, AL_BUFFER, buffer);
          alSourcef(source, AL_GAIN, volume);  // Set volume before playing
          alSource3f(source, AL_POSITION, position.x, position.y, position.z);
          alSourcePlay(source);
          AL_CheckAndThrow();
          s_Data.m_LastUsedBuffer = buffer;
          return;
      }
  }

  std::cerr << "No available source to play sound.\n";
}

void AudioManager::StopSound(const std::string& name)
{
  auto it = s_Data.p_SoundEffectBuffers.find(name);
  if (it == s_Data.p_SoundEffectBuffers.end())
      return;

  ALuint buffer = it->second;

  for (ALuint source : s_Data.m_Sources) {
      ALint currentBuffer;
      alGetSourcei(source, AL_BUFFER, &currentBuffer);
      if ((ALuint)currentBuffer == buffer) {
          alSourceStop(source);
      }
  }
}

void AudioManager::StopAllSounds()
{
	for (ALuint source : s_Data.m_Sources)
		alSourceStop(source);
}

void AudioManager::PauseSound(const std::string& name)
{
  auto it = s_Data.p_SoundEffectBuffers.find(name);
  if (it == s_Data.p_SoundEffectBuffers.end())
      return;  // sound not found

  ALuint buffer = it->second;

  for (ALuint source : s_Data.m_Sources)
  {
      ALint currentBuffer;
      alGetSourcei(source, AL_BUFFER, &currentBuffer);
      if ((ALuint)currentBuffer == buffer)
      {
          alSourcePause(source);
      }
  }
}

void AudioManager::PauseAllSounds()
{
	for (ALuint source : s_Data.m_Sources)
		alSourcePause(source);
}

void AudioManager::ResumeSound(const std::string& name)
{
  auto it = s_Data.p_SoundEffectBuffers.find(name);
  if (it == s_Data.p_SoundEffectBuffers.end())
      return;

  ALuint buffer = it->second;

  for (ALuint source : s_Data.m_Sources)
  {
      ALint currentBuffer, state;
      alGetSourcei(source, AL_BUFFER, &currentBuffer);
      alGetSourcei(source, AL_SOURCE_STATE, &state);
      if ((ALuint)currentBuffer == buffer && state == AL_PAUSED)
      {
          alSourcePlay(source);
      }
  }
}

void AudioManager::ResumeAllSounds()
{
	for (ALuint source : s_Data.m_Sources)
	{
		ALint state;
		alGetSourcei(source, AL_SOURCE_STATE, &state);
		if (state == AL_PAUSED)
			alSourcePlay(source);
	}
}

bool AudioManager::IsSoundPlaying(const std::string& name)
{
  auto it = s_Data.p_SoundEffectBuffers.find(name);
  if (it == s_Data.p_SoundEffectBuffers.end())
      return false;

  ALuint buffer = it->second;

  for (ALuint source : s_Data.m_Sources)
  {
      ALint currentBuffer, state;
      alGetSourcei(source, AL_BUFFER, &currentBuffer);
      alGetSourcei(source, AL_SOURCE_STATE, &state);
      if ((ALuint)currentBuffer == buffer && state == AL_PLAYING)
      {
          return true;
      }
  }
  return false;
}

bool AudioManager::isAnySoundsPlaying()
{
	for (ALuint source : s_Data.m_Sources)
	{
		ALint state;
		alGetSourcei(source, AL_SOURCE_STATE, &state);
		if (state == AL_PLAYING)
			return true;
	}
	return false;
}

void AudioManager::SetSoundLoop(const std::string& name, bool loop)
{
  auto it = s_Data.p_SoundEffectBuffers.find(name);
  if (it == s_Data.p_SoundEffectBuffers.end())
      return;

  ALuint buffer = it->second;

  for (ALuint source : s_Data.m_Sources)
  {
      ALint currentBuffer;
      alGetSourcei(source, AL_BUFFER, &currentBuffer);
      if ((ALuint)currentBuffer == buffer)
      {
          alSourcei(source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
          AL_CheckAndThrow();
      }
  }
}

void AudioManager::LoadSound(const char* filename)
{
  std::string name = std::filesystem::path(filename).stem().string();

  {
      std::lock_guard<std::mutex> lock(s_Data.s_AudioMutex);
      if (s_Data.p_SoundEffectBuffers.contains(name)) {
          GABGL_ERROR("Sound '" + name + "' already loaded");
          return;
      }
  }

  ALenum format = AL_NONE;
  ALuint buffer;
  SNDFILE* sndfile;
  SF_INFO sfinfo;
  short* membuf;
  sf_count_t num_frames;
  ALsizei num_bytes;

  sndfile = sf_open(filename, SFM_READ, &sfinfo);
  if (!sndfile) {
      std::cerr << "Could not open audio in " << filename << ": " << sf_strerror(sndfile) << "\n";
      return;
  }

  if (sfinfo.frames < 1 || sfinfo.frames > (sf_count_t)(INT_MAX / sizeof(short)) / sfinfo.channels) {
      std::cerr << "Bad sample count in " << filename << " (" << sfinfo.frames << ")\n";
      sf_close(sndfile);
      return;
  }

  if (sfinfo.channels == 1) format = AL_FORMAT_MONO16;
  else if (sfinfo.channels == 2) format = AL_FORMAT_STEREO16;
  else if (sfinfo.channels == 3 &&
           sf_command(sndfile, SFC_WAVEX_GET_AMBISONIC, nullptr, 0) == SF_AMBISONIC_B_FORMAT)
      format = AL_FORMAT_BFORMAT2D_16;
  else if (sfinfo.channels == 4 &&
           sf_command(sndfile, SFC_WAVEX_GET_AMBISONIC, nullptr, 0) == SF_AMBISONIC_B_FORMAT)
      format = AL_FORMAT_BFORMAT3D_16;

  if (format == AL_NONE) {
      std::cerr << "Unsupported channel count: " << sfinfo.channels << "\n";
      sf_close(sndfile);
      return;
  }

  membuf = static_cast<short*>(malloc((size_t)(sfinfo.frames * sfinfo.channels) * sizeof(short)));
  if (!membuf) {
      std::cerr << "Failed to allocate memory for audio buffer\n";
      sf_close(sndfile);
      return;
  }

  num_frames = sf_readf_short(sndfile, membuf, sfinfo.frames);
  if (num_frames < 1) {
      std::cerr << "Failed to read samples in " << filename << " (" << num_frames << ")\n";
      free(membuf);
      sf_close(sndfile);
      return;
  }

  num_bytes = static_cast<ALsizei>(num_frames * sfinfo.channels * sizeof(short));
  alGenBuffers(1, &buffer);
  alBufferData(buffer, format, membuf, num_bytes, sfinfo.samplerate);

  free(membuf);
  sf_close(sndfile);

  ALenum err = alGetError();
  if (err != AL_NO_ERROR) {
      std::cerr << "OpenAL Error: " << alGetString(err) << "\n";
      if (buffer && alIsBuffer(buffer))
          alDeleteBuffers(1, &buffer);
      return;
  }

  {
      std::lock_guard<std::mutex> lock(s_Data.s_AudioMutex);
      s_Data.p_SoundEffectBuffers[name] = buffer;
  }
  GABGL_WARN("Sound loaded: {0}",name);
}

bool AudioManager::UnLoadSound(const std::string& name)
{
  auto it = s_Data.p_SoundEffectBuffers.find(name);
  if (it != s_Data.p_SoundEffectBuffers.end()) {
      alDeleteBuffers(1, &it->second);
      s_Data.p_SoundEffectBuffers.erase(it);
      return true;
  }
  return false;
}

MusicSource::MusicSource(const char* filename)
{
	alGenSources(1, &p_Source);
	alGenBuffers(NUM_BUFFERS, p_Buffers);

	std::size_t frame_size;

	p_SndFile = sf_open(filename, SFM_READ, &p_Sfinfo);
	if (!p_SndFile)
	{
		throw("could not open provided music file -- check path");
	}

	if (p_Sfinfo.channels == 1)
		p_Format = AL_FORMAT_MONO16;
	else if (p_Sfinfo.channels == 2)
		p_Format = AL_FORMAT_STEREO16;
	else if (p_Sfinfo.channels == 3)
	{
		if (sf_command(p_SndFile, SFC_WAVEX_GET_AMBISONIC, NULL, 0) == SF_AMBISONIC_B_FORMAT)
			p_Format = AL_FORMAT_BFORMAT2D_16;
	}
	else if (p_Sfinfo.channels == 4)
	{
		if (sf_command(p_SndFile, SFC_WAVEX_GET_AMBISONIC, NULL, 0) == SF_AMBISONIC_B_FORMAT)
			p_Format = AL_FORMAT_BFORMAT3D_16;
	}
	if (!p_Format)
	{
		sf_close(p_SndFile);
		p_SndFile = NULL;
		throw("Unsupported channel count from file");
	}

	frame_size = ((size_t)BUFFER_SAMPLES * (size_t)p_Sfinfo.channels) * sizeof(short);
	p_Membuf = static_cast<short*>(malloc(frame_size));
}

MusicSource::~MusicSource()
{
	alDeleteSources(1, &p_Source);
	if (p_SndFile)
		sf_close(p_SndFile);
	p_SndFile = nullptr;
	free(p_Membuf);
	alDeleteBuffers(NUM_BUFFERS, p_Buffers);
}

void MusicSource::Play(const float& volume)
{
  alSourceRewind(p_Source);
  alSourcei(p_Source, AL_BUFFER, 0);
  alSourcef(p_Source, AL_GAIN, volume);

  // Rewind file at start of play
  if (p_SndFile)
      sf_seek(p_SndFile, 0, SEEK_SET);

  ALsizei i;
  for (i = 0; i < NUM_BUFFERS; i++)
  {
      sf_count_t slen = sf_readf_short(p_SndFile, p_Membuf, BUFFER_SAMPLES);
      if (slen < 1) break;

      slen *= p_Sfinfo.channels * (sf_count_t)sizeof(short);
      alBufferData(p_Buffers[i], p_Format, p_Membuf, (ALsizei)slen, p_Sfinfo.samplerate);
  }
  if (alGetError() != AL_NO_ERROR) throw("Error buffering for playback");

  alSourceQueueBuffers(p_Source, i, p_Buffers);
  alSourcePlay(p_Source);
  if (alGetError() != AL_NO_ERROR) throw("Error starting playback");
}

void MusicSource::Play(const glm::vec3& position, const float& volume)
{
  ALsizei i;

  alGetError();

  alSource3f(p_Source, AL_POSITION, position.x, position.y, position.z);
  alSourceRewind(p_Source);
  alSourcei(p_Source, AL_BUFFER, 0);
  alSourcef(p_Source, AL_GAIN, volume);  // Set volume

  for (i = 0; i < NUM_BUFFERS; i++)
  {
      sf_count_t slen = sf_readf_short(p_SndFile, p_Membuf, BUFFER_SAMPLES);
      if (slen < 1) break;

      slen *= p_Sfinfo.channels * (sf_count_t)sizeof(short);
      alBufferData(p_Buffers[i], p_Format, p_Membuf, (ALsizei)slen, p_Sfinfo.samplerate);
  }
  if (alGetError() != AL_NO_ERROR) throw("Error buffering for playback");

  alSourceQueueBuffers(p_Source, i, p_Buffers);
  alSourcePlay(p_Source);
  if (alGetError() != AL_NO_ERROR) throw("Error starting playback");
}

void MusicSource::Pause()
{
	alSourcePause(p_Source);
	AL_CheckAndThrow();
}

void MusicSource::Stop()
{
	alSourceStop(p_Source);
	AL_CheckAndThrow();
}

void MusicSource::Resume()
{
	alSourcePlay(p_Source);
	AL_CheckAndThrow();
}

void MusicSource::UpdateBufferStream()
{
  ALint processed, state;

  alGetSourcei(p_Source, AL_SOURCE_STATE, &state);
  alGetSourcei(p_Source, AL_BUFFERS_PROCESSED, &processed);
  AL_CheckAndThrow();

  while (processed > 0)
  {
      ALuint bufid;
      sf_count_t slen;

      alSourceUnqueueBuffers(p_Source, 1, &bufid);
      processed--;

      slen = sf_readf_short(p_SndFile, p_Membuf, BUFFER_SAMPLES);
      if (slen == 0 && loop)
      {
          // Rewind and continue streaming if looping enabled
          sf_seek(p_SndFile, 0, SEEK_SET);
          slen = sf_readf_short(p_SndFile, p_Membuf, BUFFER_SAMPLES);
      }

      if (slen > 0)
      {
          slen *= p_Sfinfo.channels * (sf_count_t)sizeof(short);
          alBufferData(bufid, p_Format, p_Membuf, (ALsizei)slen, p_Sfinfo.samplerate);
          alSourceQueueBuffers(p_Source, 1, &bufid);
      }
      else
      {
          // No data and no loop, just skip re-queuing buffer
          // Playback will stop once buffers run out
      }

      if (alGetError() != AL_NO_ERROR)
          throw("error buffering music data");
  }

  if (state != AL_PLAYING && state != AL_PAUSED)
  {
      ALint queued;
      alGetSourcei(p_Source, AL_BUFFERS_QUEUED, &queued);
      AL_CheckAndThrow();
      if (queued > 0)
      {
          alSourcePlay(p_Source);
          AL_CheckAndThrow();
      }
  }
}

ALint MusicSource::getSource()
{
	return p_Source;
}

bool MusicSource::isPlaying()
{
	ALint state;
	alGetSourcei(p_Source, AL_SOURCE_STATE, &state);
	AL_CheckAndThrow();
	return (state == AL_PLAYING);
}

void MusicSource::SetLoop(bool loop)
{
  this->loop = loop;
}

void MusicSource::SetVolume(const float& val)
{
	float newval = val;
	if (newval < 0) newval = 0;
	alSourcef(p_Source, AL_GAIN, val);
	AL_CheckAndThrow();
}

void AudioManager::LoadMusic(const char* filename)
{
  std::string name = std::filesystem::path(filename).stem().string();

  std::lock_guard<std::mutex> lock(s_Data.s_AudioMutex);
  auto [it, inserted] = s_Data.players.emplace(name, nullptr);
  if (inserted) it->second = std::make_unique<MusicSource>(filename);

  GABGL_WARN("Music loaded: {0}",name);
}

void AudioManager::PlayMusic(const std::string& name, bool loop, const float& volume)
{
  auto it = s_Data.players.find(name);
  if (it != s_Data.players.end())
  {
      it->second->Play(volume);
      if(loop) it->second->SetLoop(loop);
  }
}

void AudioManager::PlayMusic(const std::string& name, const glm::vec3& position, const float& volume)
{
  auto it = s_Data.players.find(name);
  if (it != s_Data.players.end())
      it->second->Play(position,volume);
}

void AudioManager::PauseMusic(const std::string& name)
{
  auto it = s_Data.players.find(name);
  if (it != s_Data.players.end())
      it->second->Pause();
}

void AudioManager::StopMusic(const std::string& name)
{
  auto it = s_Data.players.find(name);
  if (it != s_Data.players.end())
      it->second->Stop();
}

void AudioManager::ResumeMusic(const std::string& name)
{
  auto it = s_Data.players.find(name);
  if (it != s_Data.players.end())
      it->second->Resume();
}

void AudioManager::UpdateAllMusic()
{
  for (auto& [name, player] : s_Data.players)
      player->UpdateBufferStream();
}

size_t AudioManager::GetMusicNumTracks()
{ 
  return s_Data.players.size();
}
