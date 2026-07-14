#pragma once

#include "SceneManager.h"

#include "../input/UserInput.h"
#include "AudioManager.h"
#include "LightManager.h"
#include "Renderer.h"
#include "Settings.h"
#include "Window.h"
#include "FontManager.h"

#include <array>
#include <algorithm>

struct GameScene : Scene
{
  GameScene() : Scene("game") {}

  void OnSceneStart() override
  {
    Window::SetCursorVisible(false);
    AudioManager::PlayMusic("night_mono",glm::vec3(25,2,15),true);

    const glm::uvec2 currentResolution(Settings::GetWindowWidth(), Settings::GetWindowHeight());
    for (size_t i = 0; i < m_PauseResolutions.size(); ++i)
      if (m_PauseResolutions[i] == currentResolution) m_PauseResolutionIndex = static_cast<int>(i);

    m_PauseModeIndex = static_cast<int>(Settings::GetWindowMode());
    const auto fps = std::find(m_PauseFPSLimits.begin(), m_PauseFPSLimits.end(), Settings::GetFPSLimit());
    if (fps != m_PauseFPSLimits.end())
      m_PauseFPSIndex = static_cast<int>(std::distance(m_PauseFPSLimits.begin(), fps));

    LightManager::AddLight(LightType::DIRECT,glm::vec3(0.3,0.32,0.4),glm::vec3(0),glm::vec3(-2,-4,-1));
  }

  void OnUpdate(DeltaTime& dt) override
  {
    const bool escape = Pressed(Input::IsKeyPressed(Key::Escape), m_PreviousEscape);
    const bool start = Pressed(Input::IsGamepadButtonPressed(Gamepad::Start), m_PreviousStart);
    bool justPaused = false;

    if (!m_Paused && (escape || start))
    {
      SetPaused(true);
      justPaused = true;
    }
    else if (m_Paused && start)
      SetPaused(false);

    if (m_Paused)
    {
      Renderer::DrawPausedFrame();
      UpdatePauseMenu(escape && !justPaused);
      if (m_Paused)
        DrawPauseMenu();
      return;
    }

    Renderer::DrawScene(dt,[&]()
    {
      if (Input::IsKeyPressed(Key::W) ||
          Input::IsKeyPressed(Key::S) ||
          Input::IsKeyPressed(Key::A) ||
          Input::IsKeyPressed(Key::D))
      {
        ModelManager::GetModel("harry")->StartBlendToAnimation(1,0.8f);
      }
      else
      {
        ModelManager::GetModel("harry")->StartBlendToAnimation(0,0.8f);
      }

      if(Input::IsKeyPressed(Key::W)) ModelManager::MoveController("harry",Movement::FORWARD,10.0f,dt);
      if(Input::IsKeyPressed(Key::S)) ModelManager::MoveController("harry",Movement::BACKWARD,10.0f,dt);
      if(Input::IsKeyPressed(Key::A)) ModelManager::MoveController("harry",Movement::LEFT,10.0f,dt);
      if(Input::IsKeyPressed(Key::D)) ModelManager::MoveController("harry",Movement::RIGHT,10.0f,dt);
    });
  }

private:
  enum class PauseScreen { Main, Options };

  static bool Pressed(bool current, bool& previous)
  {
    const bool result = current && !previous;
    previous = current;
    return result;
  }

  static int Wrap(int value, int count)
  {
    return (value % count + count) % count;
  }

  void SetPaused(bool paused)
  {
    m_Paused = paused;
    m_PauseScreen = PauseScreen::Main;
    m_PauseSelected = 0;
    Window::SetCursorVisible(paused);
    if (paused)
      AudioManager::PauseMusic("night_mono");
    else
      AudioManager::ResumeMusic("night_mono");
  }

