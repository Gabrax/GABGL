#pragma once

#include "../Backend/BackendScopeRef.h"

struct GLFWwindow;

struct GraphicsContext
{
    GraphicsContext(GLFWwindow* windowHandle);

    virtual ~GraphicsContext() = default;

    void Init();
    void SwapBuffers();
    static Scope<GraphicsContext> Create(void* window);

private:
    GLFWwindow* m_WindowHandle;
};