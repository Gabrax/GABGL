#pragma once

#include <cstddef>
#include <string>
#include <vector>

struct DialogueManager
{
  static bool Start(const std::string& speaker, const std::vector<std::string>& lines);
  static void Update(float deltaSeconds);
  static bool Advance();
  static void Close();

  static bool IsActive();
  static bool IsCurrentLineFullyVisible();
  static const std::string& GetSpeaker();
  static const std::string& GetCurrentLine();
  static std::string GetVisibleLine();
  static size_t GetLineIndex();
  static size_t GetLineCount();
};
