#include "UserInput.h"

#include <GLFW/glfw3.h>
#include "../backend/Window.h"

bool Input::IsKeyPressed(const KeyCode key)
{
	auto* window = static_cast<GLFWwindow*>(Window::GetWindowPtr());
	auto state = glfwGetKey(window, static_cast<int32_t>(key));
	return state == GLFW_PRESS;
}

bool Input::IsMouseButtonPressed(const MouseCode button)
{
	auto* window = static_cast<GLFWwindow*>(Window::GetWindowPtr());
	auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
	return state == GLFW_PRESS;
}

glm::vec2 Input::GetMousePosition()
{
	auto* window = static_cast<GLFWwindow*>(Window::GetWindowPtr());
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	return { (float)xpos, (float)ypos };
}

float Input::GetMouseX()
{
	return GetMousePosition().x;
}

float Input::GetMouseY()
{
	return GetMousePosition().y;
}

bool Input::IsGamepadConnected()
{
  return glfwJoystickIsGamepad(GLFW_JOYSTICK_1) == GLFW_TRUE;
}

bool Input::IsGamepadButtonPressed(int button)
{
  GLFWgamepadstate state{};
  if (!IsGamepadConnected() || glfwGetGamepadState(GLFW_JOYSTICK_1, &state) != GLFW_TRUE)
    return false;
  return button >= 0 && button <= GLFW_GAMEPAD_BUTTON_LAST && state.buttons[button] == GLFW_PRESS;
}

float Input::GetGamepadAxis(int axis)
{
  GLFWgamepadstate state{};
  if (!IsGamepadConnected() || glfwGetGamepadState(GLFW_JOYSTICK_1, &state) != GLFW_TRUE)
    return 0.0f;
  return axis >= 0 && axis <= GLFW_GAMEPAD_AXIS_LAST ? state.axes[axis] : 0.0f;
}
