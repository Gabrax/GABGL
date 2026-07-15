#pragma once

#include <cstdint>
#include <filesystem>

enum class WindowMode : uint32_t
{
  Windowed = 0,
  Fullscreen = 1,
  Borderless = 2
};

enum class GraphicsQuality : uint32_t
{
  Off = 0,
  Low = 1,
  Medium = 2,
  High = 3
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
  static GraphicsQuality GetShadowQuality();
  static void SetShadowQuality(GraphicsQuality quality);
  static GraphicsQuality GetBloomQuality();
  static void SetBloomQuality(GraphicsQuality quality);

private:
  static void Load();
  static void SetDefaults();
};

