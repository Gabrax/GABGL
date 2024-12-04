#include <iostream>
#include <string>


#include "Input/Input.h"
#include "Window.h"
#include "stb_image.h"

namespace Window {

	// window attributes
	inline GLFWwindow* _window;
  inline GLFWmonitor* _monitor;
  inline const GLFWvidmode* _mode;
  inline int _currentWidth = 0;
  inline int _currentHeight = 0;
  inline int _windowedWidth = 1920 * 1.5f;
  inline int _windowedHeight = 1080 * 1.5f;
  inline int _fullscreenWidth = 0;
  inline int _fullscreenHeight = 0;
  inline int _mouseScreenX = 0;
  inline int _mouseScreenY = 0;
  inline int _windowHasFocus = true;
  inline bool _forceCloseWindow = false;
  inline int _scrollWheelYOffset = 0;
  inline enum WindowMode _windowMode = WINDOWED;// FULLSCREEN;
  inline enum RenderMode _renderMode = WIREFRAME;
  inline double prevTime = 0.0;
  inline double crntTime = 0.0;
  inline double timeDiff;
  inline unsigned int counter = 0;
  inline int windowPosX = (_windowedWidth - _windowedWidth) / 2;
  inline int windowPosY = (_windowedHeight - _windowedHeight) / 2;

  // timing
  inline float _deltaTime = 0.0f;	
  inline float _lastFrame = 0.0f;
  
  bool disablemovement = false;
}

GLenum glCheckError_(const char* file, int line) {
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        std::string error;
        switch (errorCode)
        {
        case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
        case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
        case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
        case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
        case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
        case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}

#define glCheckError() glCheckError_(__FILE__, __LINE__)

void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam) {
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return; // ignore these non-significant error codes
    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " << message << std::endl;
    switch (source){
    case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
    case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
    case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
    } std::cout << std::endl;
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
    case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
    case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
    case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
    case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
    case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
    } std::cout << std::endl;

    switch (severity){
    case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
    case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
    case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
    } std::cout << std::endl;
    std::cout << std::endl;
    std::cout << std::endl;
}


void Window::ToggleFullscreen()
{
    if (_windowMode == WINDOWED){
        SetWindowMode(FULLSCREEN);
    }else{
        SetWindowMode(WINDOWED);
    }
}

void Window::ToggleWireframe()
{
    
    if (_renderMode == NORMAL)
    {
        SetRenderMode(WIREFRAME);
    }
    else 
    {
        SetRenderMode(NORMAL);
    }
}

void Window::CreateWindow(WindowMode windowMode)
{
    if (windowMode == WINDOWED)
    {
        _currentWidth = _windowedWidth;
        _currentHeight = _windowedHeight;
        _window = glfwCreateWindow(_windowedWidth, _windowedHeight, "OpenGL", NULL, NULL);
        glfwSetWindowPos(_window, 100, 100);
        if (_mode != NULL) {
                int xpos = (_mode->width - _currentWidth) / 2;
                int ypos = (_mode->height - _currentHeight) / 2;
                glfwSetWindowPos(_window, xpos, ypos);
            }
    }
    else if (windowMode == FULLSCREEN)
    {
        _currentWidth = _fullscreenWidth;
        _currentHeight = _fullscreenHeight;
        _window = glfwCreateWindow(_fullscreenWidth, _fullscreenHeight, "OpenGL", _monitor, NULL);
    }
    _windowMode = windowMode;
}

void Window::SetWindowMode(WindowMode windowMode)
{
    if (windowMode == WINDOWED)
    {
        _currentWidth = _windowedWidth;
        _currentHeight = _windowedHeight;
        
        glfwSetWindowMonitor(_window, nullptr, 0, 0, _windowedWidth, _windowedHeight, 0);
        if (_mode != NULL) {
                int xpos = (_mode->width - _currentWidth) / 2;
                int ypos = (_mode->height - _currentHeight) / 2;
                glfwSetWindowPos(_window, xpos, ypos);
            }
    } 
    else if (windowMode == FULLSCREEN)
    {
        _currentWidth = _fullscreenWidth;
        _currentHeight = _fullscreenHeight;
        
        glfwSetWindowMonitor(_window, _monitor, 0, 0, _fullscreenWidth, _fullscreenHeight, _mode->refreshRate);
    }
    _windowMode = windowMode;
}

