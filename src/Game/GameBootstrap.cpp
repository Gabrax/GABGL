#include "GameBootstrap.h"

#include "DialogueManager.h"
#include "InventoryManager.h"
#include "SceneManager.h"
#include "Scenes.hpp"

#include <memory>
#include <utility>

namespace
{
  std::shared_ptr<Item> MakeInventoryItem(const SceneEntity& entity)
  {
    auto item = std::make_shared<Item>();
    item->name = entity.itemName.empty() ? entity.name : entity.itemName;
    item->modelName = entity.model;

    if (entity.model == "pistol")
    {
      item->description = "A RELIABLE SIDEARM. USEFUL AT SHORT RANGE.";
      item->type = ItemType::Weapon;
    }
    else if (entity.model == "shotgun")
    {
      item->description = "POWERFUL UP CLOSE, BUT SLOW TO RELOAD.";
      item->type = ItemType::Weapon;
    }
    else if (entity.model == "pistolammo")
    {
      item->description = "A BOX OF PISTOL AMMUNITION.";
      item->type = ItemType::Ammunition;
    }
    else if (entity.model == "shotgunammo")
    {
      item->description = "A BOX OF SHOTGUN SHELLS.";
      item->type = ItemType::Ammunition;
    }
    else if (entity.model == "aidkit")
    {
      item->description = "MEDICAL SUPPLIES FOR TREATING SERIOUS WOUNDS.";
      item->type = ItemType::Medical;
    }
    else
    {
      item->description = "A COLLECTED ITEM.";
    }

    return item;
  }
}

void Game::Register()
{
  SceneManager::RegisterScene("game", [] { return std::make_unique<GameScene>(); });
  SceneManager::RegisterScene("menu", [] { return std::make_unique<MenuScene>(); });

  SceneManager::InteractionHandlers interactions;
  interactions.startDialogue = [](const std::string& speaker, const std::vector<std::string>& lines)
  {
    DialogueManager::Start(speaker, lines);
  };
  interactions.tryPickUp = [](const SceneEntity& entity)
  {
    return InventoryManager::GetInstance().AddItem(MakeInventoryItem(entity));
  };
  interactions.rollbackPickUp = [](const SceneEntity&)
  {
    auto& inventory = InventoryManager::GetInstance();
    if (inventory.GetItemCount() > 0) inventory.RemoveItem(inventory.GetItemCount() - 1);
  };
  interactions.reset = [] { DialogueManager::Close(); };
  SceneManager::SetInteractionHandlers(std::move(interactions));
}
