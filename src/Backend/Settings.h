#pragma once

#include <cstdint>
#include <filesystem>

enum class WindowMode : uint32_t
{
  Windowed = 0,
  Fullscreen = 1,
  Borderless = 2
};

struct Settings
{
  static void Init();

  static void Save();
  static void ReloadIfChanged();
  static void ForceReload();

  static uint32_t GetWindowWidth();
  static uint32_t GetWindowHeight();
  static void SetResolution(uint32_t width, uint32_t height);
  static WindowMode GetWindowMode();
  static void SetWindowMode(WindowMode mode);
  static uint32_t GetFPSLimit();
  static void SetFPSLimit(uint32_t fps);
  static bool GetVSync();
  static void SetVSync(bool enabled);
  static float GetMusicVolume();
  static void SetMusicVolume(float volume);
  static float GetSFXVolume();
  static void SetSFXVolume(float volume);

private:
  static void Load();
  static void SetDefaults();
};

