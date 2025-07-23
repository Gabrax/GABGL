#pragma once

#include "DeltaTime.hpp"
#include "../input/Event.h"

struct Layer
{
  virtual ~Layer() = default;
  virtual void OnUpdate(DeltaTime& dt) {}; 
  virtual void OnImGuiRender() {};
  virtual void OnEvent(Event& e) {};
};

struct LayerStack
{
  static void PushLayer(Layer* layer);
  static void PushOverlay(Layer* overlay);
  static void PopLayer(Layer* layer);
  static void PopOverlay(Layer* overlay);
  static void OnUpdate(DeltaTime& dt);
  static void OnImGuiRender();
  static std::vector<Layer*>& GetLayers();
};
