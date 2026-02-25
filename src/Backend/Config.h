#pragma once

#include <cstdint>
#include <filesystem>

struct Config
{
  Config();

  void Save();
  void ReloadIfChanged();
  void ForceReload();

  uint32_t GetWindowWidth()  const { return m_WindowWidth; }
  uint32_t GetWindowHeight() const { return m_WindowHeight; }
  bool     IsVSync()         const { return m_VSYNC; }
  uint32_t GetFPS()          const { return m_FPS; }

	inline static Config& GetInstance() { return *s_Instance; }

private:
  void Load();
  void SetDefaults();

private:
  static constexpr const char* CONFIG_FILE = "gab.ini";
  static constexpr uint32_t CONFIG_VERSION = 1;

  std::filesystem::file_time_type m_LastWriteTime;

  uint32_t m_Version;

  uint32_t m_WindowWidth  = 1280;
  uint32_t m_WindowHeight = 720;
  uint32_t m_RenderWidth  = 1280;
  uint32_t m_RenderHeight = 720;

  bool     m_VSYNC = true;
  uint32_t m_FPS   = 60;

  enum class WindowOpt : uint32_t { FULLSCREEN, WINDOWED, BORDERLESS };
  enum class ShadowOpt : uint32_t { OFF, LOW, MID, HIGH };
  enum class BloomOpt  : uint32_t { OFF, LOW, MID, HIGH };

  WindowOpt m_WindowOpt = WindowOpt::WINDOWED;
  ShadowOpt m_ShadowOpt = ShadowOpt::MID;
  BloomOpt  m_BloomOpt  = BloomOpt::LOW;

  static Config* s_Instance;
};

