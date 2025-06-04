#include "Audio.h"

#include <stdio.h>
#include <vector>
#include <sndfile.h>
#include <inttypes.h>
#include <AL\alext.h>
#include <memory>
#include <iostream>

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

struct AudioSystemData
{
	ALCdevice* p_ALCDevice;
	ALCcontext* p_ALCContext;

  std::vector<ALuint> p_SoundEffectBuffers;
	std::vector<ALuint> m_Sources;
	ALuint m_LastUsedBuffer = 0;

  std::vector<std::unique_ptr<MusicSource>> players;

} s_AudioData;

void AudioSystem::Init()
{
	s_AudioData.p_ALCDevice = alcOpenDevice(nullptr); // nullptr = get default device
	if (!s_AudioData.p_ALCDevice)
		throw("failed to get sound device");

	s_AudioData.p_ALCContext = alcCreateContext(s_AudioData.p_ALCDevice, nullptr);  // create context
	if (!s_AudioData.p_ALCContext)
		throw("Failed to set sound context");

	if (!alcMakeContextCurrent(s_AudioData.p_ALCContext))   // make context current
		throw("failed to make context current");

	const ALCchar* name = nullptr;
	if (alcIsExtensionPresent(s_AudioData.p_ALCDevice, "ALC_ENUMERATE_ALL_EXT"))
		name = alcGetString(s_AudioData.p_ALCDevice, ALC_ALL_DEVICES_SPECIFIER);
	if (!name || alcGetError(s_AudioData.p_ALCDevice) != AL_NO_ERROR)
		name = alcGetString(s_AudioData.p_ALCDevice, ALC_DEVICE_SPECIFIER);
	printf("Opened \"%s\"\n", name);
}

void AudioSystem::Terminate()
{
	alcMakeContextCurrent(nullptr);
	alcDestroyContext(s_AudioData.p_ALCContext);
	alcCloseDevice(s_AudioData.p_ALCDevice);
}

void AudioSystem::GetLocation(float& x, float& y, float& z)
{
	alGetListener3f(AL_POSITION, &x, &y, &z);
	AL_CheckAndThrow();
}

void AudioSystem::GetOrientation(float& ori)
{
	alGetListenerfv(AL_ORIENTATION, &ori);
	AL_CheckAndThrow();
}

float AudioSystem::GetVolume()
{
	float curr_gain;
	alGetListenerf(AL_GAIN, &curr_gain);
	AL_CheckAndThrow();
	return curr_gain;
}

void AudioSystem::SetAttunation(int key)
{
	if (key < 0xD001 || key > 0xD006)
		throw("bad attunation key");

	alDistanceModel(key);
	AL_CheckAndThrow();
}

void AudioSystem::SetLocation(const glm::vec3& position)
{
	alListener3f(AL_POSITION, position.x, position.y, position.z);
	AL_CheckAndThrow();
}

void AudioSystem::SetOrientation(const glm::vec3& forward, const glm::vec3& up)
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

void AudioSystem::SetVolume(const float& val)
{
	// clamp between 0 and 5
	float newVol = val;
	if (newVol < 0.f)
		newVol = 0.f;
	else if (newVol > 5.f)
		newVol = 5.f;

	alListenerf(AL_GAIN, newVol);
	AL_CheckAndThrow();
}

void SoundPlayer::Init()
{
	constexpr int SOURCE_POOL_SIZE = 16;

	ALCcontext* currentCtx = alcGetCurrentContext();
	if (!currentCtx)
		std::cerr << "ERROR: No OpenAL context is active!\n";

	s_AudioData.m_Sources.resize(SOURCE_POOL_SIZE);
	alGenSources(SOURCE_POOL_SIZE, s_AudioData.m_Sources.data());
	AL_CheckAndThrow();

	s_AudioData.p_SoundEffectBuffers.clear();
}

void SoundPlayer::Terminate()
{
	alDeleteSources((ALsizei)s_AudioData.m_Sources.size(), s_AudioData.m_Sources.data());
	alDeleteBuffers((ALsizei)s_AudioData.p_SoundEffectBuffers.size(), s_AudioData.p_SoundEffectBuffers.data());

	s_AudioData.p_SoundEffectBuffers.clear();
	s_AudioData.m_Sources.clear();
}

void SoundPlayer::Play(const ALuint& buffer_to_play)
{
	for (ALuint source : s_AudioData.m_Sources)
	{
		ALint state;
		alGetSourcei(source, AL_SOURCE_STATE, &state);
		if (state != AL_PLAYING)
		{
			alSourcei(source, AL_BUFFER, buffer_to_play);
			alSourcePlay(source);
			AL_CheckAndThrow();
			s_AudioData.m_LastUsedBuffer = buffer_to_play;
			return;
		}
	}
	std::cerr << "No available source to play sound.\n";
}

