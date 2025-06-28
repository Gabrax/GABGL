#include "FontManager.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <glad/glad.h>
#include <filesystem>
#include <string>

struct FontData
{
  FT_Library ft;

  std::unordered_map<std::string, Font> m_Fonts;

} s_Data;

void FontManager::Init()
{
  GABGL_ASSERT(!FT_Init_FreeType(&s_Data.ft), "Could not init FreeType");
  
  LoadFont("res/fonts/dpcomic.ttf");
}

void FontManager::LoadFont(const char* path)
{
  Timer timer;

  FT_Face face;
  if (FT_New_Face(s_Data.ft, path, 0, &face)) GABGL_ERROR("Failed to load font");
  else
  {
    FT_Set_Pixel_Sizes(face, 0, 48);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    Font font;

    for (unsigned char c = 0; c < 128; c++)
    {
      if (FT_Load_Char(face, c, FT_LOAD_RENDER)) { GABGL_ERROR("ERROR::FREETYPE: Failed to load Glyph"); continue; }

      GLuint texture;
      glGenTextures(1, &texture);
      glBindTexture(GL_TEXTURE_2D, texture);
      glTexImage2D(GL_TEXTURE_2D,0,GL_RED,face->glyph->bitmap.width,face->glyph->bitmap.rows,0,GL_RED,GL_UNSIGNED_BYTE,face->glyph->bitmap.buffer);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      GLint swizzleMask[] = {GL_ONE, GL_ONE, GL_ONE, GL_RED};
      glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

      Character character = {
          texture,
          { face->glyph->bitmap.width, face->glyph->bitmap.rows },
          { face->glyph->bitmap_left, face->glyph->bitmap_top },
          static_cast<uint32_t>(face->glyph->advance.x)
      };

      font.m_Characters.insert({c, character});
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    FT_Done_Face(face);
    FT_Done_FreeType(s_Data.ft);

    std::string name = std::filesystem::path(path).stem().string();
    s_Data.m_Fonts[name] = font;

    GABGL_WARN("Font uploading took {0} ms", timer.ElapsedMillis());
  }
}

Font* FontManager::GetFont(const char* name)
{
  auto it = s_Data.m_Fonts.find(name);
  if (it != s_Data.m_Fonts.end())
      return &it->second;
  return nullptr;
}
