#include "GraphicsContext.h"

#include "../GL/OpenGLContext.h"

Scope<GraphicsContext> GraphicsContext::Create(void* window)
{
	return CreateScope<OpenGLContext>(static_cast<GLFWwindow*>(window));
}
