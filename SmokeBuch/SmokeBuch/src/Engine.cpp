#include "Engine.h"
#include "Core/Window.h"
#include "Core/Input.h"
#include "Renderer/Renderer.h"


void Engine::Run()
{	

	Window::Init(1920 * 1.5f, 1080 * 1.5f);

	glEnable(GL_DEPTH_TEST);

	while (Window::WindowIsOpen() && Window::WindowHasNotBeenForceClosed())
	{

		   
		

		Renderer::Render();


		if (Input::KeyPressed(GLFW_KEY_F))
		{
			Window::ToggleFullscreen();
		}
		if (Input::KeyPressed(GLFW_KEY_H))
		{
			Window::ToggleWireframe();
		}

		


		Window::ProcessInput();
		Input::Update();
		Window::SwapBuffersPollEvents();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	
}