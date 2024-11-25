#pragma once


#include "Input/Input.h"
#include "Window.h"
#include "glad/glad.h"
#include "Renderer.h"

struct Framebuffer
{

  Framebuffer(){
    initQuad();
  }

  ~Framebuffer(){
    glDeleteVertexArrays(1, &_VAO);
    glDeleteBuffers(1, &_VBO);
    glDeleteFramebuffers(1, &_FBO);
    glDeleteRenderbuffers(1, &_RBO);
    glDeleteTextures(1, &_texture);
  }

  void initQuad(){

    glGenFramebuffers(1, &_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, _FBO);

    glGenTextures(1, &_texture);
    glBindTexture(GL_TEXTURE_2D, _texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Window::GetWindowWidth(), Window::GetWindowHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texture, 0);


     glGenTextures(1, &_RBO);
    glBindTexture(GL_TEXTURE_2D, _RBO);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, Window::GetWindowWidth(), Window::GetWindowHeight(), 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, _RBO, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << '\n';
    }
  }

  void render(){

    glDisable(GL_DEPTH_TEST); 
    _shader.Use();  

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _texture);
    _shader.setInt("screenTexture", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _RBO);
    _shader.setInt("mainDepthTexture", 1);

    _shader.setFloat("renderWidth", Window::GetWindowWidth());
    _shader.setFloat("renderHeight", Window::GetWindowHeight());

    renderQuad();

    glEnable(GL_DEPTH_TEST);

    if(Input::KeyPressed(KEY_F)){
      resize(Window::GetWindowWidth(),Window::GetWindowHeight());
    }
  }

  GLuint getTexture(){
    return _texture;
  }
  void Bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, _FBO);
  }
  void UnBind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

private:

  void resize(int newWidth, int newHeight) {
    glBindFramebuffer(GL_FRAMEBUFFER, _FBO);

    // Resize the color texture
    glBindTexture(GL_TEXTURE_2D, _texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, newWidth, newHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    // Resize the depth-stencil texture
    glBindTexture(GL_TEXTURE_2D, _RBO);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, newWidth, newHeight, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

    // Check framebuffer completeness after resizing
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete after resizing!" << '\n';
    }

    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  Shader& _shader = Renderer::g_shaders.mainFB;
  GLuint _VAO = 0, _VBO, _FBO, _RBO, _texture;
  void renderQuad()
  {
    if (_VAO == 0)
    {
        GLfloat quadVertices[20] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &_VAO);
        glGenBuffers(1, &_VBO);
        glBindVertexArray(_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, _VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(_VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
  }


};
