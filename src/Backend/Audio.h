#pragma once

#include <alc.h>

struct SoundDevice
{
  static SoundDevice* get();
private:
  ALCdevice* m_ALCDevice;
  ALCcontext* m_ALCContext;
};
