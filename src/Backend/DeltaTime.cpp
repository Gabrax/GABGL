#include "DeltaTime.h"
#include <GLFW/glfw3.h>

float DeltaTime::s_LastFrameTime = 0.0f;

DeltaTime::DeltaTime()
{
	float currentTime = static_cast<float>(glfwGetTime()); 
	m_Time = currentTime - s_LastFrameTime;                
	s_LastFrameTime = currentTime;                         
}
