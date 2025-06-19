#pragma once

enum class LoadState {
    NotStarted,
    LoadingAssets,
    UploadingAssets,
    Finalizing,
    Done
};

static LoadState s_LoadState = LoadState::NotStarted;

static const char* ToString(LoadState state) {
    switch(state) {
        case LoadState::NotStarted:      return "NotStarted";
        case LoadState::LoadingAssets:   return "LoadingAssets";
        case LoadState::UploadingAssets: return "UploadingAssets";
        case LoadState::Finalizing:      return "Finalizing";
        case LoadState::Done:            return "Done";
        default:                        return "Unknown";
    }
}
struct AssetManager
{
  static void StartLoadingAssets();
  static void UpdateLoading();
  static bool LoadingComplete();
};
