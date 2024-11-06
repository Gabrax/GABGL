#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "OBJ/OBJloader.h"
#include "Window.h"
#include "Util.h"
#include "DAE/Animator.h"
#include "DAE/DAEloader.h"

struct LoadOBJ{
    LoadOBJ(const char* modelpath) : loadmodel(modelpath) {}

    ~LoadOBJ() = default;

    inline void SetupCameraUniforms(Camera& camera, float aspectRatio){
        _shader.Use();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), aspectRatio, 0.1f, 100.0f);
        _shader.setMat4("projection", projection);

        glm::mat4 view = camera.GetViewMatrix();
        _shader.setMat4("view", view);
    }

    inline void Render(Camera& camera, const glm::vec3& position, const glm::vec3& scale = glm::vec3(1.0f)){
        glm::mat4 model = glm::mat4(1.0f); 
        model = glm::translate(model, position);
        model = glm::scale(model, scale);  
        //float angle = 20.0f * (glfwGetTime() * 5.0f);
        //model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
        _shader.setMat4("model", model);
        loadmodel.Draw(_shader);

    }

private:
    Shader& _shader = g_shaders.model;
    OBJ loadmodel;
};

struct LoadDAE {
    LoadDAE(const char* modelpath) 
        : loadmodel(modelpath), 
          danceAnimation(modelpath, &loadmodel),
          animator(&danceAnimation),
          _shader(g_shaders.animated) {}

    ~LoadDAE() = default;

    inline void SetupCameraUniforms(Camera& camera, float aspectRatio, float deltaTime) {
        animator.UpdateAnimation(deltaTime);

        _shader.Use();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), aspectRatio, 0.1f, 100.0f);
        _shader.setMat4("projection", projection);

        glm::mat4 view = camera.GetViewMatrix();
        _shader.setMat4("view", view);

        auto transforms = animator.GetFinalBoneMatrices();
        for (size_t i = 0; i < transforms.size(); ++i) {
            _shader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);
        }
    }

    inline void Render(Camera& camera, const glm::vec3& position, const glm::vec3& scale = glm::vec3(1.0f)) {
        glm::mat4 model = glm::mat4(1.0f); 
        model = glm::translate(model, position);
        model = glm::scale(model, scale);  
        _shader.setMat4("model", model);
        loadmodel.Draw(_shader);
    }

private:
    Shader& _shader;
    DAE loadmodel;
    Animation danceAnimation;
    Animator animator;
};
