#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../Input/Input.h"
#include "../Renderer.h"
#include "AnimationSystem.h"
#include "AnimatedMesh.h"
#include "../Window.h"


struct AnimatedModel {

    AnimatedModel(const char* modelpath) : loadmodel(modelpath), animation(modelpath, &loadmodel) 
    {
      const char* filename = strrchr(modelpath, '/');
      filename = (filename == nullptr) ? modelpath : filename + 1;

      std::cout << filename << " loaded" << '\n';
    }

    ~AnimatedModel() noexcept = default;

    void Render(const glm::vec3& position, const glm::vec3& scale = glm::vec3(1.0f), const float rotation = 0.0f)
    {
        animation.UpdateAnimation(Window::getDeltaTime());

        if(Input::KeyPressed(KEY_1)) animation.SetAnimationByName("WALK");

        _shader.Use();
        _shader.setVec3("viewPos", this->camera.Position);

        // light properties
        _shader.setFloat("light.constant", 1.0f);
        _shader.setFloat("light.linear", 0.09f);
        _shader.setFloat("light.quadratic", 0.032f);

        // material properties
        _shader.setFloat("material.shininess", 5.0f);

        glm::mat4 projection = glm::perspective(glm::radians(this->camera.Zoom), Window::getAspectRatio(), 0.1f, 100.0f);
        _shader.setMat4("projection", projection);

        glm::mat4 view = this->camera.GetViewMatrix();
        _shader.setMat4("view", view);

        auto transforms = animation.GetFinalBoneMatrices();
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
    Shader& _shader = Renderer::g_shaders.animated;
    AnimatedMesh loadmodel;
    AnimationSystem animation;
};
