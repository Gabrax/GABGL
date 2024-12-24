#include "Windowbase.h"
#include "MainWindow.h"
#include "BackendLogger.h"

static bool s_GLFWInitialized = false;

Window* Window::Create(const WindowProps& props)
{
  return new MainWindow(props);
}

MainWindow::MainWindow(const WindowProps& props)
{
  Init(props);
}

MainWindow::~MainWindow()
{
    Terminate();
}

void MainWindow::Init(const WindowProps& props)
{
  m_Data.title = props.title;
  m_Data.Width = props.Width;
  m_Data.Height = props.Height;

  GABGL_INFO("Creating window {0} ({1},{2})", props.title, props.Width, props.Height);

  if(!s_GLFWInitialized)
  {
    int GLFWstatus = glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GABGL_ASSERT(GLFWstatus, "Failed to init GLFW");
    s_GLFWInitialized = true;
  }
  
  m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, props.title.c_str(), nullptr, nullptr);
  glfwMakeContextCurrent(m_Window);
  glfwSetWindowUserPointer(m_Window, &m_Data);
  SetVSync(true);

  int GLADstatus = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  GABGL_ASSERT(GLADstatus, "Failed to init GLAD");

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
  glfwPollEvents();
  glfwSwapBuffers(m_Window);
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