  void UpdatePauseMenu(bool escape)
  {
    const bool up = Pressed(
      Input::IsKeyPressed(Key::Up) || Input::IsKeyPressed(Key::W) ||
      Input::IsGamepadButtonPressed(Gamepad::DPadUp) || Input::GetGamepadAxis(Gamepad::LeftY) < -0.6f,
      m_PausePreviousUp);
    const bool down = Pressed(
      Input::IsKeyPressed(Key::Down) || Input::IsKeyPressed(Key::S) ||
      Input::IsGamepadButtonPressed(Gamepad::DPadDown) || Input::GetGamepadAxis(Gamepad::LeftY) > 0.6f,
      m_PausePreviousDown);
    const bool left = Pressed(
      Input::IsKeyPressed(Key::Left) || Input::IsKeyPressed(Key::A) ||
      Input::IsGamepadButtonPressed(Gamepad::DPadLeft) || Input::GetGamepadAxis(Gamepad::LeftX) < -0.6f,
      m_PausePreviousLeft);
    const bool right = Pressed(
      Input::IsKeyPressed(Key::Right) || Input::IsKeyPressed(Key::D) ||
      Input::IsGamepadButtonPressed(Gamepad::DPadRight) || Input::GetGamepadAxis(Gamepad::LeftX) > 0.6f,
      m_PausePreviousRight);
    const bool accept = Pressed(
      Input::IsKeyPressed(Key::Enter) || Input::IsKeyPressed(Key::Space) ||
      Input::IsGamepadButtonPressed(Gamepad::A),
      m_PausePreviousAccept);
    const bool back = escape || Pressed(Input::IsGamepadButtonPressed(Gamepad::B), m_PausePreviousBack);
    const bool mouseClick = Pressed(Input::IsMouseButtonPressed(Mouse::ButtonLeft), m_PausePreviousMouse);

    const int itemCount = m_PauseScreen == PauseScreen::Main ? 4 : 7;
    if (up) m_PauseSelected = Wrap(m_PauseSelected - 1, itemCount);
    if (down) m_PauseSelected = Wrap(m_PauseSelected + 1, itemCount);

    const float screenWidth = static_cast<float>(Window::GetWidth());
    const float screenHeight = static_cast<float>(Window::GetHeight());
    const float rowWidth = std::min(520.0f, screenWidth * 0.52f);
    const float rowHeight = std::clamp(screenHeight * 0.065f, 38.0f, 54.0f);
    const float startY = screenHeight * 0.65f;
    const float spacing = rowHeight + 8.0f;
    const float x = (screenWidth - rowWidth) * 0.5f;
    const glm::vec2 mouse(Input::GetMouseX(), screenHeight - Input::GetMouseY());

    int clicked = -1;
    int mouseDirection = 0;
    for (int i = 0; i < itemCount; ++i)
    {
      const float y = startY - i * spacing;
      if (mouse.x >= x && mouse.x <= x + rowWidth && mouse.y >= y && mouse.y <= y + rowHeight)
      {
        m_PauseSelected = i;
        if (mouseClick)
        {
          clicked = i;
          mouseDirection = mouse.x < x + rowWidth * 0.5f ? -1 : 1;
        }
      }
    }

    if (m_PauseScreen == PauseScreen::Main)
    {
      if (back)
      {
        SetPaused(false);
        return;
      }
      if (accept || clicked >= 0)
        ActivatePauseItem(clicked >= 0 ? clicked : m_PauseSelected);
      return;
    }

    if (back)
    {
      Settings::Save();
      m_PauseScreen = PauseScreen::Main;
      m_PauseSelected = 1;
      return;
    }

    if (left) ChangePauseOption(m_PauseSelected, -1);
    if (right) ChangePauseOption(m_PauseSelected, 1);
    if (accept || clicked >= 0)
    {
      const int activated = clicked >= 0 ? clicked : m_PauseSelected;
      if (activated == 6)
      {
        Settings::Save();
        m_PauseScreen = PauseScreen::Main;
        m_PauseSelected = 1;
      }
      else
      {
        const int direction = clicked >= 0 ? mouseDirection : (activated == 5 ? 0 : 1);
        ChangePauseOption(activated, direction);
      }
    }
  }

  void ActivatePauseItem(int index)
  {
    switch (index)
    {
      case 0:
        SetPaused(false);
        break;
      case 1:
        m_PauseScreen = PauseScreen::Options;
        m_PauseSelected = 0;
        break;
      case 2:
        Settings::Save();
        AudioManager::StopMusic("night_mono");
        m_Paused = false;
        Window::SetCursorVisible(true);
        SceneManager::LoadScene("menu");
        break;
      case 3:
        Settings::Save();
        Window::RequestClose();
        break;
      default:
        break;
    }
  }

