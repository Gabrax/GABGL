#include "Config.h"
#include <fstream>
#include <iostream>
#include "json.hpp"

using json = nlohmann::json;

Config* Config::s_Instance = nullptr;

Config::Config()
{
  this->s_Instance = this;

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

void Config::SetDefaults()
{
  m_Version = CONFIG_VERSION;
  m_WindowWidth  = 1280;
  m_WindowHeight = 720;
  m_RenderWidth  = 1280;
  m_RenderHeight = 720;
  m_VSYNC = true;
  m_FPS   = 60;
  m_WindowOpt = WindowOpt::WINDOWED;
  m_ShadowOpt = ShadowOpt::MID;
  m_BloomOpt  = BloomOpt::LOW;
}

void Config::Save()
{
  json j;

  j["version"] = CONFIG_VERSION;

  j["window"] = {
    {"width", m_WindowWidth},
    {"height", m_WindowHeight},
    {"mode", (uint32_t)m_WindowOpt}
  };

  j["render"] = {
    {"width", m_RenderWidth},
    {"height", m_RenderHeight}
  };

  j["graphics"] = {
    {"vsync", m_VSYNC},
    {"fps", m_FPS},
    {"shadows", (uint32_t)m_ShadowOpt},
    {"bloom", (uint32_t)m_BloomOpt}
  };

  std::ofstream file(CONFIG_FILE);
  file << j.dump(4);

  m_LastWriteTime = std::filesystem::last_write_time(CONFIG_FILE);
}

void Config::Load()
{
  try
  {
    std::ifstream file(CONFIG_FILE);
    json j;
    file >> j;

    m_Version = j.value("version", 0);

    if (m_Version != CONFIG_VERSION)
    {
      std::cout << "Config version mismatch. Resetting.\n";
      SetDefaults();
      Save();
      return;
    }

    m_WindowWidth  = j["window"]["width"];
    m_WindowHeight = j["window"]["height"];
    m_WindowOpt    = (WindowOpt)j["window"]["mode"];

    m_RenderWidth  = j["render"]["width"];
    m_RenderHeight = j["render"]["height"];

    m_VSYNC     = j["graphics"]["vsync"];
    m_FPS       = j["graphics"]["fps"];
    m_ShadowOpt = (ShadowOpt)j["graphics"]["shadows"];
    m_BloomOpt  = (BloomOpt)j["graphics"]["bloom"];

    m_LastWriteTime = std::filesystem::last_write_time(CONFIG_FILE);
  }
  catch (...)
  {
    std::cout << "Restoring defaults\n";
    SetDefaults();
    Save();
  }
}

void Config::ReloadIfChanged()
{
  if (!std::filesystem::exists(CONFIG_FILE))
      return;

  auto currentWriteTime = std::filesystem::last_write_time(CONFIG_FILE);

  if (currentWriteTime != m_LastWriteTime)
  {
    std::cout << "Config reloaded\n";
    Load();
  }
}

void Config::ForceReload()
{
  Load();
}
