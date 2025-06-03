#include "Audio.h"

#include <stdio.h>
#include <vector>
#include <sndfile.h>
#include <inttypes.h>
#include <AL\alext.h>

static void ALC_CheckAndThrow(ALCdevice* device)
{
	if (alcGetError(device) != ALC_NO_ERROR)
	{
		throw("error with alcDevice");
	}
}

static void AL_CheckAndThrow()
{
	if (alGetError() != AL_NO_ERROR)
	{
		throw("error with al");
	}
}

static SoundDevice* _instance = nullptr;

SoundDevice* SoundDevice::Get()
{
	Init();
	return _instance;
}

void SoundDevice::Init()
{
	if (_instance == nullptr)
		_instance = new SoundDevice();
}

void SoundDevice::GetLocation(float& x, float& y, float& z)
{
	alGetListener3f(AL_POSITION, &x, &y, &z);
	AL_CheckAndThrow();
}

void SoundDevice::GetOrientation(float& ori)
{
	alGetListenerfv(AL_ORIENTATION, &ori);
	AL_CheckAndThrow();
}

float SoundDevice::GetGain()
{
	float curr_gain;
	alGetListenerf(AL_GAIN, &curr_gain);
	AL_CheckAndThrow();
	return curr_gain;
}

void SoundDevice::SetAttunation(int key)
{
	if (key < 0xD001 || key > 0xD006)
		throw("bad attunation key");

	alDistanceModel(key);
	AL_CheckAndThrow();
}

void SoundDevice::SetLocation(const float& x, const float& y, const float& z)
{
	alListener3f(AL_POSITION, x, y, z);
	AL_CheckAndThrow();
}

void SoundDevice::SetOrientation(const float& atx, const float& aty, const float& atz, const float& upx, const float& upy, const float& upz)
{
	std::vector<float> ori;
	ori.push_back(atx);
	ori.push_back(aty);
	ori.push_back(atz);
	ori.push_back(upx);
	ori.push_back(upy);
	ori.push_back(upz);
	alListenerfv(AL_ORIENTATION, ori.data());
	AL_CheckAndThrow();
}

void SoundDevice::SetGain(const float& val)
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

SoundDevice::SoundDevice()
{
	p_ALCDevice = alcOpenDevice(nullptr); // nullptr = get default device
	if (!p_ALCDevice)
		throw("failed to get sound device");

	p_ALCContext = alcCreateContext(p_ALCDevice, nullptr);  // create context
	if (!p_ALCContext)
		throw("Failed to set sound context");

	if (!alcMakeContextCurrent(p_ALCContext))   // make context current
		throw("failed to make context current");

	const ALCchar* name = nullptr;
	if (alcIsExtensionPresent(p_ALCDevice, "ALC_ENUMERATE_ALL_EXT"))
		name = alcGetString(p_ALCDevice, ALC_ALL_DEVICES_SPECIFIER);
	if (!name || alcGetError(p_ALCDevice) != AL_NO_ERROR)
		name = alcGetString(p_ALCDevice, ALC_DEVICE_SPECIFIER);
	printf("Opened \"%s\"\n", name);
}

SoundDevice::~SoundDevice()
{
	alcMakeContextCurrent(nullptr);
	alcDestroyContext(p_ALCContext);
	alcCloseDevice(p_ALCDevice);
}

SoundEffectsPlayer::SoundEffectsPlayer()
{
	alGenSources(1, &p_Source);
	alSourcei(p_Source, AL_BUFFER, p_Buffer);
	AL_CheckAndThrow();
}

SoundEffectsPlayer::~SoundEffectsPlayer()
{
	alDeleteSources(1, &p_Source);
}

void SoundEffectsPlayer::Play(const ALuint& buffer_to_play)
{
	if (buffer_to_play != p_Buffer)
	{
			p_Buffer = buffer_to_play;
			alSourcei(p_Source, AL_BUFFER, (ALint)p_Buffer);
			AL_CheckAndThrow();
	}
	alSourcePlay(p_Source);
	AL_CheckAndThrow();
}

void SoundEffectsPlayer::Stop()
{
	alSourceStop(p_Source);
	AL_CheckAndThrow();
}

void SoundEffectsPlayer::Pause()
{
	alSourcePause(p_Source);
	AL_CheckAndThrow();
}

void SoundEffectsPlayer::Resume()
{
	alSourcePlay(p_Source);
	AL_CheckAndThrow();
}

void SoundEffectsPlayer::SetBufferToPlay(const ALuint& buffer_to_play)
{
	if (buffer_to_play != p_Buffer)
	{
		p_Buffer = buffer_to_play;
		alSourcei(p_Source, AL_BUFFER, (ALint)p_Buffer);
		AL_CheckAndThrow();
	}
}

void SoundEffectsPlayer::SetLooping(const bool& loop)
{
	alSourcei(p_Source, AL_LOOPING, (ALint)loop);
	AL_CheckAndThrow();
}

void SoundEffectsPlayer::SetPosition(const float& x, const float& y, const float& z)
{
	alSource3f(p_Source, AL_POSITION, x, y, z);
	AL_CheckAndThrow();
}

