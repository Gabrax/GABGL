#include "GraphicsContext.h"
#include "../backend/BackendLogger.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>

GraphicsContext::GraphicsContext(GLFWwindow* windowHandle)
	: m_WindowHandle(windowHandle)
{
	GABGL_ASSERT(windowHandle, "Window handle is null!")
}

void GraphicsContext::Init()
{
	glfwMakeContextCurrent(m_WindowHandle);
	int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	GABGL_ASSERT(status, "Failed to initialize Glad!");

	GABGL_INFO("OpenGL Info:");
	GABGL_INFO("  Vendor: {}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
	GABGL_INFO("  Renderer: {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
	GABGL_INFO("  Version: {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));

	GABGL_ASSERT(GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 5), "GABGL requires at least OpenGL version 4.5!");
}

void GraphicsContext::SwapBuffers()
{
	glfwSwapBuffers(m_WindowHandle);
}

Scope<GraphicsContext> GraphicsContext::Create(void* window)
{
	return CreateScope<GraphicsContext>(reinterpret_cast<GLFWwindow*>(window));
}
