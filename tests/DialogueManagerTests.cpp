#include "DialogueManager.h"

#include <cassert>
#include <string>
#include <vector>

int main()
{
  DialogueManager::Close();
  assert(!DialogueManager::IsActive());
  assert(!DialogueManager::Start("Nobody", {}));

  const std::vector<std::string> lines = {"First line", "", "Last line"};
  assert(DialogueManager::Start("NPC", lines));
  assert(DialogueManager::IsActive());
  assert(DialogueManager::GetSpeaker() == "NPC");
  assert(DialogueManager::GetCurrentLine() == "First line");
  assert(DialogueManager::GetVisibleLine().empty());
  assert(!DialogueManager::IsCurrentLineFullyVisible());
  assert(DialogueManager::GetLineIndex() == 0);
  assert(DialogueManager::GetLineCount() == 2);

  DialogueManager::Update(0.1f);
  assert(DialogueManager::GetVisibleLine() == "Firs");
  assert(DialogueManager::Advance());
  assert(DialogueManager::GetCurrentLine() == "First line");
  assert(DialogueManager::GetVisibleLine() == "First line");
  assert(DialogueManager::IsCurrentLineFullyVisible());

  assert(DialogueManager::Advance());
  assert(DialogueManager::GetCurrentLine() == "Last line");
  assert(DialogueManager::GetVisibleLine().empty());
  assert(DialogueManager::GetLineIndex() == 1);

  DialogueManager::Update(1.0f);
  assert(DialogueManager::GetVisibleLine() == "Last line");
  assert(!DialogueManager::Advance());
  assert(!DialogueManager::IsActive());
  assert(DialogueManager::GetCurrentLine().empty());
  assert(DialogueManager::GetVisibleLine().empty());
  assert(DialogueManager::GetSpeaker().empty());

  assert(DialogueManager::Start("NPC", {"Zażółć"}));
  DialogueManager::Update(0.1f);
  assert(DialogueManager::GetVisibleLine() == "Zażó");
  DialogueManager::Close();
  return 0;
}
