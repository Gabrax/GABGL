#include "Windowbase.h"
#include "MainWindow.h"
#include "StartWindow.h"
#include "BackendLogger.h"
#include "../Input/ApplicationEvent.h"
#include "../Input/MouseEvent.h"
#include "../Input/KeyEvent.h"

#include <iostream>

static void GLFWErrorCallback(int error, const char* description)
{
	GABGL_ERROR("GLFW Error ({0}): {1}", error, description);
}

MainWindow::MainWindow(const WindowDefaultData& props)
{
  Init(props);
}

MainWindow::~MainWindow()
{
    Terminate();
}

void MainWindow::Init(const WindowDefaultData& props)
{
  m_Data.title = props.title;
  m_Data.Width = props.Width;
  m_Data.Height = props.Height;

  GABGL_INFO("Creating window {0} ({1},{2})", props.title, props.Width, props.Height);

  if(!StartWindow::isGLFWInit())
  {
    int GLFWstatus = glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GABGL_ASSERT(GLFWstatus, "Failed to init GLFW");
	glfwSetErrorCallback(GLFWErrorCallback);
  }
  
  const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
  m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, props.title.c_str(), nullptr, nullptr);
  glfwMakeContextCurrent(m_Window);
  if (!StartWindow::isGLADInit())
  {
	  int GLADstatus = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	  GABGL_ASSERT(GLADstatus, "Failed to init GLAD");
  }
  int xPos = (mode->width - props.Width) / 2;
  int yPos = (mode->height - props.Height) / 2;
  glfwSetWindowPos(m_Window, xPos, yPos);  // Set the window position to the center
  glfwSetWindowUserPointer(m_Window, &m_Data);
  SetVSync(true);

  // Set GLFW callbacks
  glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
	  {
		  WindowSpecificData& data = *reinterpret_cast<WindowSpecificData*>(glfwGetWindowUserPointer(window));
		  data.Width = width;
		  data.Height = height;

		  WindowResizeEvent event(width, height);
		  data.EventCallback(event);
	  });

  glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
	  {
		  WindowSpecificData& data = *reinterpret_cast<WindowSpecificData*>(glfwGetWindowUserPointer(window));
		  WindowCloseEvent event;
		  data.EventCallback(event);
	  });

  glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
	  {
		  WindowSpecificData& data = *reinterpret_cast<WindowSpecificData*>(glfwGetWindowUserPointer(window));

		  switch (action)
		  {
			  case GLFW_PRESS:
			  {
				  KeyPressedEvent event(key, 0);
				  data.EventCallback(event);
				  break;
			  }
			  case GLFW_RELEASE:
			  {
				  KeyReleasedEvent event(key);
				  data.EventCallback(event);
				  break;
			  }
			  case GLFW_REPEAT:
			  {
				  KeyPressedEvent event(key, true);
				  data.EventCallback(event);
				  break;
			  }
		  }
	  });

  glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode)
	  {
		  WindowSpecificData& data = *reinterpret_cast<WindowSpecificData*>(glfwGetWindowUserPointer(window));

		  KeyTypedEvent event(keycode);
		  data.EventCallback(event);
	  });

  glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
	  {
		  WindowSpecificData& data = *reinterpret_cast<WindowSpecificData*>(glfwGetWindowUserPointer(window));

		  switch (action)
		  {
			  case GLFW_PRESS:
			  {
				  MouseButtonPressedEvent event(button);
				  data.EventCallback(event);
				  break;
			  }
			  case GLFW_RELEASE:
			  {
				  MouseButtonReleasedEvent event(button);
				  data.EventCallback(event);
				  break;
			  }
		  }
	  });

  glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
	  {
		  WindowSpecificData& data = *reinterpret_cast<WindowSpecificData*>(glfwGetWindowUserPointer(window));

		  MouseScrolledEvent event((float)xOffset, (float)yOffset);
		  data.EventCallback(event);
	  });

  glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
	  {
		  WindowSpecificData& data = *reinterpret_cast<WindowSpecificData*>(glfwGetWindowUserPointer(window));

		  MouseMovedEvent event((float)xPos, (float)yPos);
		  data.EventCallback(event);
	  });

  GLint major, minor;
  glGetIntegerv(GL_MAJOR_VERSION, &major);
  glGetIntegerv(GL_MINOR_VERSION, &minor);
  const GLubyte* vendor = glGetString(GL_VENDOR);
  const GLubyte* renderer = glGetString(GL_RENDERER);
  GABGL_TRACE("GPU: {}", reinterpret_cast<const char*>(renderer));
  GABGL_TRACE("GL: {0}.{1}", major, minor);

  glClearColor(1, 0, 1, 1);
}

void MainWindow::Terminate()
{
	glfwDestroyWindow(m_Window);
}

void MainWindow::Update()
{
	glClear(GL_COLOR_BUFFER_BIT);
	glfwSwapBuffers(m_Window);
	glfwPollEvents();
}

void MainWindow::SetVSync(bool enabled)
{
  if(enabled)
    glfwSwapInterval(1);
  else
    glfwSwapInterval(0);

  m_Data.VSync = enabled;
}

bool MainWindow::IsVSync() const 
{
  return m_Data.VSync;
}


