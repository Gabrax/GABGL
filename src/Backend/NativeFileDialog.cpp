#include "NativeFileDialog.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <commdlg.h>
#endif

#include <array>

std::string NativeFileDialog::OpenModelFile()
{
#ifdef _WIN32
  std::array<char, 4096> selectedPath{};
  OPENFILENAMEA dialog{};
  dialog.lStructSize = sizeof(dialog);
  dialog.lpstrFile = selectedPath.data();
  dialog.nMaxFile = static_cast<DWORD>(selectedPath.size());
  dialog.lpstrFilter = "3D Models\0*.glb;*.gltf;*.fbx;*.obj;*.dae;*.3ds\0All Files\0*.*\0";
  dialog.nFilterIndex = 1;
  dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
  return GetOpenFileNameA(&dialog) ? std::string(selectedPath.data()) : std::string();
#else
  return {};
#endif
}