  void ChangePauseOption(int index, int direction)
  {
    switch (index)
    {
      case 0:
      {
        const float value = std::clamp(Settings::GetMusicVolume() + direction * 0.1f, 0.0f, 1.0f);
        Settings::SetMusicVolume(value);
        AudioManager::SetMusicVolume(value);
        break;
      }
      case 1:
      {
        const float value = std::clamp(Settings::GetSFXVolume() + direction * 0.1f, 0.0f, 1.0f);
        Settings::SetSFXVolume(value);
        AudioManager::SetSFXVolume(value);
        break;
      }
      case 2:
        m_PauseResolutionIndex = Wrap(m_PauseResolutionIndex + direction, static_cast<int>(m_PauseResolutions.size()));
        Settings::SetResolution(m_PauseResolutions[m_PauseResolutionIndex].x, m_PauseResolutions[m_PauseResolutionIndex].y);
        Renderer::ApplyDisplaySettings();
        break;
      case 3:
        m_PauseModeIndex = Wrap(m_PauseModeIndex + direction, 3);
        Settings::SetWindowMode(static_cast<WindowMode>(m_PauseModeIndex));
        Renderer::ApplyDisplaySettings();
        break;
      case 4:
        m_PauseFPSIndex = Wrap(m_PauseFPSIndex + direction, static_cast<int>(m_PauseFPSLimits.size()));
        Settings::SetFPSLimit(m_PauseFPSLimits[m_PauseFPSIndex]);
        break;
      case 5:
        Settings::SetVSync(direction == 0 ? !Settings::GetVSync() : direction > 0);
        Window::SetVSync(Settings::GetVSync());
        break;
      default:
        return;
    }
    Settings::Save();
  }

  std::string PauseOptionLabel(int index) const
  {
    static constexpr std::array<const char*, 3> modes = {"WINDOWED", "FULLSCREEN", "BORDERLESS"};
    switch (index)
    {
      case 0: return "MUSIC VOLUME    < " + std::to_string(static_cast<int>(Settings::GetMusicVolume() * 100.0f + 0.5f)) + "% >";
      case 1: return "SFX VOLUME      < " + std::to_string(static_cast<int>(Settings::GetSFXVolume() * 100.0f + 0.5f)) + "% >";
      case 2: return "RESOLUTION      < " + std::to_string(m_PauseResolutions[m_PauseResolutionIndex].x) + " x " + std::to_string(m_PauseResolutions[m_PauseResolutionIndex].y) + " >";
      case 3: return "DISPLAY MODE    < " + std::string(modes[m_PauseModeIndex]) + " >";
      case 4: return "FPS LIMIT       < " + (m_PauseFPSLimits[m_PauseFPSIndex] == 0 ? std::string("UNLIMITED") : std::to_string(m_PauseFPSLimits[m_PauseFPSIndex])) + " >";
      case 5: return "VSYNC           < " + std::string(Settings::GetVSync() ? "ON" : "OFF") + " >";
      default: return "BACK";
    }
  }

  void DrawPauseMenu()
  {
    static constexpr std::array<const char*, 4> items = {"RESUME", "OPTIONS", "MAIN MENU", "EXIT"};
    const float screenWidth = static_cast<float>(Window::GetWidth());
    const float screenHeight = static_cast<float>(Window::GetHeight());
    const float rowHeight = std::clamp(screenHeight * 0.065f, 38.0f, 54.0f);
    const float startY = screenHeight * 0.65f;
    const float spacing = rowHeight + 8.0f;
    const Font* font = FontManager::GetFont("dpcomic");

    Renderer::BeginScene();
    Renderer::DrawText(font, m_PauseScreen == PauseScreen::Main ? "PAUSED" : "OPTIONS",
      glm::vec2(screenWidth * 0.5f, screenHeight * 0.8f), 1.1f, glm::vec4(1.0f));

    const int count = m_PauseScreen == PauseScreen::Main ? 4 : 7;
    for (int i = 0; i < count; ++i)
    {
      const std::string label = m_PauseScreen == PauseScreen::Main ? std::string(items[i]) : PauseOptionLabel(i);
      Renderer::DrawText(font, label,
        glm::vec2(screenWidth * 0.5f, startY - i * spacing + rowHeight * 0.5f), 0.58f,
        i == m_PauseSelected ? glm::vec4(1.0f) : glm::vec4(0.62f, 0.68f, 0.76f, 1.0f));
    }

    Renderer::DrawText(font,
      m_PauseScreen == PauseScreen::Main
        ? "ESC / START: RESUME    ENTER / A: SELECT"
        : "LEFT / RIGHT: CHANGE    ESC / B: BACK",
      glm::vec2(screenWidth * 0.5f, 34.0f), 0.3f, glm::vec4(0.7f));
    Renderer::EndScene();
  }