bool SoundEffectsPlayer::isPlaying()
{
	ALint playState;
	alGetSourcei(p_Source, AL_SOURCE_STATE, &playState);
	return (playState == AL_PLAYING);
}

SoundEffectsLibrary* SoundEffectsLibrary::Get()
{
	static SoundEffectsLibrary* sndbuf = new SoundEffectsLibrary();
	return sndbuf;
}

ALuint SoundEffectsLibrary::Load(const char* filename)
{

	ALenum err, format;
	ALuint buffer;
	SNDFILE* sndfile;
	SF_INFO sfinfo;
	short* membuf;
	sf_count_t num_frames;
	ALsizei num_bytes;

	/* Open the audio file and check that it's usable. */
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

	/* Get the sound format, and figure out the OpenAL format */
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

	/* Decode the whole audio file to a buffer. */
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

	/* Buffer the audio data into a new buffer object, then free the data and
	 * close the file.
	 */
	buffer = 0;
	alGenBuffers(1, &buffer);
	alBufferData(buffer, format, membuf, num_bytes, sfinfo.samplerate);

	free(membuf);
	sf_close(sndfile);

	/* Check if an error occured, and clean up if so. */
	err = alGetError();
	if (err != AL_NO_ERROR)
	{
		fprintf(stderr, "OpenAL Error: %s\n", alGetString(err));
		if (buffer && alIsBuffer(buffer))
			alDeleteBuffers(1, &buffer);
		return 0;
	}

	p_SoundEffectBuffers.push_back(buffer);  // add to the list of known buffers

	return buffer;
}

bool SoundEffectsLibrary::UnLoad(const ALuint& buffer)
{
	auto it = p_SoundEffectBuffers.begin();
	while (it != p_SoundEffectBuffers.end())
	{
		if (*it == buffer)
		{
			alDeleteBuffers(1, &*it);

			it = p_SoundEffectBuffers.erase(it);

			return true;
		}
		else {
			++it;
		}
	}
	return false;  // couldn't find to remove
}

SoundEffectsLibrary::SoundEffectsLibrary()
{
	p_SoundEffectBuffers.clear();
}

SoundEffectsLibrary::~SoundEffectsLibrary()
{
	alDeleteBuffers((ALsizei)p_SoundEffectBuffers.size(), p_SoundEffectBuffers.data());

	p_SoundEffectBuffers.clear();
}

void MusicBuffer::Play()
{
	ALsizei i;

	// clear any al errors
	alGetError();

	/* Rewind the source position and clear the buffer queue */
	alSourceRewind(p_Source);
	alSourcei(p_Source, AL_BUFFER, 0);

	/* Fill the buffer queue */
	for (i = 0; i < NUM_BUFFERS; i++)
	{
		/* Get some data to give it to the buffer */
		sf_count_t slen = sf_readf_short(p_SndFile, p_Membuf, BUFFER_SAMPLES);
		if (slen < 1) break;

		slen *= p_Sfinfo.channels * (sf_count_t)sizeof(short);
		alBufferData(p_Buffers[i], p_Format, p_Membuf, (ALsizei)slen, p_Sfinfo.samplerate);
	}
	if (alGetError() != AL_NO_ERROR)
	{
		throw("Error buffering for playback");
	}

	/* Now queue and start playback! */
	alSourceQueueBuffers(p_Source, i, p_Buffers);
	alSourcePlay(p_Source);
	if (alGetError() != AL_NO_ERROR)
	{
		throw("Error starting playback");
	}

}

void MusicBuffer::Pause()
{
	alSourcePause(p_Source);
	AL_CheckAndThrow();
}

void MusicBuffer::Stop()
{
	alSourceStop(p_Source);
	AL_CheckAndThrow();
}

void MusicBuffer::Resume()
{
	alSourcePlay(p_Source);
	AL_CheckAndThrow();
}

void MusicBuffer::UpdateBufferStream()
{
	ALint processed, state;

	// clear error 
	//alGetError();
	/* Get relevant source info */
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

ALint MusicBuffer::getSource()
{
	return p_Source;
}

bool MusicBuffer::isPlaying()
{
	ALint state;
	alGetSourcei(p_Source, AL_SOURCE_STATE, &state);
	AL_CheckAndThrow();
	return (state == AL_PLAYING);
}

void MusicBuffer::SetGain(const float& val)
{
	float newval = val;
	if (newval < 0)
		newval = 0;
	alSourcef(p_Source, AL_GAIN, val);
	AL_CheckAndThrow();
}

MusicBuffer::MusicBuffer(const char* filename)
{
	alGenSources(1, &p_Source);
	alGenBuffers(NUM_BUFFERS, p_Buffers);

	std::size_t frame_size;

	p_SndFile = sf_open(filename, SFM_READ, &p_Sfinfo);
	if (!p_SndFile)
	{
		throw("could not open provided music file -- check path");
	}

	/* Get the sound format, and figure out the OpenAL format */
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

MusicBuffer::~MusicBuffer()
{
	alDeleteSources(1, &p_Source);
	if (p_SndFile)
		sf_close(p_SndFile);
	p_SndFile = nullptr;
	free(p_Membuf);
	alDeleteBuffers(NUM_BUFFERS, p_Buffers);
}
