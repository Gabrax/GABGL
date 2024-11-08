#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "OBJ/OBJloader.h"
#include "Util.h"
#include "DAE/Animator.h"
#include "DAE/DAEloader.h"

struct LoadOBJ{
    LoadOBJ(const char* modelpath) : loadmodel(modelpath) {
        puts("OBJ loaded");
    }

    ~LoadOBJ() = default;

    inline void Render(const glm::vec3& position, const glm::vec3& scale = glm::vec3(1.0f), const glm::vec3& lightPos = glm::vec3(0.0f),const float rotation = 0.0f){
        _shader.Use();
        _shader.setVec3("light.position", lightPos);
        _shader.setVec3("viewPos", this->camera.Position);

        // light properties
        _shader.setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
        _shader.setVec3("light.diffuse", 0.5f, 0.5f, 0.5f);
        _shader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);
        _shader.setFloat("light.constant", 1.0f);
        _shader.setFloat("light.linear", 0.09f);
        _shader.setFloat("light.quadratic", 0.032f);

        // material properties
        _shader.setFloat("material.shininess", 5.0f);
        
        glm::mat4 projection = glm::perspective(glm::radians(this->camera.Zoom), Window::_aspectRatio, 0.1f, 100.0f);
        _shader.setMat4("projection", projection);

        _shader.setMat4("view", this->camera.GetViewMatrix());
        glm::mat4 model = glm::mat4(1.0f); 
        model = glm::translate(model, position);
        model = glm::scale(model, scale);  
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));
        _shader.setMat4("model", model);
        loadmodel.Draw(_shader);
    }

private:
    Camera& camera = Window::_camera;
    Shader& _shader = g_shaders.model;
    OBJ loadmodel;
};

struct LoadDAE {
    LoadDAE(const char* modelpath) 
        : loadmodel(modelpath), 
          danceAnimation(modelpath, &loadmodel),
          animator(&danceAnimation) {
            puts("DAE loaded");
          }

    ~LoadDAE() = default;

    inline void Render(const glm::vec3& position, const glm::vec3& scale = glm::vec3(1.0f), const glm::vec3& lightPos = glm::vec3(0.0f), const float rotation = 0.0f) {
        animator.UpdateAnimation(Window::_deltaTime);
        
        _shader.Use();
        _shader.setVec3("light.position", lightPos);
        _shader.setVec3("viewPos", this->camera.Position);

        // light properties
        _shader.setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
        _shader.setVec3("light.diffuse", 0.5f, 0.5f, 0.5f);
        _shader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);
        _shader.setFloat("light.constant", 1.0f);
        _shader.setFloat("light.linear", 0.09f);
        _shader.setFloat("light.quadratic", 0.032f);

        // material properties
        _shader.setFloat("material.shininess", 5.0f);

        glm::mat4 projection = glm::perspective(glm::radians(this->camera.Zoom), Window::_aspectRatio, 0.1f, 100.0f);
        _shader.setMat4("projection", projection);

        glm::mat4 view = this->camera.GetViewMatrix();
        _shader.setMat4("view", view);

        auto transforms = animator.GetFinalBoneMatrices();
        for (size_t i = 0; i < transforms.size(); ++i) {
            _shader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);
        }
        glm::mat4 model = glm::mat4(1.0f); 
        model = glm::translate(model, position);
        model = glm::scale(model, scale);  
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));
        _shader.setMat4("model", model);
        loadmodel.Draw(_shader);
    }

private:
    Camera& camera = Window::_camera;
    Shader& _shader = g_shaders.animated;
    DAE loadmodel;
    Animation danceAnimation;
    Animator animator;
};
