#include "Settings.h"
#include <cstdint>
#include <fstream>
#include "json.hpp"
#include "Logger.h"
#include <algorithm>

using json = nlohmann::json;

static constexpr const char* CONFIG_FILE = "gab.ini";

static std::filesystem::file_time_type m_LastWriteTime;

static uint32_t m_WindowWidth  = 1280;
static uint32_t m_WindowHeight = 720;
static uint32_t m_RenderWidth  = m_WindowWidth;
static uint32_t m_RenderHeight = m_WindowHeight;

enum class ShadowOpt : uint32_t { OFF, LOW, MID, HIGH };
enum class BloomOpt  : uint32_t { OFF, LOW, MID, HIGH };

static WindowMode m_WindowMode = WindowMode::Windowed;
static uint32_t m_FPSLimit = 60;
static bool m_VSync = false;
static float m_MusicVolume = 0.5f;
static float m_SFXVolume = 0.7f;
static ShadowOpt m_ShadowOpt = ShadowOpt::MID;
static BloomOpt  m_BloomOpt  = BloomOpt::LOW;

void Settings::Init()
{
  if (std::filesystem::exists(CONFIG_FILE))
  {
    Load();
  }
  else
  {
    SetDefaults();
    Save();
  }
}

void Settings::SetDefaults()
{
  m_WindowWidth  = 1280;
  m_WindowHeight = 720;
  m_RenderWidth  = 1280;
  m_RenderHeight = 720;
  m_WindowMode = WindowMode::Windowed;
  m_FPSLimit = 60;
  m_VSync = false;
  m_MusicVolume = 0.5f;
  m_SFXVolume = 0.7f;
  m_ShadowOpt = ShadowOpt::MID;
  m_BloomOpt  = BloomOpt::LOW;
}

void Settings::Save()
{
  json j;

  j["window"] = {
    {"width", m_WindowWidth},
    {"height", m_WindowHeight},
    {"mode", static_cast<uint32_t>(m_WindowMode)},
    {"fps_limit", m_FPSLimit},
    {"vsync", m_VSync}
  };

  j["audio"] = {
    {"music", m_MusicVolume},
    {"sfx", m_SFXVolume}
  };

  j["render"] = {
    {"width", m_RenderWidth},
    {"height", m_RenderHeight}
  };

  j["graphics"] = {
    {"shadows", (uint32_t)m_ShadowOpt},
    {"bloom", (uint32_t)m_BloomOpt}
  };

  std::ofstream file(CONFIG_FILE);
  file << j.dump(4);

  m_LastWriteTime = std::filesystem::last_write_time(CONFIG_FILE);
}

void Settings::Load()
{
  try
  {
    std::ifstream file(CONFIG_FILE);
    json j;
    file >> j;

    const auto& window = j["window"];
    m_WindowWidth  = window.value("width", 1280u);
    m_WindowHeight = window.value("height", 720u);

    if (window.contains("fps_limit"))
      m_FPSLimit = window.value("fps_limit", 60u);
    else
      m_FPSLimit = 60;
    m_VSync = window.value("vsync", false);

    const uint32_t storedMode = window.value("mode", 1u);
    if (window.contains("fps_limit"))
      m_WindowMode = static_cast<WindowMode>(std::min(storedMode, 2u));
    else
      m_WindowMode = storedMode == 0 ? WindowMode::Fullscreen
        : storedMode == 2 ? WindowMode::Borderless
        : WindowMode::Windowed;
    
    m_RenderWidth  = j["render"]["width"];
    m_RenderHeight = j["render"]["height"];

    m_ShadowOpt = (ShadowOpt)j["graphics"]["shadows"];
    m_BloomOpt  = (BloomOpt)j["graphics"]["bloom"];

    if (j.contains("audio"))
    {
      m_MusicVolume = std::clamp(j["audio"].value("music", 0.5f), 0.0f, 1.0f);
      m_SFXVolume = std::clamp(j["audio"].value("sfx", 0.7f), 0.0f, 1.0f);
    }

    m_LastWriteTime = std::filesystem::last_write_time(CONFIG_FILE);
  }
  catch (...)
  {
    GABGL_INFO("Restoring defaults");
    SetDefaults();
    Save();
  }
}

void Settings::ReloadIfChanged()
{
  if (!std::filesystem::exists(CONFIG_FILE))
      return;

  auto currentWriteTime = std::filesystem::last_write_time(CONFIG_FILE);

  if (currentWriteTime != m_LastWriteTime)
  {
    GABGL_INFO("Settings reloaded");
    Load();
  }
}

void Settings::ForceReload()
{
  Load();
}

uint32_t Settings::GetWindowWidth()
{
  return m_WindowWidth;
}

uint32_t Settings::GetWindowHeight()
{
  return m_WindowHeight;
}

void Settings::SetResolution(uint32_t width, uint32_t height)
{
  m_WindowWidth = width;
  m_WindowHeight = height;
}

WindowMode Settings::GetWindowMode() { return m_WindowMode; }
void Settings::SetWindowMode(WindowMode mode) { m_WindowMode = mode; }
uint32_t Settings::GetFPSLimit() { return m_FPSLimit; }
void Settings::SetFPSLimit(uint32_t fps) { m_FPSLimit = fps; }
bool Settings::GetVSync() { return m_VSync; }
void Settings::SetVSync(bool enabled) { m_VSync = enabled; }
float Settings::GetMusicVolume() { return m_MusicVolume; }
void Settings::SetMusicVolume(float volume) { m_MusicVolume = std::clamp(volume, 0.0f, 1.0f); }
float Settings::GetSFXVolume() { return m_SFXVolume; }
void Settings::SetSFXVolume(float volume) { m_SFXVolume = std::clamp(volume, 0.0f, 1.0f); }
