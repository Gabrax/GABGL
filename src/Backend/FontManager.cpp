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

void FontManager::Shutdown()
{
  FT_Done_FreeType(s_Data.ft); 
}

void FontManager::LoadFont(const char* path)
{
  Timer timer;

  FT_Face face;
  if (FT_New_Face(s_Data.ft, path, 0, &face)) {
    GABGL_ERROR("Failed to load font");
    return;
  }

  FT_Set_Pixel_Sizes(face, 0, 48);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // still global state

  Font font;

  for (unsigned char c = 0; c < 128; c++)
  {
    if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
      GABGL_ERROR("ERROR::FREETYPE: Failed to load Glyph");
      continue;
    }

    GLuint texture;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);
    glTextureStorage2D(texture, 1, GL_R8, face->glyph->bitmap.width, face->glyph->bitmap.rows);
    glTextureSubImage2D(texture, 0, 0, 0,
                        face->glyph->bitmap.width,
                        face->glyph->bitmap.rows,
                        GL_RED, GL_UNSIGNED_BYTE,
                        face->glyph->bitmap.buffer);

    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLint swizzleMask[] = { GL_ONE, GL_ONE, GL_ONE, GL_RED };
    glTextureParameteriv(texture, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

    Character character = {
        texture,
        { face->glyph->bitmap.width, face->glyph->bitmap.rows },
        { face->glyph->bitmap_left, face->glyph->bitmap_top },
        static_cast<uint32_t>(face->glyph->advance.x)
    };

    font.m_Characters.insert({c, character});
  }

  FT_Done_Face(face);

  std::string name = std::filesystem::path(path).stem().string();
  s_Data.m_Fonts[name] = font;

  GABGL_WARN("Font uploading took {0} ms", timer.ElapsedMillis());
}

Font* FontManager::GetFont(const char* name)
{
  auto it = s_Data.m_Fonts.find(name);
  if (it != s_Data.m_Fonts.end())
      return &it->second;
  return nullptr;
}
