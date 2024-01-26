#include "Window.h"
#include <iostream>
#include <string>

namespace Window {
    inline GLFWwindow* _window;
    inline GLFWmonitor* _monitor;
    inline const GLFWvidmode* _mode;
    inline int _currentWidth = 0;
    inline int _currentHeight = 0;
    inline int _windowedWidth = 0;
    inline int _windowedHeight = 0;
    inline int _fullscreenWidth = 0;
    inline int _fullscreenHeight = 0;
    inline int _mouseScreenX = 0;
    inline int _mouseScreenY = 0;
    inline int _windowHasFocus = true;
    inline bool _forceCloseWindow = false;
    inline int _scrollWheelYOffset = 0;
    inline enum WindowMode _windowMode = WINDOWED;// FULLSCREEN;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void window_focus_callback(GLFWwindow* window, int focused);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

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

void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei /*length*/, const char* message, const void* /*userParam*/) {
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

void Window::ToggleFullscreen() {
    if (_windowMode == WINDOWED)
        SetWindowMode(FULLSCREEN);
    else
        SetWindowMode(WINDOWED);
}

int Window::GetScrollWheelYOffset() {
    return _scrollWheelYOffset;
}

void Window::ResetScrollWheelYOffset() {
    _scrollWheelYOffset = 0;
}

void Window::CreateWindow(WindowMode windowMode) {
    if (windowMode == WINDOWED) {
        _currentWidth = _windowedWidth;
        _currentHeight = _windowedHeight;
        _window = glfwCreateWindow(_windowedWidth, _windowedHeight, "JD", NULL, NULL);
    }
    else if (windowMode == FULLSCREEN) {
        _currentWidth = _fullscreenWidth;
        _currentHeight = _fullscreenHeight;
        _window = glfwCreateWindow(_fullscreenWidth, _fullscreenHeight, "GAB", _monitor, NULL);
    }
    _windowMode = windowMode;
}

void Window::SetWindowMode(WindowMode windowMode) {
    if (windowMode == WINDOWED) {
        _currentWidth = _windowedWidth;
        _currentHeight = _windowedHeight;
        glfwSetWindowMonitor(_window, nullptr, 0, 0, _windowedWidth, _windowedHeight, 0);
    } 
    else if (windowMode == FULLSCREEN) {
        _currentWidth = _fullscreenWidth;
        _currentHeight = _fullscreenHeight;
        glfwSetWindowMonitor(_window, _monitor, 0, 0, _fullscreenWidth, _fullscreenHeight, _mode->refreshRate);
    }
    _windowMode = windowMode;
}


void Window::Init(int width, int height) {

    glfwInit();
	glfwSetErrorCallback([](int error, const char* description) {
        std::cout << "GLFW Error (" << std::to_string(error) << "): " << description << "\n";
	});

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);    
    // Resolution and window size
    _monitor = glfwGetPrimaryMonitor();
    _mode = glfwGetVideoMode(_monitor);
    glfwWindowHint(GLFW_RED_BITS, _mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, _mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, _mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, _mode->refreshRate);
    _fullscreenWidth = _mode->width;
    _fullscreenHeight = _mode->height;
    _windowedWidth = width;
    _windowedHeight = height;
	if (_windowedWidth > _fullscreenWidth || _windowedHeight > _fullscreenHeight) {
		_windowedWidth = _fullscreenWidth * 0.75f;
		_windowedHeight = _fullscreenHeight * 0.75f;
	}
    if (_windowMode == FULLSCREEN) {
        _currentWidth = _fullscreenWidth;
        _currentHeight = _fullscreenHeight;
        _window = glfwCreateWindow(_fullscreenWidth, _fullscreenHeight, "JD", _monitor, NULL);
    } 
    else {
        _currentWidth = _windowedWidth;
        _currentHeight = _windowedHeight;
        _window = glfwCreateWindow(_windowedWidth, _windowedHeight, "GAB", NULL, NULL);
		glfwSetWindowPos(_window, 0, 0);
    }
    if (_window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(_window);
    glfwSetFramebufferSizeCallback(_window, framebuffer_size_callback);
    glfwSetWindowFocusCallback(_window, window_focus_callback); 
    glfwSetScrollCallback(_window, scroll_callback);
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
        //std::cout << "Debug GL context enabled\n";
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // makes sure errors are displayed synchronously
        glDebugMessageCallback(glDebugOutput, nullptr);
    } else {
        std::cout << "Debug GL context not available\n";
    }    
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Clear screen to black
    glClear(GL_COLOR_BUFFER_BIT);
    glfwSwapBuffers(_window);
    glfwPollEvents();
}

void Window::ProcessInput() {
    processInput(_window);
}

void Window::SwapBuffersPollEvents() {
    glfwSwapBuffers(_window);
    glfwPollEvents();
}

void Window::Cleanup() {
    glfwTerminate();
}

bool Window::WindowIsOpen() {
    return !glfwWindowShouldClose(_window);
}

int Window::GetWindowWidth() {
    return _currentWidth;
}

int Window::GetWindowHeight() {
    return _currentHeight;
}

int Window::GetCursorX() {
    double xpos, ypos;
    glfwGetCursorPos(_window, &xpos, &ypos);
    return int(xpos);
}

int Window::GetCursorY() {
    double xpos, ypos;
    glfwGetCursorPos(_window, &xpos, &ypos);
    return int(ypos);
}

void Window::DisableCursor() {
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Window::HideCursor() {
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

void Window::ShowCursor() {
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

GLFWwindow* Window::GetWindowPtr() {
    return _window;
}

int Window::GetCursorScreenX() {
    return _mouseScreenX;
}

int Window::GetCursorScreenY() {
    return _mouseScreenY;
}

bool Window::WindowHasFocus() {
    return _windowHasFocus;
}

bool Window::WindowHasNotBeenForceClosed() {
    return !_forceCloseWindow;
}

void Window::ForceCloseWindow() {
    _forceCloseWindow = true;
}

// Callbacks

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height) {
    glViewport(0, 0, width, height);
}

void window_focus_callback(GLFWwindow* /*window*/, int focused) {
    if (focused){
        Window::_windowHasFocus = true;
    }
    else{
        Window::_windowHasFocus = false;
    }
}

void scroll_callback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset) {
    Window::_scrollWheelYOffset = (int)yoffset;
}