void SoundPlayer::Play(const ALuint& buffer_to_play, const glm::vec3& position)
{
	for (ALuint source : s_AudioData.m_Sources)
	{
		ALint state;
		alGetSourcei(source, AL_SOURCE_STATE, &state);
		if (state != AL_PLAYING)
		{
			alSourcei(source, AL_BUFFER, buffer_to_play);
			alSource3f(source, AL_POSITION, position.x, position.y, position.z);  
			alSourcePlay(source);
			AL_CheckAndThrow();
			s_AudioData.m_LastUsedBuffer = buffer_to_play;
			return;
		}
	}
	std::cerr << "No available source to play sound.\n";
}

void SoundPlayer::Stop(ALuint buffer)
{
	for (ALuint source : s_AudioData.m_Sources)
	{
		ALint currentBuffer;
		alGetSourcei(source, AL_BUFFER, &currentBuffer);
		if ((ALuint)currentBuffer == buffer)
		{
			alSourceStop(source);
		}
	}
}

void SoundPlayer::StopAll()
{
	for (ALuint source : s_AudioData.m_Sources)
		alSourceStop(source);
}

void SoundPlayer::Pause(ALuint buffer)
{
	for (ALuint source : s_AudioData.m_Sources)
	{
		ALint currentBuffer;
		alGetSourcei(source, AL_BUFFER, &currentBuffer);
		if ((ALuint)currentBuffer == buffer)
		{
			alSourcePause(source);
		}
	}
}

void SoundPlayer::PauseAll()
{
	for (ALuint source : s_AudioData.m_Sources)
		alSourcePause(source);
}

