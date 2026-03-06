#include "Settings.h"
#include <cstdint>
#include <fstream>
#include "json.hpp"
#include "Logger.h"

using json = nlohmann::json;

static constexpr const char* CONFIG_FILE = "gab.ini";

static std::filesystem::file_time_type m_LastWriteTime;

static uint32_t m_WindowWidth  = 1280;
static uint32_t m_WindowHeight = 720;
static uint32_t m_RenderWidth  = m_WindowWidth;
static uint32_t m_RenderHeight = m_WindowHeight;

enum class WindowOpt : uint32_t { FULLSCREEN, WINDOWED, BORDERLESS };
enum class FPSOpt : uint32_t
{
    UNLIMITED = 0,
    VSYNC = 1,
    FPS_60    = 8,
    FPS_75    = 6,
    FPS_120   = 5,
    FPS_144   = 4,
    FPS_165   = 3,
    FPS_240   = 2,
    FPS_360   = 1
};
enum class ShadowOpt : uint32_t { OFF, LOW, MID, HIGH };
enum class BloomOpt  : uint32_t { OFF, LOW, MID, HIGH };

static WindowOpt m_WindowOpt = WindowOpt::WINDOWED;
static FPSOpt m_FPSOpt = FPSOpt::FPS_60;
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
  m_WindowOpt = WindowOpt::WINDOWED;
  m_ShadowOpt = ShadowOpt::MID;
  m_BloomOpt  = BloomOpt::LOW;
}

void Settings::Save()
{
  json j;

  j["window"] = {
    {"width", m_WindowWidth},
    {"height", m_WindowHeight},
    {"mode", (uint32_t)m_WindowOpt},
    {"fps",(uint32_t)m_FPSOpt}
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

    m_WindowWidth  = j["window"]["width"];
    m_WindowHeight = j["window"]["height"];
    m_WindowOpt    = (WindowOpt)j["window"]["mode"];
    m_FPSOpt = (FPSOpt)j["window"]["fps"];
    
    m_RenderWidth  = j["render"]["width"];
    m_RenderHeight = j["render"]["height"];

    m_ShadowOpt = (ShadowOpt)j["graphics"]["shadows"];
    m_BloomOpt  = (BloomOpt)j["graphics"]["bloom"];

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

uint32_t Settings::GetFPS()
{
  return static_cast<uint32_t>(m_FPSOpt);
}
