#include "FontManager.h"

#include "RenderBackend.h"
#include "Timer.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <ranges>
#include <string>
#include <vector>

namespace
{
  constexpr uint32_t FontAtlasWidth = 1024;
  constexpr uint32_t FontAtlasHeight = 512;
  constexpr uint32_t GlyphPadding = 2;
}

struct FontData
{
  FT_Library ft = nullptr;
  std::unordered_map<std::string, Font> m_Fonts;
  bool Initialized = false;
} s_Data;

void FontManager::Init()
{
  if (s_Data.Initialized) return;
  if (FT_Init_FreeType(&s_Data.ft))
  {
    gablog_log(LOG_ASSERT, __FILE__, __LINE__, "Could not init FreeType");
    gabdebug_break();
  }
  s_Data.Initialized = true;

  std::filesystem::path defaultFont = "../res/fonts/dpcomic.ttf";
  if (!std::filesystem::exists(defaultFont))
    defaultFont = "res/fonts/dpcomic.ttf";
  LoadFont(defaultFont.string().c_str());
}

void FontManager::Shutdown()
{
  if (!s_Data.Initialized) return;
  for (auto &font: s_Data.m_Fonts | std::views::values)
  {
    if (font.m_AtlasHandle != 0)
      RenderBackend::Get().DestroyFontAtlas(font.m_AtlasHandle);
    font.m_AtlasHandle = 0;
    font.m_Characters.clear();
  }
  s_Data.m_Fonts.clear();

  if (s_Data.ft)
  {
    FT_Done_FreeType(s_Data.ft);
    s_Data.ft = nullptr;
  }
  s_Data.Initialized = false;
}

void FontManager::LoadFont(const char* path)
{
  if (!s_Data.Initialized || !path) return;
  Timer timer;

  FT_Face face = nullptr;
  if (FT_New_Face(s_Data.ft, path, 0, &face)) {
    gablog_log(LOG_ERROR, __FILE__, __LINE__, "Failed to load font: %s", path);
    return;
  }

  FT_Set_Pixel_Sizes(face, 0, 48);

  Font font;
  font.m_Ascender = static_cast<float>(face->size->metrics.ascender) / 64.0f;
  font.m_Descender = static_cast<float>(-face->size->metrics.descender) / 64.0f;
  font.m_LineHeight = static_cast<float>(face->size->metrics.height) / 64.0f;

  std::vector<uint8_t> atlas(
    static_cast<size_t>(FontAtlasWidth) * FontAtlasHeight, 0);
  // Every backend can use this texel for solid-color UI geometry.
  atlas[0] = 255;
  uint32_t cursorX = GlyphPadding;
  uint32_t cursorY = GlyphPadding;
  uint32_t rowHeight = 0;

  for (unsigned char c = 0; c < 128; c++)
  {
    if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
      gablog_log(LOG_ERROR, __FILE__, __LINE__, "ERROR::FREETYPE: Failed to load Glyph");
      continue;
    }

    const FT_Bitmap& bitmap = face->glyph->bitmap;
    if (bitmap.width > 0 && bitmap.rows > 0)
    {
      if (cursorX + bitmap.width + GlyphPadding >= FontAtlasWidth)
      {
        cursorX = GlyphPadding;
        cursorY += rowHeight + GlyphPadding;
        rowHeight = 0;
      }
      if (cursorY + bitmap.rows + GlyphPadding >= FontAtlasHeight)
      {
        gablog_log(LOG_ERROR, __FILE__, __LINE__, "Font atlas is full while loading: %s", path);
        break;
      }

      const int pitch = bitmap.pitch;
      for (uint32_t row = 0; row < bitmap.rows; ++row)
      {
        const uint32_t sourceRow = pitch >= 0 ? row : bitmap.rows - 1u - row;
        std::memcpy(
          atlas.data() + static_cast<size_t>(cursorY + row) * FontAtlasWidth + cursorX,
          bitmap.buffer + static_cast<size_t>(sourceRow) * std::abs(pitch),
          bitmap.width);
      }
    }

    Character character;
    character.Size = {static_cast<int>(bitmap.width), static_cast<int>(bitmap.rows)};
    character.Bearing = {face->glyph->bitmap_left, face->glyph->bitmap_top};
    character.Advance = static_cast<uint32_t>(face->glyph->advance.x);
    character.UVTopLeft = {
      static_cast<float>(cursorX) / FontAtlasWidth,
      static_cast<float>(cursorY) / FontAtlasHeight};
    character.UVBottomRight = {
      static_cast<float>(cursorX + bitmap.width) / FontAtlasWidth,
      static_cast<float>(cursorY + bitmap.rows) / FontAtlasHeight};

    font.m_Characters.insert({c, character});
    if (bitmap.width > 0 && bitmap.rows > 0)
    {
      cursorX += bitmap.width + GlyphPadding;
      rowHeight = std::max(rowHeight, bitmap.rows);
    }
  }

  FT_Done_Face(face);

  font.m_AtlasHandle = RenderBackend::Get().CreateFontAtlas(
    atlas.data(), FontAtlasWidth, FontAtlasHeight);
  if (font.m_AtlasHandle == 0)
  {
    gablog_log(LOG_ERROR, __FILE__, __LINE__, "Failed to upload font atlas: %s", path);
    return;
  }

  std::string name = std::filesystem::path(path).stem().string();
  if (auto existing = s_Data.m_Fonts.find(name); existing != s_Data.m_Fonts.end())
  {
    if (existing->second.m_AtlasHandle != 0)
      RenderBackend::Get().DestroyFontAtlas(existing->second.m_AtlasHandle);
    existing->second = std::move(font);
  }
  else
  {
    s_Data.m_Fonts.emplace(name, std::move(font));
  }

  gablog_log(LOG_WARN, __FILE__, __LINE__, "Font uploading took %.3f ms", timer.ElapsedMillis());
}

Font* FontManager::GetFont(const char* name)
{
  auto it = s_Data.m_Fonts.find(name);
  if (it != s_Data.m_Fonts.end())
      return &it->second;
  return nullptr;
}
