#pragma once

#include <unordered_map>
#include "Logger.h"
#include "glm/glm.hpp"

struct Character
{
  uint32_t TextureID;
  glm::ivec2 Size;
  glm::ivec2 Bearing;
  uint32_t Advance;
};

struct Font
{
  std::unordered_map<char, Character> m_Characters;
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
