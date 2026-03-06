#pragma once

#include <cstdint>
#include <filesystem>

struct Settings
{
  static void Init();

  static void Save();
  static void ReloadIfChanged();
  static void ForceReload();

  static uint32_t GetWindowWidth();
  static uint32_t GetWindowHeight();
  static uint32_t GetFPS();

private:
  static void Load();
  static void SetDefaults();
};

