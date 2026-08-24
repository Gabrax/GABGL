#pragma once

#include <cstddef>
#include <string>
#include <vector>

struct DialogueManager
{
  static bool Start(const std::string& speaker, const std::vector<std::string>& lines);
  static bool Advance();
  static void Close();

  static bool IsActive();
  static const std::string& GetSpeaker();
  static const std::string& GetCurrentLine();
  static size_t GetLineIndex();
  static size_t GetLineCount();
};
