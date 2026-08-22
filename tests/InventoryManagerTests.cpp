#include "InventoryManager.h"

#include <memory>
#include <stdexcept>
#include <string>

namespace
{
  std::shared_ptr<Item> MakeItem(const std::string& name)
  {
    auto item = std::make_shared<Item>();
    item->name = name;
    item->description = "Test item";
    return item;
  }

  void Expect(const bool condition, const char* message)
  {
    if (!condition) throw std::runtime_error(message);
  }
}

int main()
{
  auto& inventory = InventoryManager::GetInstance();
  inventory.Clear();

  Expect(!inventory.IsOpen(), "Clear should close the inventory");
  Expect(inventory.GetItemCount() == 0, "A cleared inventory should be empty");
  Expect(!inventory.AddItem(nullptr), "A null item should be rejected");

  inventory.ToggleInventory();
  Expect(inventory.IsOpen(), "ToggleInventory should open a closed inventory");
  inventory.SetOpen(false);
  Expect(!inventory.IsOpen(), "SetOpen(false) should close the inventory");

  Expect(inventory.AddItem(MakeItem("Pistol")), "A valid item should be added");
  Expect(inventory.AddItem(MakeItem("Aid kit")), "A second valid item should be added");
  Expect(inventory.GetItemCount() == 2, "Both valid item objects should occupy slots");

  Expect(!inventory.RemoveItem(99), "An out-of-range slot should not be removed");
  Expect(inventory.RemoveItem(1), "An occupied slot should be removable");
  Expect(inventory.GetItemCount() == 1, "Removing an item should release its slot");

  while (!inventory.IsFull())
    Expect(inventory.AddItem(MakeItem("Item")), "Items should fit until capacity is reached");

  Expect(inventory.GetItemCount() == InventoryManager::GetCapacity(), "IsFull should match capacity");
  Expect(!inventory.AddItem(MakeItem("Overflow")), "An item beyond capacity should be rejected");

  inventory.SetOpen(true);
  inventory.Clear();
  Expect(!inventory.IsOpen(), "Clear should also reset open state");
  Expect(inventory.GetItems().empty(), "Clear should remove every item");
}
