#include "DialogueManager.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
  struct DialogueState
  {
    std::string speaker;
    std::vector<std::string> lines;
    size_t lineIndex = 0;
    size_t visibleBytes = 0;
    float characterAccumulator = 0.0f;
    bool active = false;
  };

  constexpr float CharactersPerSecond = 40.0f;
  DialogueState s_Dialogue;
  const std::string s_Empty;

  size_t NextUtf8CharacterEnd(const std::string& text, size_t byteIndex)
  {
    if (byteIndex >= text.size()) return text.size();

    const unsigned char lead = static_cast<unsigned char>(text[byteIndex]);
    size_t length = 1;
    if ((lead & 0xE0u) == 0xC0u) length = 2;
    else if ((lead & 0xF0u) == 0xE0u) length = 3;
    else if ((lead & 0xF8u) == 0xF0u) length = 4;

    if (byteIndex + length > text.size()) return byteIndex + 1;
    for (size_t offset = 1; offset < length; ++offset)
      if ((static_cast<unsigned char>(text[byteIndex + offset]) & 0xC0u) != 0x80u)
        return byteIndex + 1;
    return byteIndex + length;
  }

  void ResetLineReveal()
  {
    s_Dialogue.visibleBytes = 0;
    s_Dialogue.characterAccumulator = 0.0f;
  }
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
  ResetLineReveal();
  return true;
}

void DialogueManager::Update(float deltaSeconds)
{
  if (!s_Dialogue.active || !std::isfinite(deltaSeconds) || deltaSeconds <= 0.0f)
    return;

  const std::string& line = s_Dialogue.lines[s_Dialogue.lineIndex];
  if (s_Dialogue.visibleBytes >= line.size()) return;

  s_Dialogue.characterAccumulator += deltaSeconds * CharactersPerSecond;
  const size_t charactersToReveal = static_cast<size_t>(std::floor(std::min(
    s_Dialogue.characterAccumulator, static_cast<float>(line.size()))));
  s_Dialogue.characterAccumulator -= static_cast<float>(charactersToReveal);

  for (size_t character = 0;
       character < charactersToReveal && s_Dialogue.visibleBytes < line.size();
       ++character)
    s_Dialogue.visibleBytes = NextUtf8CharacterEnd(line, s_Dialogue.visibleBytes);
}

bool DialogueManager::Advance()
{
  if (!s_Dialogue.active) return false;
  if (!IsCurrentLineFullyVisible())
  {
    s_Dialogue.visibleBytes = s_Dialogue.lines[s_Dialogue.lineIndex].size();
    s_Dialogue.characterAccumulator = 0.0f;
    return true;
  }
  if (++s_Dialogue.lineIndex < s_Dialogue.lines.size())
  {
    ResetLineReveal();
    return true;
  }

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

bool DialogueManager::IsCurrentLineFullyVisible()
{
  return s_Dialogue.active && s_Dialogue.lineIndex < s_Dialogue.lines.size() &&
    s_Dialogue.visibleBytes >= s_Dialogue.lines[s_Dialogue.lineIndex].size();
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

std::string DialogueManager::GetVisibleLine()
{
  if (!s_Dialogue.active || s_Dialogue.lineIndex >= s_Dialogue.lines.size())
    return {};
  return s_Dialogue.lines[s_Dialogue.lineIndex].substr(0, s_Dialogue.visibleBytes);
}

size_t DialogueManager::GetLineIndex()
{
  return s_Dialogue.active ? s_Dialogue.lineIndex : 0;
}

size_t DialogueManager::GetLineCount()
{
  return s_Dialogue.active ? s_Dialogue.lines.size() : 0;
}
