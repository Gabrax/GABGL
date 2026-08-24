#include "DialogueManager.h"

#include <utility>

namespace
{
  struct DialogueState
  {
    std::string speaker;
    std::vector<std::string> lines;
    size_t lineIndex = 0;
    bool active = false;
  };

  DialogueState s_Dialogue;
  const std::string s_Empty;
}

bool DialogueManager::Start(const std::string& speaker, const std::vector<std::string>& lines)
{
  std::vector<std::string> usableLines;
  usableLines.reserve(lines.size());
  for (const auto& line : lines)
    if (!line.empty()) usableLines.push_back(line);

  if (usableLines.empty()) return false;

  s_Dialogue.speaker = speaker;
  s_Dialogue.lines = std::move(usableLines);
  s_Dialogue.lineIndex = 0;
  s_Dialogue.active = true;
  return true;
}

bool DialogueManager::Advance()
{
  if (!s_Dialogue.active) return false;
  if (++s_Dialogue.lineIndex < s_Dialogue.lines.size()) return true;

  Close();
  return false;
}

void DialogueManager::Close()
{
  s_Dialogue = {};
}

bool DialogueManager::IsActive()
{
  return s_Dialogue.active;
}

const std::string& DialogueManager::GetSpeaker()
{
  return s_Dialogue.active ? s_Dialogue.speaker : s_Empty;
}

const std::string& DialogueManager::GetCurrentLine()
{
  return s_Dialogue.active && s_Dialogue.lineIndex < s_Dialogue.lines.size()
    ? s_Dialogue.lines[s_Dialogue.lineIndex]
    : s_Empty;
}

size_t DialogueManager::GetLineIndex()
{
  return s_Dialogue.active ? s_Dialogue.lineIndex : 0;
}

size_t DialogueManager::GetLineCount()
{
  return s_Dialogue.active ? s_Dialogue.lines.size() : 0;
}
