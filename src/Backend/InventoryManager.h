#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

enum class ItemType : int
{
  Miscellaneous = 0,
  Weapon,
  Ammunition,
  Medical
};

struct Item
{
  std::string name;
  std::string description;
  float weight = 0.0f;
  ItemType type = ItemType::Miscellaneous;
};

class InventoryManager
{
public:
  static InventoryManager& GetInstance();

  [[nodiscard]] bool IsOpen() const { return m_IsOpen; }
  void ToggleInventory();
  void SetOpen(bool open) { m_IsOpen = open; }

  // Returns false for a null item or when every slot is occupied.
  bool AddItem(std::shared_ptr<Item> item);
  bool RemoveItem(size_t index);
  void Clear();

  [[nodiscard]] const std::vector<std::shared_ptr<Item>>& GetItems() const { return m_Items; }
  [[nodiscard]] size_t GetItemCount() const { return m_Items.size(); }
  [[nodiscard]] static constexpr size_t GetCapacity() { return MaxSlots; }
  [[nodiscard]] bool IsFull() const { return m_Items.size() >= MaxSlots; }
  [[nodiscard]] float GetTotalWeight() const;

private:
  InventoryManager() = default;
  ~InventoryManager() = default;

  InventoryManager(const InventoryManager&) = delete;
  InventoryManager& operator=(const InventoryManager&) = delete;

  static constexpr size_t MaxSlots = 30;
  std::vector<std::shared_ptr<Item>> m_Items;
  bool m_IsOpen = false;
};
