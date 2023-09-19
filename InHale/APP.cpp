#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h> //OpenGL
#include "stb_image.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Texture.h"
#include "Shaderclass.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "Camera.h"


unsigned int length = 800;
unsigned int width = 800;


int main()
{
	if (!glfwInit())
		return -1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLfloat vertices[] =
	{       
		
	    -0.5f,  0.0f,  0.5f,    0.83f,  0.70f,  0.44f,    0.0f, 0.0f,
		-0.5f,  0.0f, -0.5f,    0.83f,  0.70f,  0.44f,    5.0f, 0.0f,
		 0.5f,  0.0f, -0.5f,    0.83f,  0.70f,  0.44f,    0.0f, 0.0f,
		 0.5f,  0.0f,  0.5f,    0.83f,  0.70f,  0.44f,    5.0f, 0.0f,
		 0.0f,  0.8f,  0.0f,    0.92f,  0.86f,  0.76f,    2.5f, 5.0f
		
	};
	
	GLuint indices[] =
	{
		0, 1, 2,
		0, 2, 3,
		0, 1, 4,
		1, 2, 4,
		2, 3, 4,
		3, 0, 4
		
	};


	GLFWwindow* window = glfwCreateWindow(length, width, "SmokeBuch", NULL, NULL); // info about the window
	if (window == NULL)
	{
		std::cout << "Window error" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window); // applying info of the window

	glfwSwapInterval(1);

	gladLoadGL();

	glViewport(0, 0, length, width);

	
	Shader shaderProgram("default.vert", "default.frag");

	VAO VAO1;
	VAO1.Bind();

	VBO VBO1(vertices, sizeof(vertices));
	EBO EBO1(indices, sizeof(indices));

	VAO1.LinkAttrib(VBO1, 0, 3, GL_FLOAT, 8 * sizeof(float), (void*)0);
	VAO1.LinkAttrib(VBO1, 1, 3, GL_FLOAT, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	VAO1.LinkAttrib(VBO1, 1, 2, GL_FLOAT, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	VAO1.Unbind();
	VBO1.Unbind();
	EBO1.Unbind();

	
	//texture
	Texture Grass("peter.png", GL_TEXTURE_2D, GL_TEXTURE0, GL_RGB, GL_UNSIGNED_BYTE);
	Grass.Bind();
	Grass.texUnit(shaderProgram, "tex0", 0);
	

	

	// enabling depth of sculpture
	glEnable(GL_DEPTH_TEST);

	Camera camera(width, length, glm::vec3(0.0f, 0.0f, 2.0f));

	 // main while loop
	while (!glfwWindowShouldClose(window)) // loop for not closing window
	{
		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shaderProgram.Activate();

		camera.Inputs(window);
		camera.Matrix(45.0f, 0.1f, 100.0f, shaderProgram, "camMatrix");
		

		
		Grass.Bind();
		VAO1.Bind();
		glDrawElements(GL_TRIANGLES, sizeof(indices)/sizeof(int), GL_UNSIGNED_INT, 0);
		glfwSwapBuffers(window);

		// all GLFW events
		glfwPollEvents();
	}
	
	VAO1.Delete();
	VBO1.Delete();
	EBO1.Delete();
	Grass.Delete();
	shaderProgram.Delete();


	glfwDestroyWindow(window);

	glfwTerminate();
	std::cin.get();
}