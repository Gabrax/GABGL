#include "Engine.h"
#include "Core/Window.h"
#include "Core/Input.h"
//#include "Renderer/Renderer.h"

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "stb_image.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <freetype/freetype.h>
#include FT_FREETYPE_H
#include "Renderer/Shader.h"
#include "Core/Camera.h"
#include "Renderer/Model.h"
#include "Core/Audio.h"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>


//camera properties
Camera camera(glm::vec3(0.0f, 0.0f, 6.0f));
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float lastX = 800 / 2.0f;
float lastY = 600 / 2.0f;
bool firstMouse = true;

//glm::vec3 lightPos(0.0, 1.0f, -5.0f);



void CameraMovement();
unsigned int loadTexture(const char* path);
unsigned int loadCubemap(vector<std::string> faces);
void RenderText(Shader& shader, std::string text, float x, float y, float scale, glm::vec3 color);

struct Character {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2   Size;      // Size of glyph
    glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Horizontal offset to advance to next glyph
};
std::map<GLchar, Character> Characters;


        unsigned int VBO, VAO, EBO,textVAO, textVBO;
        inline double prevTime = 0.0;
        inline double crntTime = 0.0;
        inline double timeDiff;
        unsigned int counter = 0;


void Engine::Run()
{	

    Window::Init(1920 * 1.5f, 1080 * 1.5f);
    Audio::Init();
    Audio::PlayAudio("nujabes.wav", 0.005f);
    glfwSwapInterval(0);
    

    glEnable(GL_DEPTH_TEST);
    

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

        std::vector<std::string> SkyboxFaces
        {
            "res/textures/SkyBox/right.jpg",
            "res/textures/SkyBox/left.jpg",
            "res/textures/SkyBox/top.jpg",
            "res/textures/SkyBox/bottom.jpg",
            "res/textures/SkyBox/front.jpg",
            "res/textures/SkyBox/back.jpg"
        };

        float skyboxVertices[] = {
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

        stbi_set_flip_vertically_on_load(false);
        unsigned int Cubemap = loadCubemap(SkyboxFaces);
        stbi_set_flip_vertically_on_load(true);

    Shader Cube("res/shaders/Basic.vert", "res/shaders/Basic.frag");
    Shader Light("res/shaders/LightSource.vert", "res/shaders/LightSource.frag");
    Shader CubeMap("res/shaders/CubeMap.vert", "res/shaders/CubeMap.frag");
    Shader _BPshader("res/shaders/Model.vert", "res/shaders/Model.frag","res/shaders/Model.geom");
    Shader _text("res/shaders/Text.vert", "res/shaders/Text.frag");
    Shader _asteroidsShader("res/shaders/asteroids.vert", "res/shaders/asteroids.frag");
    Shader _planetShader("res/shaders/planet.vert", "res/shaders/planet.frag");


    Model _BPmodel("res/models/backpack/backpack.obj");
    Model _asteroids("res/models/asteroid/rock.obj");
    Model _planet("res/models/planet/planet.obj");

        // FreeType
        // --------
        FT_Library ft;
        // All functions return a value different than 0 whenever an error occurred
        if (FT_Init_FreeType(&ft))
        {
            std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;

        }

        // find path to font
        std::string font_name = "res/fonts/Antonio-Bold.ttf";
        if (font_name.empty())
        {
            std::cout << "ERROR::FREETYPE: Failed to load font_name" << std::endl;

        }

        // load font as face
        FT_Face face;
        if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
            std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;

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
                Characters.insert(std::pair<char, Character>(c, character));
            }
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        // destroy FreeType once we're finished
        FT_Done_Face(face);
        FT_Done_FreeType(ft);


        glGenVertexArrays(1, &textVAO);
        glGenBuffers(1, &textVBO);
        glBindVertexArray(textVAO);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        
        unsigned int texture0, texture1, texture2;
        texture0 = loadTexture("res/textures/angel.jpg");
        texture1 = loadTexture("res/textures/container2_specular.png");
        texture2 = loadTexture("res/textures/matrix.jpg");

        unsigned int amount = 100000;
        glm::mat4* modelMatrices;
        modelMatrices = new glm::mat4[amount];
        srand(static_cast<unsigned int>(glfwGetTime())); // initialize random seed
        float radius = 150.0;
        float offset = 25.0f;
        for (unsigned int i = 0; i < amount; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            // 1. translation: displace along circle with 'radius' in range [-offset, offset]
            float angle = (float)i / (float)amount * 360.0f;
            float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
            float x = sin(angle) * radius + displacement;
            displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
            float y = displacement * 0.4f; // keep height of asteroid field smaller compared to width of x and z
            displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
            float z = cos(angle) * radius + displacement;
            model = glm::translate(model, glm::vec3(x, y, z));

            // 2. scale: Scale between 0.05 and 0.25f
            float scale = static_cast<float>((rand() % 20) / 100.0 + 0.05);
            model = glm::scale(model, glm::vec3(scale));

            // 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
            float rotAngle = static_cast<float>((rand() % 360));
            model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

            // 4. now add to list of matrices
            modelMatrices[i] = model;
        }

        // configure instanced array
        // -------------------------
        unsigned int buffer;
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, amount * sizeof(glm::mat4), &modelMatrices[0], GL_STATIC_DRAW);

        // set transformation matrices as an instance vertex attribute (with divisor 1)
        // note: we're cheating a little by taking the, now publicly declared, VAO of the model's mesh(es) and adding new vertexAttribPointers
        // normally you'd want to do this in a more organized fashion, but for learning purposes this will do.
        // -----------------------------------------------------------------------------------------------------------------------------------
        for (unsigned int i = 0; i < _asteroids.meshes.size(); i++)
        {
            unsigned int VAO = _asteroids.meshes[i].VAO;
            glBindVertexArray(VAO);
            // set attribute pointers for matrix (4 times vec4)
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
            glEnableVertexAttribArray(5);
            glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
            glEnableVertexAttribArray(6);
            glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

            glVertexAttribDivisor(3, 1);
            glVertexAttribDivisor(4, 1);
            glVertexAttribDivisor(5, 1);
            glVertexAttribDivisor(6, 1);

            glBindVertexArray(0);
        }
        

	while (Window::WindowIsOpen() && Window::WindowHasNotBeenForceClosed())
	{
        Window::ShowFPS();
        CameraMovement();
        
       
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        //glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
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

        unsigned int skyboxVAO, skyboxVBO;
        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);
        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);



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
        Cube.setFloat("time", glfwGetTime());

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
		   

        // Cube.setVec3("light.position", lightPos);
        //Cube.Use();
        Cube.setVec3("viewPos", camera.Position);
        Cube.setFloat("material.shininess", 32.0f);


        // directional light
        //Cube.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        //Cube.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
        //Cube.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
        //Cube.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);
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
        //Cube.setVec3("spotLight.position", camera.Position);
        //Cube.setVec3("spotLight.direction", camera.Front);
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

        glm::mat4 model = glm::mat4(1.0f);
        Cube.setMat4("model", model);

        //glGenTextures(1, &textureID);
        //glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

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
        // cube model




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
            pointLightPositions[i].y = 1.0f * sin(glfwGetTime()) * 2.0f;
            model2 = glm::translate(model2, pointLightPositions[i]);
            model2 = glm::scale(model2, glm::vec3(0.2f));
            Light.setMat4("model", model2);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        //light source

          //model
         _BPshader.Use();
         glm::mat4 model3 = glm::mat4(1.0f);
         model3 = glm::translate(model3, glm::vec3(0.0f, 1.0f, 0.0f));
         model3 = glm::scale(model3, glm::vec3(0.2f, 0.2f, 0.2f));
         _BPshader.setMat4("projection", projection);
         _BPshader.setMat4("view", view);
         _BPshader.setMat4("model", model3);
         _BPshader.setFloat("time", glfwGetTime() * 0.5f);
         _BPmodel.Draw(_BPshader);
         //model

         //cube map
         glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
         CubeMap.Use();
         CubeMap.setInt("skybox", 0);
         view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
         CubeMap.setMat4("view", view);
         CubeMap.setMat4("projection", projection);
         glBindVertexArray(skyboxVAO);
         glActiveTexture(GL_TEXTURE0);
         glBindTexture(GL_TEXTURE_CUBE_MAP, Cubemap);
         glDrawArrays(GL_TRIANGLES, 0, 36);
         glBindVertexArray(0);
         glDepthFunc(GL_LESS); 
         //cube map

         //saturn wannabe
         _planetShader.Use();
         glm::mat4 Planetprojection = glm::perspective(glm::radians(45.0f), (float)800 / (float)600, 0.1f, 1000.0f);
         view = camera.GetViewMatrix();
         _planetShader.setMat4("view", view);
         _planetShader.setMat4("projection", Planetprojection);

         glm::mat4 model4 = glm::mat4(1.0f);
         model4 = glm::translate(model4, glm::vec3(0.0f, -3.0f, 0.0f));
         model4 = glm::scale(model4, glm::vec3(1.0f, 1.0f, 1.0f));
         _planetShader.setMat4("model", model4);
         _planet.Draw(_planetShader);
         //saturn wannabe

         //asteroids
         _asteroidsShader.Use();
         _asteroidsShader.setMat4("view", view);
         _asteroidsShader.setMat4("projection", Planetprojection);
         _asteroidsShader.setInt("texture_diffuse1", 0);
         glActiveTexture(GL_TEXTURE0);
         glBindTexture(GL_TEXTURE_2D, _asteroids.textures_loaded[0].id);
         for (unsigned int i = 0; i < _asteroids.meshes.size(); i++)
         {
             glBindVertexArray(_asteroids.meshes[i].VAO);
             glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(_asteroids.meshes[i].indices.size()), GL_UNSIGNED_INT, 0, amount);
             glBindVertexArray(0);
         }
         //asteroids


         
         //text rendering
         glEnable(GL_CULL_FACE);
         glEnable(GL_BLEND);
         glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
         glDisable(GL_DEPTH_TEST);
         _text.Use();
         projection = glm::ortho(0.0f, static_cast<float>(800.0f), 0.0f, static_cast<float>(600.0f));
         glUniformMatrix4fv(glGetUniformLocation(_text.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

         crntTime = glfwGetTime();
         timeDiff = crntTime - prevTime;
         counter++;
         if (timeDiff >= 1.0 / 6000.0)
         {
             std::string FPS = std::to_string((1.0 / timeDiff) * counter);
             RenderText(_text, "FPS :" + FPS, 25.0f, 500.0f, 0.5f, glm::vec3(0.0f, 0.0f, 0.0f));
             prevTime = crntTime;
             counter = 0;
         }
         RenderText(_text, "suck deez nuts", 650.0f, 570.0f, 0.5f, glm::vec3(0.0f, 0.0f, 0.0f));
         glEnable(GL_DEPTH_TEST);
         glDisable(GL_CULL_FACE);
         //text rendering

		


		if (Input::KeyPressed(GLFW_KEY_F))
		{
			Window::ToggleFullscreen();
            Audio::PlayAudio("SELECT.wav", 0.5f);
		}
		if (Input::KeyPressed(GLFW_KEY_H))
		{
			Window::ToggleWireframe();
            Audio::PlayAudio("RE_Beep.wav", 0.05f);
		}
        
		


		Window::ProcessInput();
		Input::Update();
		Window::SwapBuffersPollEvents();
        Audio::Update();
		

        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        //glDeleteBuffers(1, &EBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
    
	
}


void RenderText(Shader& shader, std::string text, float x, float y, float scale, glm::vec3 color)
{
    // activate corresponding render state	
    shader.Use();
    glUniform3f(glGetUniformLocation(shader.ID, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

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
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
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

unsigned int loadTexture(const char* path)
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

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void CameraMovement()
{
    if (Input::KeyDown(GAB_KEY_W))
    {
        camera.ProcessKeyboard(FORWARD, deltaTime);
    }
    if (Input::KeyDown(GAB_KEY_S))
    {
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    }
    if (Input::KeyDown(GAB_KEY_A))
    {
        camera.ProcessKeyboard(LEFT, deltaTime);
    }
    if (Input::KeyDown(GAB_KEY_D))
    {
        camera.ProcessKeyboard(RIGHT, deltaTime);
    }
    if (Input::KeyDown(GAB_KEY_SPACE))
    {
        camera.ProcessKeyboard(UP, deltaTime);
    }
    if (Input::KeyDown(GAB_KEY_LEFT_SHIFT))
    {
        camera.ProcessKeyboard(DOWN, deltaTime);
    }
}

void Engine::mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
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

void Engine::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}