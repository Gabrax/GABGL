#include <GLFW/glfw3.h>
#include "window.h"
#include "../input/EngineEvent.h"
#include "../input/KeyEvent.h"
#include "BackendLogger.h"
#include <stb_image.h>
#include "Renderer.h"
#include "LayerStack.h"

static void GLFWErrorCallback(int error, const char* description)
{
	GABGL_ERROR("GLFW Error ({0}): {1}", error, description);
}

Window::Window(const WindowDefaultData& props)
{
  Init(props);
  SetEventCallback(BIND_EVENT(OnEvent));
}

Window::~Window(){}

void Window::Init(const WindowDefaultData& props)
{
  m_Data.title = props.title;
  m_Data.Width = props.Width;
  m_Data.Height = props.Height;

  GABGL_INFO("Creating window {0} ({1},{2})", props.title, props.Width, props.Height);

  int GLFWstatus = glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
  GABGL_ASSERT(GLFWstatus, "Failed to init GLFW");
  glfwSetErrorCallback(GLFWErrorCallback);
  
#ifdef DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

  m_Window = glfwCreateWindow((int)m_Data.Width, (int)m_Data.Height, m_Data.title.c_str(), nullptr, nullptr);
  m_Monitor = glfwGetPrimaryMonitor();
  m_Mode = glfwGetVideoMode(m_Monitor);
  glfwMakeContextCurrent(m_Window);

	int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	GABGL_ASSERT(status, "Failed to initialize Glad!");

	GABGL_INFO("OpenGL Info:");
	GABGL_INFO("  Vendor: {}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
	GABGL_INFO("  Renderer: {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
	GABGL_INFO("  Version: {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));

	GABGL_ASSERT(GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 5), "GABGL requires at least OpenGL version 4.5!");

  glfwSetWindowUserPointer(m_Window, &m_Data);

  SetWindowIcon("res/textures/gabglicon.png", m_Window);
  SetResizable(false);
  CenterWindowPos();
  SetVSync(true);
  Maximize(false);

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

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void Window::Terminate()
{
	glfwDestroyWindow(m_Window);
}

void Window::Update()
{
	glfwPollEvents();
	glfwSwapBuffers(m_Window);
}

std::unique_ptr<Window> Window::Create(const WindowDefaultData& props)
{
  return std::make_unique<Window>(props);
}

void Window::SetWindowIcon(const char* iconpath, GLFWwindow* window)
{
  stbi_set_flip_vertically_on_load(0);
  GLFWimage images[1];
  images[0].pixels = stbi_load(iconpath, &images[0].width, &images[0].height, 0, 4);
  if (images[0].pixels) {
    glfwSetWindowIcon(window, 1, images);
    stbi_image_free(images[0].pixels);
  }
}

void Window::SetVSync(bool enabled)
{
  if(enabled) glfwSwapInterval(1);
  else glfwSwapInterval(0);

  m_Data.VSync = enabled;
}

bool Window::IsVSync() const 
{
  return m_Data.VSync;
}

void Window::SetResolution(uint32_t width, uint32_t height) 
{ 
  glViewport(0, 0, width, height);
  m_Data.Width = width;
  m_Data.Height = height;
}

void Window::CenterWindowPos()
{
  glfwGetWindowSize(m_Window, &currWidth, &currHeight);
  int32_t xpos = (m_Mode->width - currWidth) / 2, ypos = (m_Mode->height - currHeight) / 2;

  glfwSetWindowPos(m_Window, xpos, ypos);
}

void Window::SetFullscreen(bool full)
{
  if(full)
  {
    glfwSetWindowMonitor(m_Window, m_Monitor, 0, 0, m_Mode->width, m_Mode->height, m_Mode->refreshRate);
    if(IsVSync()) SetVSync(true);
  } 
  else
  {
    glfwSetWindowMonitor(m_Window, nullptr, currWidth, currHeight, 1000, 600, 0);
    CenterWindowPos();
  }
}

void Window::Maximize(bool maximize)  
{
  if(maximize) glfwMaximizeWindow(m_Window);
}

void Window::SetResizable(bool enable)
{
  glfwSetWindowAttrib(m_Window, GLFW_RESIZABLE, enable);
}

void Window::SetCursorVisible(bool enable)
{
  if (enable)
  {
    glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    int32_t width, height;
    glfwGetWindowSize(m_Window, &width, &height);
    glfwSetCursorPos(m_Window, width / 2.0, height / 2.0);

  }
  else
  {
    glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  }
}

void Window::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT(OnWindowClose));
	dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT(OnWindowResize));

  auto& m_LayerStack = LayerStack::GetLayers();
	for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
	{
		if (e.Handled)
			break;
		(*it)->OnEvent(e);
	}
}

bool Window::OnWindowClose(WindowCloseEvent& e)
{
	m_isRunning = false;
	return true;
}

bool Window::OnWindowResize(WindowResizeEvent& e)
{

	if (e.GetWidth() == 0 || e.GetHeight() == 0)
	{
		m_isMinimized = true;
		return false;
	}

	m_isMinimized = false;
  glViewport(0, 0, e.GetWidth(), e.GetHeight());

	return false;
}