  bool m_Paused = false;
  PauseScreen m_PauseScreen = PauseScreen::Main;
  int m_PauseSelected = 0;
  int m_PauseResolutionIndex = 1;
  int m_PauseModeIndex = 0;
  int m_PauseFPSIndex = 2;
  bool m_PreviousEscape = false;
  bool m_PreviousStart = false;
  bool m_PausePreviousUp = false;
  bool m_PausePreviousDown = false;
  bool m_PausePreviousLeft = false;
  bool m_PausePreviousRight = false;
  bool m_PausePreviousAccept = false;
  bool m_PausePreviousBack = false;
  bool m_PausePreviousMouse = false;

  static constexpr std::array<glm::uvec2, 6> m_PauseResolutions = {
    glm::uvec2(1024, 576), glm::uvec2(1280, 720), glm::uvec2(1600, 900),
    glm::uvec2(1920, 1080), glm::uvec2(2560, 1440), glm::uvec2(3840, 2160)
  };
  static constexpr std::array<uint32_t, 7> m_PauseFPSLimits = {0, 30, 60, 75, 120, 144, 240};
};

struct MenuScene : Scene
{
  MenuScene() : Scene("menu") {}

  void OnSceneStart() override
  {
    Window::SetCursorVisible(true);
    AudioManager::PlayMusic("menu",true);

    const glm::uvec2 currentResolution(Settings::GetWindowWidth(), Settings::GetWindowHeight());
    for (size_t i = 0; i < m_Resolutions.size(); ++i)
      if (m_Resolutions[i] == currentResolution) m_ResolutionIndex = static_cast<int>(i);

    m_ModeIndex = static_cast<int>(Settings::GetWindowMode());
    const auto fps = std::find(m_FPSLimits.begin(), m_FPSLimits.end(), Settings::GetFPSLimit());
    if (fps != m_FPSLimits.end()) m_FPSIndex = static_cast<int>(std::distance(m_FPSLimits.begin(), fps));
  }

  void OnUpdate(DeltaTime& dt) override
  {
    (void)dt;
    AudioManager::UpdateAllMusic();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, Window::GetWidth(), Window::GetHeight());
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const bool navigateUp = Pressed(
      Input::IsKeyPressed(Key::Up) || Input::IsKeyPressed(Key::W) ||
      Input::IsGamepadButtonPressed(Gamepad::DPadUp) || Input::GetGamepadAxis(Gamepad::LeftY) < -0.6f,
      m_PreviousUp);
    const bool navigateDown = Pressed(
      Input::IsKeyPressed(Key::Down) || Input::IsKeyPressed(Key::S) ||
      Input::IsGamepadButtonPressed(Gamepad::DPadDown) || Input::GetGamepadAxis(Gamepad::LeftY) > 0.6f,
      m_PreviousDown);
    const bool navigateLeft = Pressed(
      Input::IsKeyPressed(Key::Left) || Input::IsKeyPressed(Key::A) ||
      Input::IsGamepadButtonPressed(Gamepad::DPadLeft) || Input::GetGamepadAxis(Gamepad::LeftX) < -0.6f,
      m_PreviousLeft);
    const bool navigateRight = Pressed(
      Input::IsKeyPressed(Key::Right) || Input::IsKeyPressed(Key::D) ||
      Input::IsGamepadButtonPressed(Gamepad::DPadRight) || Input::GetGamepadAxis(Gamepad::LeftX) > 0.6f,
      m_PreviousRight);
    const bool accept = Pressed(
      Input::IsKeyPressed(Key::Enter) || Input::IsKeyPressed(Key::Space) ||
      Input::IsGamepadButtonPressed(Gamepad::A),
      m_PreviousAccept);
    const bool back = Pressed(
      Input::IsKeyPressed(Key::Escape) || Input::IsGamepadButtonPressed(Gamepad::B),
      m_PreviousBack);
    const bool mouseClick = Pressed(Input::IsMouseButtonPressed(Mouse::ButtonLeft), m_PreviousMouse);

