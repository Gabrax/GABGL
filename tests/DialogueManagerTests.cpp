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
  assert(DialogueManager::GetLineIndex() == 0);
  assert(DialogueManager::GetLineCount() == 2);

  assert(DialogueManager::Advance());
  assert(DialogueManager::GetCurrentLine() == "Last line");
  assert(DialogueManager::GetLineIndex() == 1);

  assert(!DialogueManager::Advance());
  assert(!DialogueManager::IsActive());
  assert(DialogueManager::GetCurrentLine().empty());
  assert(DialogueManager::GetSpeaker().empty());
  return 0;
}
