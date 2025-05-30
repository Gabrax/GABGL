#include "Skybox.h"


namespace SkyboxVerts
{
  GLfloat vertices[108] = {
      // positions          
      -1.0f,  1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f,
       1.0f, -1.0f, -1.0f,
       1.0f, -1.0f, -1.0f,
       1.0f,  1.0f, -1.0f,
      -1.0f,  1.0f, -1.0f,

      -1.0f, -1.0f,  1.0f,
      -1.0f, -1.0f, -1.0f,
      -1.0f,  1.0f, -1.0f,
      -1.0f,  1.0f, -1.0f,
      -1.0f,  1.0f,  1.0f,
      -1.0f, -1.0f,  1.0f,

       1.0f, -1.0f, -1.0f,
       1.0f, -1.0f,  1.0f,
       1.0f,  1.0f,  1.0f,
       1.0f,  1.0f,  1.0f,
       1.0f,  1.0f, -1.0f,
       1.0f, -1.0f, -1.0f,

      -1.0f, -1.0f,  1.0f,
      -1.0f,  1.0f,  1.0f,
       1.0f,  1.0f,  1.0f,
       1.0f,  1.0f,  1.0f,
       1.0f, -1.0f,  1.0f,
      -1.0f, -1.0f,  1.0f,

      -1.0f,  1.0f, -1.0f,
       1.0f,  1.0f, -1.0f,
       1.0f,  1.0f,  1.0f,
       1.0f,  1.0f,  1.0f,
      -1.0f,  1.0f,  1.0f,
      -1.0f,  1.0f, -1.0f,

      -1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f,  1.0f,
       1.0f, -1.0f, -1.0f,
       1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f,  1.0f,
       1.0f, -1.0f,  1.0f
  };
}

/*Skybox() = default;*/
/**/
/*~Skybox()*/
/*{*/
/*  glDeleteBuffers(1, &_VBO);*/
/*  glDeleteVertexArrays(1, &_VAO);*/
/*  glDeleteTextures(1, &_texture);*/
/*}*/
/**/
/*void Bake()*/
/*{*/
/*  glGenVertexArrays(1, &_VAO);*/
/*  glGenBuffers(1, &_VBO);*/
/*  glBindVertexArray(_VAO);*/
/*  glBindBuffer(GL_ARRAY_BUFFER, _VBO);*/
/*  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);*/
/*  glEnableVertexAttribArray(0);*/
/*  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);*/
/**/
/*  std::array<std::string,6> faces*/
/*  {*/
/*      "../res/skybox/NightSky_Right.png",*/
/*      "../res/skybox/NightSky_Left.png",*/
/*      "../res/skybox/NightSky_Top.png",*/
/*      "../res/skybox/NightSky_Bottom.png",*/
/*      "../res/skybox/NightSky_Front.png",*/
/*      "../res/skybox/NightSky_Back.png"*/
/*  };*/
/*  stbi_set_flip_vertically_on_load(false);*/
/*  _texture = loadCubemap(faces);*/
/*  stbi_set_flip_vertically_on_load(true);*/
/**/
/*  puts("EnvMap loaded");*/
/*}*/
/**/
/*void Render()*/
/*{*/
/*  glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content*/
/*  _shader.Use();*/
/*  glm::mat4 projection = glm::perspective(glm::radians(this->_camera.Zoom), Window::getAspectRatio(), 0.001f, 2000.0f);*/
/*  glm::mat4 view = glm::mat4(glm::mat3(this->_camera.GetViewMatrix())); // remove translation from the view matrix*/
/*  _shader.setMat4("view", view);*/
/*  _shader.setMat4("projection", projection);*/
/*  // skybox cube*/
/*  glBindVertexArray(_VAO);*/
/*  glActiveTexture(GL_TEXTURE0);*/
/*  glBindTexture(GL_TEXTURE_CUBE_MAP, _texture);*/
/*  glDrawArrays(GL_TRIANGLES, 0, 36);*/
/*  glBindVertexArray(0);*/
/*  glDepthFunc(GL_LESS); // set depth function back to default*/
/*}*/
/**/
/*unsigned int loadCubemap(std::array<std::string,6> faces) {*/
/*    unsigned int textureID;*/
/*    glGenTextures(1, &textureID);*/
/*    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);*/
/**/
/*    int width, height, nrChannels;*/
/*    for (unsigned int i = 0; i < faces.size(); i++) {*/
/*        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);*/
/*        if (data) {*/
/*            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;*/
/*            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);*/
/*            stbi_image_free(data);*/
/*        } else {*/
/*            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;*/
/*            stbi_image_free(data);*/
/*        }*/
/*    }*/
/**/
/*    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);*/
/*    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);*/
/*    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);*/
/*    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);*/
/*    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);*/
/*    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);*/
/**/
/*    return textureID;*/
/*}*/
/**/
