#include "InventoryManager.h"

#include <numeric>

InventoryManager& InventoryManager::GetInstance()
{
  static InventoryManager instance;
  return instance;
}

void InventoryManager::ToggleInventory()
{
  m_IsOpen = !m_IsOpen;
}

bool InventoryManager::AddItem(std::shared_ptr<Item> item)
{
  if (!item || IsFull()) return false;

  item->weight = item->weight < 0.0f ? 0.0f : item->weight;
  m_Items.push_back(std::move(item));
  return true;
}

bool InventoryManager::RemoveItem(const size_t index)
{
  if (index >= m_Items.size()) return false;
  m_Items.erase(m_Items.begin() + static_cast<std::ptrdiff_t>(index));
  return true;
}

void InventoryManager::Clear()
{
  m_Items.clear();
  m_IsOpen = false;
}

float InventoryManager::GetTotalWeight() const
{
  return std::accumulate(m_Items.begin(), m_Items.end(), 0.0f,
    [](const float total, const std::shared_ptr<Item>& item)
    {
      return total + (item ? item->weight : 0.0f);
    });
}
