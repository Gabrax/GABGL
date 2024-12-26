#pragma once

#include "DeltaTime.h"
#include "../Input/Event.h"
#include <string>

struct Layer
{
	Layer(const std::string& name = "Layer");
	virtual ~Layer() = default;

	virtual void OnAttach() {}
	virtual void OnDetach() {}
	virtual void OnUpdate(DeltaTime dt) {}
	virtual void OnImGuiRender() {}
	virtual void OnEvent(Event& event) {}

	const std::string& GetName() const { return m_DebugName; }
protected:
	std::string m_DebugName;
};
