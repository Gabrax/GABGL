#pragma once

#include "Event.h"
#include "KeyCodes.h"

struct KeyEvent : Event
{
	KeyCode GetKeyCode() const { return m_KeyCode; }

	EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
protected:
	KeyEvent(const KeyCode keycode)
		: m_KeyCode(keycode) {}

	KeyCode m_KeyCode;
};

struct KeyPressedEvent : KeyEvent
{
	KeyPressedEvent(const KeyCode keycode, bool isRepeat = false)
		: KeyEvent(keycode), m_IsRepeat(isRepeat) {}

	bool IsRepeat() const { return m_IsRepeat; }

	std::string ToString() const override
	{
		std::stringstream ss;
		ss << "KeyPressedEvent: " << m_KeyCode << " (repeat = " << m_IsRepeat << ")";
		return ss.str();
	}

	EVENT_CLASS_TYPE(KeyPressed)
private:
	bool m_IsRepeat;
};

struct KeyReleasedEvent : KeyEvent
{
	KeyReleasedEvent(const KeyCode keycode)
		: KeyEvent(keycode) {}

	std::string ToString() const override
	{
		std::stringstream ss;
		ss << "KeyReleasedEvent: " << m_KeyCode;
		return ss.str();
	}

	EVENT_CLASS_TYPE(KeyReleased)
};

struct KeyTypedEvent : KeyEvent
{
	KeyTypedEvent(const KeyCode keycode)
		: KeyEvent(keycode) {}

	std::string ToString() const override
	{
		std::stringstream ss;
		ss << "KeyTypedEvent: " << m_KeyCode;
		return ss.str();
	}

	EVENT_CLASS_TYPE(KeyTyped)
};

struct MouseMovedEvent : Event
{
	MouseMovedEvent(const float x, const float y)
		: m_MouseX(x), m_MouseY(y) {}

	float GetX() const { return m_MouseX; }
	float GetY() const { return m_MouseY; }

	std::string ToString() const override
	{
		std::stringstream ss;
		ss << "MouseMovedEvent: " << m_MouseX << ", " << m_MouseY;
		return ss.str();
	}

	EVENT_CLASS_TYPE(MouseMoved)
	EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
private:
	float m_MouseX, m_MouseY;
};

struct MouseScrolledEvent : Event
{
	MouseScrolledEvent(const float xOffset, const float yOffset)
		: m_XOffset(xOffset), m_YOffset(yOffset) {}

	float GetXOffset() const { return m_XOffset; }
	float GetYOffset() const { return m_YOffset; }

	std::string ToString() const override
	{
		std::stringstream ss;
		ss << "MouseScrolledEvent: " << GetXOffset() << ", " << GetYOffset();
		return ss.str();
	}

	EVENT_CLASS_TYPE(MouseScrolled)
	EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
private:
	float m_XOffset, m_YOffset;
};

struct MouseButtonEvent : Event
{
	MouseCode GetMouseButton() const { return m_Button; }

	EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput | EventCategoryMouseButton)
protected:
	MouseButtonEvent(const MouseCode button)
		: m_Button(button) {}

	MouseCode m_Button;
};

struct MouseButtonPressedEvent : MouseButtonEvent
{
	MouseButtonPressedEvent(const MouseCode button)
		: MouseButtonEvent(button) {}

	std::string ToString() const override
	{
		std::stringstream ss;
		ss << "MouseButtonPressedEvent: " << m_Button;
		return ss.str();
	}

	EVENT_CLASS_TYPE(MouseButtonPressed)
};

struct MouseButtonReleasedEvent : MouseButtonEvent
{
	MouseButtonReleasedEvent(const MouseCode button)
		: MouseButtonEvent(button) {}

	std::string ToString() const override
	{
		std::stringstream ss;
		ss << "MouseButtonReleasedEvent: " << m_Button;
		return ss.str();
	}

	EVENT_CLASS_TYPE(MouseButtonReleased)
};
