#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "stb_image.h"
#include <array>
#include <string>

struct Skybox {

    Skybox() = default;
    ~Skybox();
    void Bake();
    void Render();
private:

    GLuint _VBO, _VAO;
    GLuint _texture;
    // loads a cubemap texture from 6 individual texture faces
    // order:
    // +X (right)
    // -X (left)
    // +Y (top)
    // -Y (bottom)
    // +Z (front) 
    // -Z (back)
    // -------------------------------------------------------
    uint32_t loadCubemap(const std::array<std::string,6>& faces);

};