    const int itemCount = m_Screen == Screen::Main ? 4 : 7;
    if (navigateUp) m_Selected = (m_Selected + itemCount - 1) % itemCount;
    if (navigateDown) m_Selected = (m_Selected + 1) % itemCount;

    const float width = static_cast<float>(Window::GetWidth());
    const float height = static_cast<float>(Window::GetHeight());
    const glm::vec2 mouse(Input::GetMouseX(), height - Input::GetMouseY());
    const float buttonWidth = std::min(520.0f, width * 0.52f);
    const float buttonHeight = std::clamp(height * 0.065f, 38.0f, 54.0f);
    const float startY = height * 0.65f;
    const float spacing = buttonHeight + 8.0f;
    const float buttonX = (width - buttonWidth) * 0.5f;

    int mouseActivated = -1;
    int mouseDirection = 0;
    for (int i = 0; i < itemCount; ++i)
    {
      const float y = startY - i * spacing;
      const bool hovered = mouse.x >= buttonX && mouse.x <= buttonX + buttonWidth &&
        mouse.y >= y && mouse.y <= y + buttonHeight;
      if (hovered)
      {
        m_Selected = i;
        if (mouseClick)
        {
          mouseActivated = i;
          mouseDirection = mouse.x < buttonX + buttonWidth * 0.5f ? -1 : 1;
        }
      }
    }

    if (m_Screen == Screen::Main)
    {
      if (accept || mouseActivated >= 0)
        ActivateMain(mouseActivated >= 0 ? mouseActivated : m_Selected);
      if (back)
        Window::RequestClose();
    }
    else
    {
      if (back)
      {
        Settings::Save();
        m_Screen = Screen::Main;
        m_Selected = 2;
      }
      else
      {
        if (navigateLeft) ChangeOption(m_Selected, -1);
        if (navigateRight) ChangeOption(m_Selected, 1);
        if (accept || mouseActivated >= 0)
        {
          const int activated = mouseActivated >= 0 ? mouseActivated : m_Selected;
          if (activated == 6)
          {
            Settings::Save();
            m_Screen = Screen::Main;
            m_Selected = 2;
          }
          else
          {
            const int direction = mouseActivated >= 0 ? mouseDirection : (activated == 5 ? 0 : 1);
            ChangeOption(activated, direction);
          }
        }
      }
    }

    DrawMenu(buttonX, startY, buttonWidth, buttonHeight, spacing, width, height);
  }

