#pragma once

#include <cstdint>
#include <unordered_map>
#include "Logger.h"
#include "glm/glm.hpp"

struct Character
{
  glm::ivec2 Size{0};
  glm::ivec2 Bearing{0};
  uint32_t Advance = 0;
  glm::vec2 UVTopLeft{0.0f};
  glm::vec2 UVBottomRight{0.0f};
};

struct Font
{
  std::unordered_map<char, Character> m_Characters;
  uint64_t m_AtlasHandle = 0;
  float m_Ascender = 0.0f;
  float m_Descender = 0.0f;
  float m_LineHeight = 0.0f;
};

struct FontManager
{
  static void Init();
  static void Shutdown();
  static void LoadFont(const char* path);
  static Font* GetFont(const char* name);
};