void SoundPlayer::Resume(ALuint buffer)
{
	for (ALuint source : s_AudioData.m_Sources)
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

void SoundPlayer::ResumeAll()
{
	for (ALuint source : s_AudioData.m_Sources)
	{
		ALint state;
		alGetSourcei(source, AL_SOURCE_STATE, &state);
		if (state == AL_PAUSED)
			alSourcePlay(source);
	}
}

bool SoundPlayer::IsPlaying(ALuint buffer)
{
	for (ALuint source : s_AudioData.m_Sources)
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

bool SoundPlayer::isAnyPlaying()
{
	for (ALuint source : s_AudioData.m_Sources)
	{
		ALint state;
		alGetSourcei(source, AL_SOURCE_STATE, &state);
		if (state == AL_PLAYING)
			return true;
	}
	return false;
}

void SoundPlayer::SetLoop(ALuint buffer, bool loop)
{
	for (ALuint source : s_AudioData.m_Sources)
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

void SoundPlayer::SetLoopALL(bool loop)
{
	for (ALuint source : s_AudioData.m_Sources)
	{
		alSourcei(source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
		AL_CheckAndThrow();
	}
}

ALuint SoundPlayer::Load(const char* filename)
{
	ALenum err, format;
	ALuint buffer;
	SNDFILE* sndfile;
	SF_INFO sfinfo;
	short* membuf;
	sf_count_t num_frames;
	ALsizei num_bytes;

	sndfile = sf_open(filename, SFM_READ, &sfinfo);
	if (!sndfile)
	{
		fprintf(stderr, "Could not open audio in %s: %s\n", filename, sf_strerror(sndfile));
		return 0;
	}
	if (sfinfo.frames < 1 || sfinfo.frames >(sf_count_t)(INT_MAX / sizeof(short)) / sfinfo.channels)
	{
		fprintf(stderr, "Bad sample count in %s (%" PRId64 ")\n", filename, sfinfo.frames);
		sf_close(sndfile);
		return 0;
	}

	format = AL_NONE;
	if (sfinfo.channels == 1)
		format = AL_FORMAT_MONO16;
	else if (sfinfo.channels == 2)
		format = AL_FORMAT_STEREO16;
	else if (sfinfo.channels == 3)
	{
		if (sf_command(sndfile, SFC_WAVEX_GET_AMBISONIC, NULL, 0) == SF_AMBISONIC_B_FORMAT)
			format = AL_FORMAT_BFORMAT2D_16;
	}
	else if (sfinfo.channels == 4)
	{
		if (sf_command(sndfile, SFC_WAVEX_GET_AMBISONIC, NULL, 0) == SF_AMBISONIC_B_FORMAT)
			format = AL_FORMAT_BFORMAT3D_16;
	}
	if (!format)
	{
		fprintf(stderr, "Unsupported channel count: %d\n", sfinfo.channels);
		sf_close(sndfile);
		return 0;
	}

	membuf = static_cast<short*>(malloc((size_t)(sfinfo.frames * sfinfo.channels) * sizeof(short)));

	num_frames = sf_readf_short(sndfile, membuf, sfinfo.frames);
	if (num_frames < 1)
	{
		free(membuf);
		sf_close(sndfile);
		fprintf(stderr, "Failed to read samples in %s (%" PRId64 ")\n", filename, num_frames);
		return 0;
	}
	num_bytes = (ALsizei)(num_frames * sfinfo.channels) * (ALsizei)sizeof(short);

	buffer = 0;
	alGenBuffers(1, &buffer);
	alBufferData(buffer, format, membuf, num_bytes, sfinfo.samplerate);

	free(membuf);
	sf_close(sndfile);

	err = alGetError();
	if (err != AL_NO_ERROR)
	{
		fprintf(stderr, "OpenAL Error: %s\n", alGetString(err));
		if (buffer && alIsBuffer(buffer))
			alDeleteBuffers(1, &buffer);
		return 0;
	}

	s_AudioData.p_SoundEffectBuffers.push_back(buffer);  // add to the list of known buffers

	return buffer;
}

bool SoundPlayer::UnLoad(const ALuint& buffer)
{
	auto it = s_AudioData.p_SoundEffectBuffers.begin();
	while (it != s_AudioData.p_SoundEffectBuffers.end())
	{
		if (*it == buffer)
		{
			alDeleteBuffers(1, &*it);

			it = s_AudioData.p_SoundEffectBuffers.erase(it);

			return true;
		}
		else {
			++it;
		}
	}
	return false;  // couldn't find to remove
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

void MusicSource::Play()
{
	ALsizei i;

	alGetError();

	alSourceRewind(p_Source);
	alSourcei(p_Source, AL_BUFFER, 0);

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

void MusicSource::Play(const glm::vec3& position)
{
    ALsizei i;

    alGetError();

    alSource3f(p_Source, AL_POSITION, position.x, position.y, position.z);

    alSourceRewind(p_Source);
    alSourcei(p_Source, AL_BUFFER, 0);

    for (i = 0; i < NUM_BUFFERS; i++)
    {
        /* Get some data to give it to the buffer */
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

	/* Unqueue and handle each processed buffer */
	while (processed > 0)
	{
		ALuint bufid;
		sf_count_t slen;

		alSourceUnqueueBuffers(p_Source, 1, &bufid);
		processed--;

		/* Read the next chunk of data, refill the buffer, and queue it
		 * back on the source */
		slen = sf_readf_short(p_SndFile, p_Membuf, BUFFER_SAMPLES);
		if (slen > 0)
		{
			slen *= p_Sfinfo.channels * (sf_count_t)sizeof(short);
			alBufferData(bufid, p_Format, p_Membuf, (ALsizei)slen,
				p_Sfinfo.samplerate);
			alSourceQueueBuffers(p_Source, 1, &bufid);
		}
		if (alGetError() != AL_NO_ERROR)
		{
			throw("error buffering music data");
		}
	}

	/* Make sure the source hasn't underrun */
	if (state != AL_PLAYING && state != AL_PAUSED)
	{
		ALint queued;

		/* If no buffers are queued, playback is finished */
		alGetSourcei(p_Source, AL_BUFFERS_QUEUED, &queued);
		AL_CheckAndThrow();
		if (queued == 0)
			return;

		alSourcePlay(p_Source);
		AL_CheckAndThrow();
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
    alSourcei(p_Source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
    AL_CheckAndThrow();
}

void MusicSource::SetVolume(const float& val)
{
	float newval = val;
	if (newval < 0)
		newval = 0;
	alSourcef(p_Source, AL_GAIN, val);
	AL_CheckAndThrow();
}

void MusicPlayer::Load(const char* filename)
{
  auto player = std::make_unique<MusicSource>(filename);
  s_AudioData.players.push_back(std::move(player));
}

void MusicPlayer::Play(size_t index)
{
  if (index < s_AudioData.players.size())
      s_AudioData.players[index]->Play();
}

void MusicPlayer::Play(size_t index,const glm::vec3& position)
{
  if (index < s_AudioData.players.size())
      s_AudioData.players[index]->Play(position);
}

void MusicPlayer::Pause(size_t index)
{
  if (index < s_AudioData.players.size())
      s_AudioData.players[index]->Pause();
}

void MusicPlayer::Stop(size_t index)
{
  if (index < s_AudioData.players.size())
      s_AudioData.players[index]->Stop();
}

void MusicPlayer::Resume(size_t index)
{
  if (index < s_AudioData.players.size())
      s_AudioData.players[index]->Resume();
}

void MusicPlayer::SetLoop(size_t index, bool loop)
{
  if (index < s_AudioData.players.size())
      s_AudioData.players[index]->SetLoop(loop);
}

void MusicPlayer::UpdateAll()
{
  for (auto& player : s_AudioData.players)
      player->UpdateBufferStream();
}

size_t MusicPlayer::GetNumTracks()
{ 
  return s_AudioData.players.size();
}