private:
  enum class Screen { Main, Options };

  static bool Pressed(bool current, bool& previous)
  {
    const bool result = current && !previous;
    previous = current;
    return result;
  }

  void ActivateMain(int index)
  {
    switch (index)
    {
      case 0:
      case 1:
        AudioManager::StopMusic("menu");
        Window::SetCursorVisible(false);
        SceneManager::LoadScene("game");
        break;
      case 2:
        m_Screen = Screen::Options;
        m_Selected = 0;
        break;
      case 3:
        Settings::Save();
        Window::RequestClose();
        break;
      default:
        break;
    }
  }

  void ChangeOption(int index, int direction)
  {
    switch (index)
    {
      case 0:
      {
        const float volume = std::clamp(Settings::GetMusicVolume() + direction * 0.1f, 0.0f, 1.0f);
        Settings::SetMusicVolume(volume);
        AudioManager::SetMusicVolume(volume);
        break;
      }
      case 1:
      {
        const float volume = std::clamp(Settings::GetSFXVolume() + direction * 0.1f, 0.0f, 1.0f);
        Settings::SetSFXVolume(volume);
        AudioManager::SetSFXVolume(volume);
        break;
      }
      case 2:
        m_ResolutionIndex = Wrap(m_ResolutionIndex + direction, static_cast<int>(m_Resolutions.size()));
        Settings::SetResolution(m_Resolutions[m_ResolutionIndex].x, m_Resolutions[m_ResolutionIndex].y);
        Renderer::ApplyDisplaySettings();
        break;
      case 3:
        m_ModeIndex = Wrap(m_ModeIndex + direction, 3);
        Settings::SetWindowMode(static_cast<WindowMode>(m_ModeIndex));
        Renderer::ApplyDisplaySettings();
        break;
      case 4:
        m_FPSIndex = Wrap(m_FPSIndex + direction, static_cast<int>(m_FPSLimits.size()));
        Settings::SetFPSLimit(m_FPSLimits[m_FPSIndex]);
        break;
      case 5:
        Settings::SetVSync(direction == 0 ? !Settings::GetVSync() : direction > 0);
        Window::SetVSync(Settings::GetVSync());
        break;
      default:
        return;
    }
    Settings::Save();
  }

  void DrawMenu(float x, float startY, float width, float height, float spacing, float screenWidth, float screenHeight)
  {
    static constexpr std::array<const char*, 4> mainItems = {"NEW GAME", "LOAD GAME", "OPTIONS", "EXIT"};
    const Font* font = FontManager::GetFont("dpcomic");

    Renderer::BeginScene();
    Renderer::DrawText(font, m_Screen == Screen::Main ? "GABGL" : "OPTIONS",
      glm::vec2(screenWidth * 0.5f, screenHeight * 0.79f), 1.25f, glm::vec4(0.82f, 0.9f, 1.0f, 1.0f));

    const int count = m_Screen == Screen::Main ? 4 : 7;
    for (int i = 0; i < count; ++i)
    {
      const float y = startY - i * spacing;
      const bool selected = i == m_Selected;
      const std::string label = m_Screen == Screen::Main ? std::string(mainItems[i]) : OptionLabel(i);
      Renderer::DrawText(font, label, glm::vec2(x + width * 0.5f, y + height * 0.5f), 0.58f,
        selected ? glm::vec4(1.0f) : glm::vec4(0.72f, 0.78f, 0.86f, 1.0f));
    }

    Renderer::DrawText(font,
      m_Screen == Screen::Main
        ? "UP / DOWN: NAVIGATE    ENTER / A: SELECT"
        : "UP / DOWN: SELECT    LEFT / RIGHT: CHANGE    ESC / B: BACK",
      glm::vec2(screenWidth * 0.5f, 34.0f), 0.3f, glm::vec4(0.55f, 0.62f, 0.72f, 1.0f));
    Renderer::EndScene();
  }

  std::string OptionLabel(int index) const
  {
    static constexpr std::array<const char*, 3> modeNames = {"WINDOWED", "FULLSCREEN", "BORDERLESS"};
    switch (index)
    {
      case 0: return "MUSIC VOLUME    < " + std::to_string(static_cast<int>(Settings::GetMusicVolume() * 100.0f + 0.5f)) + "% >";
      case 1: return "SFX VOLUME      < " + std::to_string(static_cast<int>(Settings::GetSFXVolume() * 100.0f + 0.5f)) + "% >";
      case 2: return "RESOLUTION      < " + std::to_string(m_Resolutions[m_ResolutionIndex].x) + " x " + std::to_string(m_Resolutions[m_ResolutionIndex].y) + " >";
      case 3: return "DISPLAY MODE    < " + std::string(modeNames[m_ModeIndex]) + " >";
      case 4: return "FPS LIMIT       < " + (m_FPSLimits[m_FPSIndex] == 0 ? std::string("UNLIMITED") : std::to_string(m_FPSLimits[m_FPSIndex])) + " >";
      case 5: return "VSYNC           < " + std::string(Settings::GetVSync() ? "ON" : "OFF") + " >";
      default: return "BACK";
    }
  }

  static int Wrap(int value, int count)
  {
    return (value % count + count) % count;
  }

  Screen m_Screen = Screen::Main;
  int m_Selected = 0;
  int m_ResolutionIndex = 1;
  int m_ModeIndex = 0;
  int m_FPSIndex = 2;
  bool m_PreviousUp = false;
  bool m_PreviousDown = false;
  bool m_PreviousLeft = false;
  bool m_PreviousRight = false;
  bool m_PreviousAccept = false;
  bool m_PreviousBack = false;
  bool m_PreviousMouse = false;

  static constexpr std::array<glm::uvec2, 6> m_Resolutions = {
    glm::uvec2(1024, 576), glm::uvec2(1280, 720), glm::uvec2(1600, 900),
    glm::uvec2(1920, 1080), glm::uvec2(2560, 1440), glm::uvec2(3840, 2160)
  };
  static constexpr std::array<uint32_t, 7> m_FPSLimits = {0, 30, 60, 75, 120, 144, 240};
};
