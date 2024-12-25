#include "Windowbase.h"
#include "StartWindow.h"
#include "BackendLogger.h"
#include "../Input/EngineEvent.h"
#include "../Input/KeyEvent.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imguiThemes.h"
#include <stb_image.h>

bool StartWindow::m_GLFWInitialized = false;
bool StartWindow::m_GLADInitialized = false;
bool StartWindow::m_WindowClosed = false;

static void GLFWErrorCallback(int error, const char* description)
{
	GABGL_ERROR("GLFW Error ({0}): {1}", error, description);
}

StartWindow::StartWindow(const WindowDefaultData& props)
{
	Init(props);
	ImGui::CreateContext();

	imguiThemes::embraceTheDarkness();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

	io.FontGlobalScale = 1.0f;

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.Colors[ImGuiCol_WindowBg].w = 0.f;  // Set window background to be transparent
		style.Colors[ImGuiCol_DockingEmptyBg].w = 0.f; // Transparent docking area
	}

	ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
}

StartWindow::~StartWindow()
{
	Terminate();
}

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
		GABGL_ASSERT(GLFWstatus, "Failed to init GLFW");
		glfwSetErrorCallback(GLFWErrorCallback);
		m_GLFWInitialized = true;
	}

	m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, props.title.c_str(), nullptr, nullptr);
	glfwMakeContextCurrent(m_Window);
	GLFWimage images[1];
	images[0].pixels = stbi_load("../res/Opengllogo.png", &images[0].width, &images[0].height, 0, 4);
	if (images[0].pixels) {
		glfwSetWindowIcon(m_Window, 1, images);
		stbi_image_free(images[0].pixels);
	}
	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	int xPos = (mode->width - props.Width) / 2;
	int yPos = (mode->height - props.Height) / 2;
	glfwSetWindowPos(m_Window, xPos, yPos);  // Set the window position to the center
	if(!isGLADInit())
	{
		int GLADstatus = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		GABGL_ASSERT(GLADstatus, "Failed to init GLAD");
		m_GLADInitialized = true;
	}
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

	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
}

void StartWindow::Terminate()
{
	glfwDestroyWindow(m_Window);
	m_WindowClosed = true;
}

void StartWindow::Update()
{
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

	ImGui::Begin("My ImGui Window");  

	ImGui::Text("Hello, ImGui!");     

	if (ImGui::Button("Click Me"))
	{
		m_WindowClosed = true;
	}

	ImGui::End();

	ImGui::Begin("Select Project Window");

	if (ImGui::Button("Proj"))
	{
		
	}

	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}

	glfwSwapBuffers(m_Window);
	glfwPollEvents();
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


