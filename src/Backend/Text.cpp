#include "Text.h"
#include "BackendLogger.h"

Text::Text()
{
  m_TextShader = Shader::Create("res/shaders/text_shader.glsl");
  projection = glm::ortho(0.0f, static_cast<float>(1000), 0.0f, static_cast<float>(600));
    m_TextShader->Use();
  m_TextShader->setMat4("projection", projection);
  Init();
}

void Text::Init()
{
  FT_Library ft;
  // All functions return a value different than 0 whenever an error occurred
  GABGL_ASSERT(!FT_Init_FreeType(&ft), "Could not init FreeType Library");

  // load font as face
  FT_Face face;
  if (FT_New_Face(ft, "res/fonts/dpcomic.ttf", 0, &face)) {
    GABGL_ERROR("Failed to load font");
  }
  else {
      // set size to load glyphs as
      FT_Set_Pixel_Sizes(face, 0, 48);

      // disable byte-alignment restriction
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

      // load first 128 characters of ASCII set
      for (unsigned char c = 0; c < 128; c++)
      {
          // Load character glyph 
          if (FT_Load_Char(face, c, FT_LOAD_RENDER))
          {
              std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
              continue;
          }
          // generate texture
          unsigned int texture;
          glGenTextures(1, &texture);
          glBindTexture(GL_TEXTURE_2D, texture);
          glTexImage2D(
              GL_TEXTURE_2D,
              0,
              GL_RED,
              face->glyph->bitmap.width,
              face->glyph->bitmap.rows,
              0,
              GL_RED,
              GL_UNSIGNED_BYTE,
              face->glyph->bitmap.buffer
          );
          // set texture options
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
          // now store character for later use
          Character character = {
              texture,
              glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
              glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
              static_cast<unsigned int>(face->glyph->advance.x)
          };
          m_Characters.insert(std::pair<char, Character>(c, character));
      }
      glBindTexture(GL_TEXTURE_2D, 0);
  }
  // destroy FreeType once we're finished
  FT_Done_Face(face);
  FT_Done_FreeType(ft);

  
  // configure VAO/VBO for texture quads
  // -----------------------------------
  glGenVertexArrays(1, &m_VAO);
  glGenBuffers(1, &m_VBO);
  glBindVertexArray(m_VAO);
  glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

}

void Text::RenderText(std::string text, float x, float y, float scale, glm::vec3 color)
{
    // activate corresponding render state	
    m_TextShader->Use();
    glUniform3f(glGetUniformLocation(m_TextShader->getID(), "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(m_VAO);

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) 
    {
        Character ch = m_Characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },            
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }           
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
