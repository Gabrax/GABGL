#include "Windowbase.h"
#include "StartWindow.h"
#include "BackendLogger.h"
#include "../Input/EngineEvent.h"
#include "../Input/KeyEvent.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imguiThemes.h"
#include "../Renderer/Texture.h"

bool StartWindow::m_GLFWInitialized = false;

static void GLFWErrorCallback(int error, const char* description)
{
	GABGL_ERROR("GLFW Error ({0}): {1}", error, description);
}

StartWindow::StartWindow(const WindowDefaultData& props) { Init(props); }

StartWindow::~StartWindow() { /*Terminate();*/ }

void StartWindow::Init(const WindowDefaultData& props)
{
	m_Data.title = props.title;
	m_Data.Width = props.Width;
	m_Data.Height = props.Height;

	GABGL_INFO("Creating window {0} ({1},{2})", props.title, props.Width, props.Height);

	if (!isGLFWInit())
	{
		int GLFWstatus = glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
		GABGL_ASSERT(GLFWstatus, "Failed to init GLFW");
		glfwSetErrorCallback(GLFWErrorCallback);
		m_GLFWInitialized = true;
	}

	m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, props.title.c_str(), nullptr, nullptr);
	glfwSetWindowAttrib(m_Window, GLFW_RESIZABLE, false);

	SetWindowIcon("../res/engineTextures/gabglicon.png", m_Window);
	
	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	int xPos = (mode->width - props.Width) / 2;
	int yPos = (mode->height - props.Height) / 2;
	glfwSetWindowPos(m_Window, xPos, yPos);  // Set the window position to the center

	m_Context = GraphicsContext::Create(m_Window);
	m_Context->Init();

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

	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
}

void StartWindow::Terminate()
{
	glfwDestroyWindow(m_Window);
	m_WindowClosed = true;
}

void StartWindow::Update()
{
	glfwPollEvents();
	m_Context->SwapBuffers();
}

void StartWindow::SetVSync(bool enabled)
{
	if (enabled)
		glfwSwapInterval(1);
	else
		glfwSwapInterval(0);

	m_Data.VSync = enabled;
}

bool StartWindow::IsVSync() const
{
	return m_Data.VSync;
}

