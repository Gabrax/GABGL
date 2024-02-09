#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "Renderer.h"
#include "Shader.h"
#include "stb_image.h"
#include "../Core/Window.h"
#include "../Core/Input.h"
#include "../Core/Camera.h"
#include "Model.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

//camera properties
Camera camera(glm::vec3(0.0f, 0.0f, 6.0f));
float deltaTime = 0.0f;	
float lastFrame = 0.0f;
float lastX = 800 / 2.0f;
float lastY = 600 / 2.0f;
bool firstMouse = true;

//glm::vec3 lightPos(0.0, 1.0f, -5.0f);



    float vertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    /*unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };*/

    glm::vec3 cubePositions[] = {
        glm::vec3(0.0f,  0.0f,  0.0f),
        glm::vec3(2.0f,  5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3(2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f,  3.0f, -7.5f),
        glm::vec3(1.3f, -2.0f, -2.5f),
        glm::vec3(1.5f,  2.0f, -2.5f),
        glm::vec3(1.5f,  0.2f, -1.5f),
        glm::vec3(-1.3f,  1.0f, -1.5f)
    };

    glm::vec3 pointLightPositions[] = {
        glm::vec3(0.7f,  0.2f,  2.0f),
        glm::vec3(2.3f, -3.3f, -4.0f),
        glm::vec3(-4.0f,  2.0f, -12.0f),
        glm::vec3(0.0f,  0.0f, -3.0f)
    };

void Renderer::Render()
{
    Window::ShowFPS();
    Shader Cube("res/shaders/Basic.vert", "res/shaders/Basic.frag");
    Shader Light("res/shaders/LightSource.vert", "res/shaders/LightSource.frag");
    Shader _model("res/shaders/Model.vert", "res/shaders/Model.frag");
    Model _Model("res/models/backpack/backpack.obj");
    Keyboard();

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    //glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    //glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    //glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    //light cube
    unsigned int lightVAO;
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);
    // we only need to bind to the VBO, the container's VBO's data already contains the data.
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // set the vertex attribute 
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);


    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    

    stbi_set_flip_vertically_on_load(true);
    unsigned int texture0, texture1, texture2;
    texture0 = loadTexture("res/textures/angel.jpg");
    texture1 = loadTexture("res/textures/container2_specular.png");
    texture2 = loadTexture("res/textures/matrix.jpg");



    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texture2);



    // cube model
    Cube.Use();
    Cube.setInt("material.diffuse", 0);
    Cube.setInt("material.specular", 1);
    Cube.setInt("material.emission", 2);
    glUniform1f(glGetUniformLocation(Cube.ID, "time"), glfwGetTime());
    
    
    
   // Cube.setVec3("light.position", lightPos);
    //Cube.setVec3("viewPos", camera.Position);
    Cube.setFloat("material.shininess", 50.0f);


    // directional light
    Cube.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
    Cube.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
    Cube.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
    Cube.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);
    // point light 1
    Cube.setVec3("pointLights[0].position", pointLightPositions[0]);
    Cube.setVec3("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
    Cube.setVec3("pointLights[0].diffuse", 0.0f, 1.0f, 1.0f);
    Cube.setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
    Cube.setFloat("pointLights[0].constant", 1.0f);
    Cube.setFloat("pointLights[0].linear", 0.09f);
    Cube.setFloat("pointLights[0].quadratic", 0.032f);
    // point light 2
    Cube.setVec3("pointLights[1].position", pointLightPositions[1]);
    Cube.setVec3("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
    Cube.setVec3("pointLights[1].diffuse", 0.0f, 1.0f, 0.0f);
    Cube.setVec3("pointLights[1].specular", 1.0f, 1.0f, 1.0f);
    Cube.setFloat("pointLights[1].constant", 1.0f);
    Cube.setFloat("pointLights[1].linear", 0.09f);
    Cube.setFloat("pointLights[1].quadratic", 0.032f);
    // point light 3
    Cube.setVec3("pointLights[2].position", pointLightPositions[2]);
    Cube.setVec3("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
    Cube.setVec3("pointLights[2].diffuse", 1.0f, 0.0f, 0.0f);
    Cube.setVec3("pointLights[2].specular", 1.0f, 1.0f, 1.0f);
    Cube.setFloat("pointLights[2].constant", 1.0f);
    Cube.setFloat("pointLights[2].linear", 0.09f);
    Cube.setFloat("pointLights[2].quadratic", 0.032f);
    // point light 4
    Cube.setVec3("pointLights[3].position", pointLightPositions[3]);
    Cube.setVec3("pointLights[3].ambient", 0.05f, 0.05f, 0.05f);
    Cube.setVec3("pointLights[3].diffuse", 0.8f, 0.8f, 0.8f);
    Cube.setVec3("pointLights[3].specular", 1.0f, 1.0f, 1.0f);
    Cube.setFloat("pointLights[3].constant", 1.0f);
    Cube.setFloat("pointLights[3].linear", 0.09f);
    Cube.setFloat("pointLights[3].quadratic", 0.032f);
    // spotLight
    Cube.setVec3("spotLight.position", camera.Position);
    Cube.setVec3("spotLight.direction", camera.Front);
    Cube.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
    Cube.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
    Cube.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
    Cube.setFloat("spotLight.constant", 1.0f);
    Cube.setFloat("spotLight.linear", 0.09f);
    Cube.setFloat("spotLight.quadratic", 0.032f);
    Cube.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
    Cube.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));



    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), 800.0f / 600.0f, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    Cube.setMat4("projection", projection);
    Cube.setMat4("view", view);

    //glUniform4f(vertexColorLocation, 0.0f, greenValue, 0.0f, 1.0f);
    //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT,0); // call this when indices specified
    glBindVertexArray(VAO);
    for (unsigned int i = 0; i < 10; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, cubePositions[i]);
        float angle = 20.0f * i;
        model = glm::rotate(model, (float)glfwGetTime() * glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
        Cube.setMat4("model", model);

        glDrawArrays(GL_TRIANGLES, 0, 36);
    }




    //light source
    //lightPos.z = 1.0f * sin(glfwGetTime()) * 2.0f;
    //lightPos.x = 5.0f * sin(glfwGetTime() / 2.0f) * 1.0f;
    Light.Use();
    Light.setMat4("projection", projection);
    Light.setMat4("view", view);

    glBindVertexArray(lightVAO);
    for (unsigned int i = 0; i < 4; i++)
    {
        glm::mat4 model2 = glm::mat4(1.0f);
        //pointLightPositions[i].z = 1.0f * sin(glfwGetTime()) * 2.0f;
        model2 = glm::translate(model2, pointLightPositions[i]);
        model2 = glm::scale(model2, glm::vec3(0.2f));
        Light.setMat4("model", model2);
        glDrawArrays(GL_TRIANGLES, 0, 36);


    }


   /*//model
    _model.Use();
    _model.setMat4("projection", projection);
    _model.setMat4("view", view);
    glm::mat4 model3 = glm::mat4(1.0f);
    model3 = glm::translate(model3, glm::vec3(0.0f, 0.0f, 3.0f));
    model3 = glm::scale(model3, glm::vec3(0.2f, 0.2f, 0.2f));
    _model.setMat4("model", model3);
    _Model.Draw(_model);*/


   glDeleteVertexArrays(1, &VAO);
   glDeleteBuffers(1, &VBO);
   glDeleteBuffers(1, &EBO);

}

void Load()
{

}



unsigned int Renderer::loadTexture(const char* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void Renderer::Keyboard()
{
    if (Input::KeyDown(GLFW_KEY_W))
    {
        camera.ProcessKeyboard(FORWARD, deltaTime);
    }
    if (Input::KeyDown(GLFW_KEY_S))
    {
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    }
    if (Input::KeyDown(GLFW_KEY_A))
    {
        camera.ProcessKeyboard(LEFT, deltaTime);
    }
    if (Input::KeyDown(GLFW_KEY_D))
    {
        camera.ProcessKeyboard(RIGHT, deltaTime);
    }
    if (Input::KeyDown(GLFW_KEY_SPACE))
    {
        camera.ProcessKeyboard(UP, deltaTime);
    }
    if (Input::KeyDown(GLFW_KEY_LEFT_SHIFT))
    {
        camera.ProcessKeyboard(DOWN, deltaTime);
    }
}

void Renderer::mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
   

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void Renderer::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