void Window::SetRenderMode(RenderMode renderMode)
{
    if (renderMode == WIREFRAME)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else if (renderMode == NORMAL)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    _renderMode = renderMode;
}


void Window::Init()
{

    glfwInit();
    glfwSetErrorCallback([](int error, const char* description)
      {
          std::cout << "GLFW Error (" << std::to_string(error) << "): " << description << "\n";
    });


    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    #ifdef __APPLE__
	    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);    
    glfwWindowHint(GLFW_SAMPLES, 8);


    // Resolution and window size
    _monitor = glfwGetPrimaryMonitor();
    _mode = glfwGetVideoMode(_monitor);
    glfwWindowHint(GLFW_RED_BITS, _mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, _mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, _mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, _mode->refreshRate);
    _fullscreenWidth = _mode->width;
    _fullscreenHeight = _mode->height;
    /*_windowedWidth = width;*/
    /*_windowedHeight = height;*/

    if (_windowedWidth > _fullscreenWidth || _windowedHeight > _fullscreenHeight){
      _windowedWidth = static_cast<int>(_fullscreenWidth * 0.75f);
      _windowedHeight = static_cast<int>(_fullscreenHeight * 0.75f);
    }
    CreateWindow(WINDOWED);
    
    
    if (_window == nullptr){
        std::cout << "Failed to create GLFW window" << '\n';
        Cleanup();
        return;
    }

    GLFWimage images[1];
    images[0].pixels = stbi_load("res/Opengllogo.png", &images[0].width, &images[0].height, 0, 4); 
    if (images[0].pixels) {
        glfwSetWindowIcon(_window, 1, images);
        stbi_image_free(images[0].pixels); 
    }

    glfwMakeContextCurrent(_window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(_window, framebuffer_size_callback);
    glfwSetCursorPosCallback(_window, mouse_callback);
    glfwSetScrollCallback(_window, scroll_callback);
    DisableCursor();
    glfwSetWindowFocusCallback(_window, window_focus_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return;
    }
    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    std::cout << "\nGPU: " << renderer << "\n";
    std::cout << "GL version: " << major << "." << minor << "\n\n";

    int flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        std::cout << "Debug GL context enabled\n\n";
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // makes sure errors are displayed synchronously
        glDebugMessageCallback(glDebugOutput, nullptr);
    } else {
        std::cout << "Debug GL context not available\n";
    } 
       
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void Window::DeltaTime()
{
    float currentFrame = static_cast<float>(glfwGetTime());
    _deltaTime = currentFrame - _lastFrame;
    _lastFrame = currentFrame;
}

float Window::getDeltaTime()
{
  return _deltaTime;
}

void Window::ShowFPS()
{
    crntTime = glfwGetTime();
    timeDiff = crntTime - prevTime;
    counter++;
    if (timeDiff >= 1.0 / 30.0)
    {
        std::string FPS = std::to_string(static_cast<int>((1.0 / timeDiff) * counter));
        std::string newTitle = "OpenGL - " + FPS + "FPS";
        glfwSetWindowTitle(_window, newTitle.c_str());
        prevTime = crntTime;
        counter = 0;
    }
}

void Window::EndFrame()
{
    Input::Update();
    processInput(_window);
    glfwSwapBuffers(_window);
}

void Window::BeginFrame()
{
    ShowFPS();
    DeltaTime();
    glfwPollEvents();
}

float Window::getAspectRatio()  
{
  return static_cast<float>(_windowedWidth) / static_cast<float>(_windowedHeight);
}

