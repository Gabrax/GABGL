#pragma once

#include "Shader.h"

#include <ft2build.h>
#include FT_FREETYPE_H  
#include <map>
#include "BackendLogger.h"

struct Text
{
  Text();
  void Init();

  void RenderText(std::string text, float x, float y, float scale, glm::vec3 color);

  struct Character {
    uint32_t TextureID; // ID handle of the glyph texture
    glm::ivec2   Size;      // Size of glyph
    glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
    uint32_t Advance;   // Horizontal offset to advance to next glyph
  };
  
	std::shared_ptr<Shader> m_TextShader;
  std::map<GLchar, Character> m_Characters;
  glm::mat4 projection;
  unsigned int m_VAO, m_VBO;
};
