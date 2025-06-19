#include "LayerStack.h"

#include "../input/Event.h"
#include <string>
#include <vector>

#include <iostream>

static std::vector<Layer*> m_Layers;
static uint32_t m_LayerInsertIndex = 0;

std::vector<Layer*>& LayerStack::GetLayers()
{
  return m_Layers;
}

void LayerStack::PushLayer(Layer* layer)
{
  m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
  m_LayerInsertIndex++;
}

void LayerStack::PushOverlay(Layer* overlay)
{
  m_Layers.emplace_back(overlay);
}

void LayerStack::PopLayer(Layer* layer)
{
  auto it = std::find(m_Layers.begin(), m_Layers.begin() + m_LayerInsertIndex, layer);
  if (it != m_Layers.begin() + m_LayerInsertIndex) {
      delete *it;
      m_Layers.erase(it);
      m_LayerInsertIndex--;
  }
}

void LayerStack::PopOverlay(Layer* overlay)
{
  auto it = std::find(m_Layers.begin() + m_LayerInsertIndex, m_Layers.end(), overlay);
  if (it != m_Layers.end()) {
      delete *it;
      m_Layers.erase(it);
  }
}

void LayerStack::OnUpdate(DeltaTime& dt)
{
  for (Layer* layer : m_Layers)
      layer->OnUpdate(dt);
}

void LayerStack::OnImGuiRender()
{
  for (Layer* layer : m_Layers)
      layer->OnImGuiRender();
}