void Window::Cleanup()
{
    glfwTerminate();
}

GLFWwindow* Window::GetWindowPtr()
{
    return _window;
}

bool Window::WindowIsOpen()
{
    return !glfwWindowShouldClose(_window);
}

bool Window::WindowHasFocus()
{
    return _windowHasFocus;
}

bool Window::WindowHasNotBeenForceClosed()
{
    return !_forceCloseWindow;
}

void Window::ForceCloseWindow()
{
    _forceCloseWindow = true;
}

void Window::window_focus_callback(GLFWwindow* window, int focused)
{
    if (focused)
    {
        Window::_windowHasFocus = true;
    }
    else
    {
        Window::_windowHasFocus = false;
    }
}

int Window::GetWindowWidth()
{
    return _currentWidth;
}

int Window::GetWindowHeight()
{
    return _currentHeight;
}

int Window::GetWindowedWidth()
{
  return _windowedWidth;
}

int Window::GetWindowedHeight()
{
  return _windowedHeight;
}

int Window::GetFullscreenWidth()
{
  return _fullscreenWidth;
}

int Window::GetFullscreenHeight()
{
 return _fullscreenHeight;
}

int Window::GetScrollWheelYOffset()
{
    return _scrollWheelYOffset;
}

void Window::ResetScrollWheelYOffset()
{
    _scrollWheelYOffset = 0;
}

int Window::GetCursorX()
{
    double xpos, ypos;
    glfwGetCursorPos(_window, &xpos, &ypos);
    return int(xpos);
}

int Window::GetCursorY()
{
    double xpos, ypos;
    glfwGetCursorPos(_window, &xpos, &ypos);
    return int(ypos);
}

int Window::GetCursorScreenX()
{
    return _mouseScreenX;
}

int Window::GetCursorScreenY()
{
    return _mouseScreenY;
}

void Window::DisableCursor()
{
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Window::HideCursor()
{
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

void Window::ShowCursor()
{
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void Window::processInput(GLFWwindow* window)
{
  if(!disablemovement){
    if (Input::KeyDown(KEY_ESCAPE)) glfwSetWindowShouldClose(window, true);
    if (Input::KeyDown(KEY_W)) _camera.ProcessKeyboard(FORWARD, _deltaTime);
    if (Input::KeyDown(KEY_S)) _camera.ProcessKeyboard(BACKWARD, _deltaTime);
    if (Input::KeyDown(KEY_A)) _camera.ProcessKeyboard(LEFT, _deltaTime);
    if (Input::KeyDown(KEY_D)) _camera.ProcessKeyboard(RIGHT, _deltaTime);
    if (Input::KeyDown(KEY_SPACE)) _camera.ProcessKeyboard(UP, _deltaTime);
    if (Input::KeyDown(KEY_LEFT_CONTROL)) _camera.ProcessKeyboard(DOWN, _deltaTime);
  }
}

void Window::mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (_firstMouse)
    {
        _lastX = xpos;
        _lastY = ypos;
        _firstMouse = false;
    }

    float xoffset = xpos - _lastX;
    float yoffset = _lastY - ypos; // reversed since y-coordinates go from bottom to top

    _lastX = xpos;
    _lastY = ypos;

    if(!disablemovement) _camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void Window::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if(!disablemovement) _camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void Window::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    if (width > 0 && height > 0) {
        glViewport(0, 0, width, height);
        // Set cursor to the center of the resized window
        double centerX = static_cast<double>(width) / 2.0;
        double centerY = static_cast<double>(height) / 2.0;

        glfwSetCursorPos(window, centerX, centerY);  // Move the cursor
        _lastX = static_cast<float>(centerX);
        _lastY = static_cast<float>(centerY);

        _firstMouse = true; // Skip the next offset calculation
    }
}

void Window::DisableMovement()
{
  disablemovement = !disablemovement;
}



