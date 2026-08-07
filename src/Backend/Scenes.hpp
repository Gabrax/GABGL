#pragma once

#include "SceneManager.h"

#include "../input/UserInput.h"
#include "AudioManager.h"
#include "LightManager.h"
#include "Renderer.h"
#include "Settings.h"
#include "Window.h"
#include "FontManager.h"
#include "ParticleRenderer.h"
#include "PhysX.h"

#include <array>
#include <algorithm>
#include <cmath>

namespace SceneUI
{
  inline float Scale(float width, float height)
  {
    // All menu measurements use the same 1280x720 reference canvas. This keeps
    // text, rows and spacing in sync instead of scaling only their positions.
    return std::max(0.25f, std::min(width / 1280.0f, height / 720.0f));
  }

  inline float Saturate(float value)
  {
    return std::clamp(value, 0.0f, 1.0f);
  }

  inline float Smooth(float value)
  {
    value = Saturate(value);
    return value * value * (3.0f - 2.0f * value);
  }
}

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

  }

  void OnUpdate(DeltaTime& dt) override
  {
    m_PauseFrameDelta = std::clamp(dt.GetSeconds(), 0.0f, 0.05f);
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
      m_PauseTime += m_PauseFrameDelta;
      m_PauseReveal = std::min(1.0f, m_PauseReveal + m_PauseFrameDelta / 0.28f);
      UpdatePauseMenu(escape && !justPaused);
      if (SceneManager::IsLoading())
        return;

      if (m_Paused)
      {
        DeltaTime frozenTime(0.0f);
        Renderer::DrawScene(frozenTime, []() {}, false);
        DrawPauseMenu();
        return;
      }
    }

    UpdateInteractions();
    UpdateWeapon();

    Renderer::DrawScene(dt,[&]
    {
      const auto player = ModelManager::GetModel("harry");
      if (!player) return;

      if (Input::IsKeyPressed(Key::W) ||
          Input::IsKeyPressed(Key::S) ||
          Input::IsKeyPressed(Key::A) ||
          Input::IsKeyPressed(Key::D))
      {
        player->StartBlendToAnimation(1,0.8f);
      }
      else
      {
        player->StartBlendToAnimation(0,0.8f);
      }

      if(Input::IsKeyPressed(Key::W)) ModelManager::MoveController("harry",Movement::FORWARD,10.0f,dt);
      if(Input::IsKeyPressed(Key::S)) ModelManager::MoveController("harry",Movement::BACKWARD,10.0f,dt);
      if(Input::IsKeyPressed(Key::A)) ModelManager::MoveController("harry",Movement::LEFT,10.0f,dt);
      if(Input::IsKeyPressed(Key::D)) ModelManager::MoveController("harry",Movement::RIGHT,10.0f,dt);
    });
  }

private:
  enum class PauseScreen { Main, Options };

  void UpdateWeapon()
  {
    const bool firePressed = Pressed(Input::IsMouseButtonPressed(Mouse::ButtonLeft), m_PreviousFire);
    if (!firePressed || !Camera::IsAiming()) return;

    AudioManager::PlaySound("select2", 0.8f);

    const glm::vec3 direction = Camera::GetForwardDirection();
    const glm::vec3 origin = Camera::GetPosition() + direction * 0.15f;

    const auto player = ModelManager::GetModel("harry");
    const PxRigidActor* playerActor =
      player && player->GetController() ? player->GetController()->getActor() : nullptr;

    if (PhysicsRaycastHit hit; PhysX::Raycast(origin, direction, 250.0f, hit, playerActor, 12.0f))
      ParticleRenderer::EmitImpact(hit.position, hit.normal);
  }

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
    m_PreviousFire = Input::IsMouseButtonPressed(Mouse::ButtonLeft);
    m_PauseScreen = PauseScreen::Main;
    m_PauseSelected = 0;
    if (paused)
    {
      m_PauseTime = 0.0f;
      m_PauseReveal = 0.0f;
      m_PauseHighlightInitialized = false;
    }
    Window::SetCursorVisible(paused);
    Camera::ResetMouseDelta();
    if (paused) AudioManager::PauseMusic("night_mono");
    else AudioManager::ResumeMusic("night_mono");
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

    const int itemCount = m_PauseScreen == PauseScreen::Main ? 4 : 9;
    if (up) m_PauseSelected = Wrap(m_PauseSelected - 1, itemCount);
    if (down) m_PauseSelected = Wrap(m_PauseSelected + 1, itemCount);

    const auto screenWidth = static_cast<float>(Window::GetWidth());
    const auto screenHeight = static_cast<float>(Window::GetHeight());
    const float uiScale = SceneUI::Scale(screenWidth, screenHeight);
    const float rowWidth = std::min(520.0f * uiScale, screenWidth * 0.52f);
    const bool optionsScreen = m_PauseScreen == PauseScreen::Options;
    const float rowHeight = optionsScreen
      ? 36.0f * uiScale
      : 47.0f * uiScale;
    const float startY = screenHeight * (optionsScreen ? 0.70f : 0.65f);
    const float spacing = rowHeight + (optionsScreen ? 4.0f : 8.0f) * uiScale;
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
      SetPauseScreen(PauseScreen::Main, 1);
      return;
    }

    if (left) ChangePauseOption(m_PauseSelected, -1);
    if (right) ChangePauseOption(m_PauseSelected, 1);
    if (accept || clicked >= 0)
    {
      const int activated = clicked >= 0 ? clicked : m_PauseSelected;
      if (activated == 8)
      {
        Settings::Save();
        SetPauseScreen(PauseScreen::Main, 1);
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
        SetPauseScreen(PauseScreen::Options, 0);
        break;
      case 2:
        Settings::Save();
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
      case 6:
        Settings::SetShadowQuality(static_cast<GraphicsQuality>(Wrap(
          static_cast<int>(Settings::GetShadowQuality()) + direction, 4)));
        Renderer::ApplyGraphicsSettings();
        break;
      case 7:
        Settings::SetBloomQuality(static_cast<GraphicsQuality>(Wrap(
          static_cast<int>(Settings::GetBloomQuality()) + direction, 4)));
        break;
      default:
        return;
    }
    Settings::Save();
  }

  [[nodiscard]] std::string PauseOptionLabel(int index) const
  {
    static constexpr std::array<const char*, 3> modes = {"WINDOWED", "FULLSCREEN", "BORDERLESS"};
    static constexpr std::array<const char*, 4> qualities = {"OFF", "LOW", "MEDIUM", "HIGH"};
    switch (index)
    {
      case 0: return "MUSIC VOLUME    < " + std::to_string(static_cast<int>(Settings::GetMusicVolume() * 100.0f + 0.5f)) + "% >";
      case 1: return "SFX VOLUME      < " + std::to_string(static_cast<int>(Settings::GetSFXVolume() * 100.0f + 0.5f)) + "% >";
      case 2: return "RESOLUTION      < " + std::to_string(m_PauseResolutions[m_PauseResolutionIndex].x) + " x " + std::to_string(m_PauseResolutions[m_PauseResolutionIndex].y) + " >";
      case 3: return "DISPLAY MODE    < " + std::string(modes[m_PauseModeIndex]) + " >";
      case 4: return "FPS LIMIT       < " + (m_PauseFPSLimits[m_PauseFPSIndex] == 0 ? std::string("UNLIMITED") : std::to_string(m_PauseFPSLimits[m_PauseFPSIndex])) + " >";
      case 5: return "VSYNC           < " + std::string(Settings::GetVSync() ? "ON" : "OFF") + " >";
      case 6: return "SHADOW QUALITY  < " + std::string(qualities[static_cast<int>(Settings::GetShadowQuality())]) + " >";
      case 7: return "BLOOM QUALITY   < " + std::string(qualities[static_cast<int>(Settings::GetBloomQuality())]) + " >";
      default: return "BACK";
    }
  }

  void DrawPauseMenu()
  {
    static constexpr std::array<const char*, 4> items = {"RESUME", "OPTIONS", "MAIN MENU", "EXIT"};
    const auto screenWidth = static_cast<float>(Window::GetWidth());
    const auto screenHeight = static_cast<float>(Window::GetHeight());
    const float uiScale = SceneUI::Scale(screenWidth, screenHeight);
    const float rowWidth = std::min(520.0f * uiScale, screenWidth * 0.52f);
    const bool optionsScreen = m_PauseScreen == PauseScreen::Options;
    const float rowHeight = optionsScreen
      ? 36.0f * uiScale
      : 47.0f * uiScale;
    const float startY = screenHeight * (optionsScreen ? 0.70f : 0.65f);
    const float spacing = rowHeight + (optionsScreen ? 4.0f : 8.0f) * uiScale;
    const Font* font = FontManager::GetFont("dpcomic");
    const float reveal = SceneUI::Smooth(m_PauseReveal);

    Renderer::BeginScene();

    Renderer::DrawQuad(
      glm::vec2(screenWidth * 0.5f, screenHeight * 0.5f),
      glm::vec2(screenWidth, screenHeight), 0.0f,
      glm::vec4(0.008f, 0.012f, 0.025f, 0.64f * reveal));

    const float targetHighlightY = startY - m_PauseSelected * spacing + rowHeight * 0.5f;
    if (!m_PauseHighlightInitialized)
    {
      m_PauseHighlightY = targetHighlightY;
      m_PauseHighlightInitialized = true;
    }
    const float follow = 1.0f - std::exp(-15.0f * m_PauseFrameDelta);
    m_PauseHighlightY += (targetHighlightY - m_PauseHighlightY) * follow;
    Renderer::DrawQuad(
      glm::vec2(screenWidth * 0.5f, m_PauseHighlightY),
      glm::vec2(rowWidth * (0.82f + std::sin(m_PauseTime * 5.0f) * 0.015f), rowHeight * 0.82f),
      0.0f, glm::vec4(0.22f, 0.46f, 0.76f, 0.19f * reveal));

    Renderer::DrawText(font, m_PauseScreen == PauseScreen::Main ? "PAUSED" : "OPTIONS",
      glm::vec2(screenWidth * 0.5f,
        screenHeight * 0.8f + (1.0f - reveal) * 24.0f * uiScale + std::sin(m_PauseTime * 1.8f) * 2.0f * uiScale),
      (0.96f + reveal * 0.14f) * uiScale, glm::vec4(1.0f, 1.0f, 1.0f, reveal));

    const int count = m_PauseScreen == PauseScreen::Main ? 4 : 9;
    for (int i = 0; i < count; ++i)
    {
      const std::string label = m_PauseScreen == PauseScreen::Main ? std::string(items[i]) : PauseOptionLabel(i);
      const float itemReveal = SceneUI::Smooth(m_PauseReveal * 1.5f - i * 0.075f);
      const bool selected = i == m_PauseSelected;
      const float slide = (1.0f - itemReveal) * 48.0f * uiScale;
      const float pulse = selected ? 1.0f + std::sin(m_PauseTime * 5.5f) * 0.02f : 1.0f;
      const glm::vec3 color = selected ? glm::vec3(1.0f) : glm::vec3(0.62f, 0.68f, 0.76f);
      Renderer::DrawText(font, label,
        glm::vec2(screenWidth * 0.5f + slide, startY - i * spacing + rowHeight * 0.5f),
        0.58f * uiScale * pulse, glm::vec4(color, itemReveal));
    }

    Renderer::DrawText(font,
      m_PauseScreen == PauseScreen::Main
        ? "ESC / START: RESUME    ENTER / A: SELECT"
        : "LEFT / RIGHT: CHANGE    ESC / B: BACK",
      glm::vec2(screenWidth * 0.5f, 34.0f * uiScale), 0.3f * uiScale,
      glm::vec4(0.7f, 0.7f, 0.7f, reveal));
    Renderer::EndScene();
  }

  void SetPauseScreen(PauseScreen screen, int selected)
  {
    m_PauseScreen = screen;
    m_PauseSelected = selected;
    m_PauseReveal = 0.0f;
    m_PauseHighlightInitialized = false;
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
  bool m_PreviousFire = false;
  float m_PauseTime = 0.0f;
  float m_PauseFrameDelta = 0.0f;
  float m_PauseReveal = 0.0f;
  float m_PauseHighlightY = 0.0f;
  bool m_PauseHighlightInitialized = false;

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

    m_MenuTime = 0.0f;
    m_IntroProgress = 0.0f;
    m_ScreenReveal = 0.0f;
    m_HighlightInitialized = false;
  }

  void OnUpdate(DeltaTime& dt) override
  {
    m_FrameDelta = std::clamp(dt.GetSeconds(), 0.0f, 0.05f);
    m_MenuTime += m_FrameDelta;
    m_IntroProgress = std::min(1.0f, m_IntroProgress + m_FrameDelta / 0.75f);
    m_ScreenReveal = std::min(1.0f, m_ScreenReveal + m_FrameDelta / 0.38f);

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

    const int itemCount = m_Screen == Screen::Main ? 4 : 9;
    if (navigateUp) m_Selected = (m_Selected + itemCount - 1) % itemCount;
    if (navigateDown) m_Selected = (m_Selected + 1) % itemCount;

    const auto width = static_cast<float>(Window::GetWidth());
    const auto height = static_cast<float>(Window::GetHeight());
    const float uiScale = SceneUI::Scale(width, height);
    const glm::vec2 mouse(Input::GetMouseX(), height - Input::GetMouseY());
    const float buttonWidth = std::min(520.0f * uiScale, width * 0.52f);
    const bool optionsScreen = m_Screen == Screen::Options;
    const float buttonHeight = optionsScreen
      ? 36.0f * uiScale
      : 47.0f * uiScale;
    const float startY = height * (optionsScreen ? 0.70f : 0.65f);
    const float spacing = buttonHeight + (optionsScreen ? 4.0f : 8.0f) * uiScale;
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
        SetScreen(Screen::Main, 2);
      }
      else
      {
        if (navigateLeft) ChangeOption(m_Selected, -1);
        if (navigateRight) ChangeOption(m_Selected, 1);
        if (accept || mouseActivated >= 0)
        {
          const int activated = mouseActivated >= 0 ? mouseActivated : m_Selected;
          if (activated == 8)
          {
            Settings::Save();
            SetScreen(Screen::Main, 2);
          }
          else
          {
            const int direction = mouseActivated >= 0 ? mouseDirection : (activated == 5 ? 0 : 1);
            ChangeOption(activated, direction);
          }
        }
      }
    }

    // The input layout above belongs to the screen that was active at the
    // beginning of this frame. A click may have switched Main <-> Options, so
    // calculate the drawing layout again from the new state. Otherwise the new
    // screen is rendered with the previous screen's row size for one frame.
    const bool drawOptionsScreen = m_Screen == Screen::Options;
    const float drawButtonWidth = std::min(520.0f * uiScale, width * 0.52f);
    const float drawButtonHeight = (drawOptionsScreen ? 36.0f : 47.0f) * uiScale;
    const float drawStartY = height * (drawOptionsScreen ? 0.70f : 0.65f);
    const float drawSpacing = drawButtonHeight + (drawOptionsScreen ? 4.0f : 8.0f) * uiScale;
    const float drawButtonX = (width - drawButtonWidth) * 0.5f;
    DrawMenu(drawButtonX, drawStartY, drawButtonWidth, drawButtonHeight,
      drawSpacing, width, height);
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
        Window::SetCursorVisible(false);
        SceneManager::LoadScene("game");
        break;
      case 2:
        SetScreen(Screen::Options, 0);
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
      case 6:
        Settings::SetShadowQuality(static_cast<GraphicsQuality>(Wrap(
          static_cast<int>(Settings::GetShadowQuality()) + direction, 4)));
        Renderer::ApplyGraphicsSettings();
        break;
      case 7:
        Settings::SetBloomQuality(static_cast<GraphicsQuality>(Wrap(
          static_cast<int>(Settings::GetBloomQuality()) + direction, 4)));
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
    const float uiScale = SceneUI::Scale(screenWidth, screenHeight);
    const float intro = SceneUI::Smooth(m_IntroProgress);
    const float screenReveal = SceneUI::Smooth(m_ScreenReveal);

    Renderer::BeginScene();

    Renderer::DrawQuad(
      glm::vec2(screenWidth * 0.5f, screenHeight * 0.5f),
      glm::vec2(screenWidth, screenHeight), 0.0f,
      glm::vec4(0.012f, 0.018f, 0.035f, 1.0f));

    // Slowly drifting translucent columns give the otherwise flat menu some
    // depth without competing with the low-resolution presentation.
    for (int i = 0; i < 7; ++i)
    {
      const float columnWidth = (42.0f + static_cast<float>((i * 29) % 70)) * uiScale;
      const float travel = screenWidth + columnWidth * 2.0f;
      const float speed = (10.0f + static_cast<float>((i * 7) % 13)) * uiScale;
      float columnX = std::fmod(i * screenWidth / 6.0f + m_MenuTime * speed, travel) - columnWidth;
      Renderer::DrawQuad(
        glm::vec2(columnX, screenHeight * 0.5f),
        glm::vec2(columnWidth, screenHeight * 1.15f),
        (i % 2 == 0 ? -2.0f : 2.0f),
        glm::vec4(0.12f, 0.24f, 0.42f, 0.025f * intro));
    }

    const float targetHighlightY = startY - m_Selected * spacing + height * 0.5f;
    if (!m_HighlightInitialized)
    {
      m_HighlightY = targetHighlightY;
      m_HighlightInitialized = true;
    }
    const float highlightFollow = 1.0f - std::exp(-14.0f * m_FrameDelta);
    m_HighlightY += (targetHighlightY - m_HighlightY) * highlightFollow;
    const float highlightPulse = 1.0f + std::sin(m_MenuTime * 4.5f) * 0.025f;
    Renderer::DrawQuad(
      glm::vec2(x + width * 0.5f, m_HighlightY),
      glm::vec2(width * 0.82f * highlightPulse, height * 0.82f), 0.0f,
      glm::vec4(0.18f, 0.42f, 0.72f, 0.16f * screenReveal));
    Renderer::DrawQuad(
      glm::vec2(x + width * 0.09f, m_HighlightY),
      glm::vec2(4.0f * uiScale, height * 0.62f), 0.0f,
      glm::vec4(0.55f, 0.82f, 1.0f, 0.8f * screenReveal));

    const float titleFloat = std::sin(m_MenuTime * 1.7f) * 2.5f * uiScale;
    Renderer::DrawText(font, m_Screen == Screen::Main ? "GABGL" : "OPTIONS",
      glm::vec2(screenWidth * 0.5f, screenHeight * 0.79f + titleFloat + (1.0f - intro) * 28.0f * uiScale),
      (1.08f + intro * 0.17f) * uiScale,
      glm::vec4(0.82f, 0.9f, 1.0f, intro));
    Renderer::DrawQuad(
      glm::vec2(screenWidth * 0.5f, screenHeight * 0.735f),
      glm::vec2(210.0f * uiScale * intro, 2.0f * uiScale), 0.0f,
      glm::vec4(0.32f, 0.62f, 0.92f, 0.55f * intro));

    const int count = m_Screen == Screen::Main ? 4 : 9;
    for (int i = 0; i < count; ++i)
    {
      const float y = startY - i * spacing;
      const bool selected = i == m_Selected;
      const std::string label = m_Screen == Screen::Main ? std::string(mainItems[i]) : OptionLabel(i);
      const float itemReveal = SceneUI::Smooth(m_ScreenReveal * 1.45f - i * 0.075f);
      const float slide = (1.0f - itemReveal) * 65.0f * uiScale * (i % 2 == 0 ? 1.0f : -1.0f);
      const float pulse = selected ? 1.0f + std::sin(m_MenuTime * 5.5f) * 0.025f : 1.0f;
      const glm::vec3 color = selected ? glm::vec3(1.0f) : glm::vec3(0.72f, 0.78f, 0.86f);
      Renderer::DrawText(font, label,
        glm::vec2(x + width * 0.5f + slide, y + height * 0.5f),
        0.58f * uiScale * pulse, glm::vec4(color, itemReveal));
    }

    Renderer::DrawText(font,
      m_Screen == Screen::Main
        ? "UP / DOWN: NAVIGATE    ENTER / A: SELECT"
        : "UP / DOWN: SELECT    LEFT / RIGHT: CHANGE    ESC / B: BACK",
      glm::vec2(screenWidth * 0.5f, 34.0f * uiScale), 0.3f * uiScale,
      glm::vec4(0.55f, 0.62f, 0.72f, screenReveal));
    Renderer::EndScene();
  }

  void SetScreen(Screen screen, int selected)
  {
    m_Screen = screen;
    m_Selected = selected;
    m_ScreenReveal = 0.0f;
    m_HighlightInitialized = false;
  }

  [[nodiscard]] std::string OptionLabel(int index) const
  {
    static constexpr std::array<const char*, 3> modeNames = {"WINDOWED", "FULLSCREEN", "BORDERLESS"};
    static constexpr std::array<const char*, 4> qualityNames = {"OFF", "LOW", "MEDIUM", "HIGH"};
    switch (index)
    {
      case 0: return "MUSIC VOLUME    < " + std::to_string(static_cast<int>(Settings::GetMusicVolume() * 100.0f + 0.5f)) + "% >";
      case 1: return "SFX VOLUME      < " + std::to_string(static_cast<int>(Settings::GetSFXVolume() * 100.0f + 0.5f)) + "% >";
      case 2: return "RESOLUTION      < " + std::to_string(m_Resolutions[m_ResolutionIndex].x) + " x " + std::to_string(m_Resolutions[m_ResolutionIndex].y) + " >";
      case 3: return "DISPLAY MODE    < " + std::string(modeNames[m_ModeIndex]) + " >";
      case 4: return "FPS LIMIT       < " + (m_FPSLimits[m_FPSIndex] == 0 ? std::string("UNLIMITED") : std::to_string(m_FPSLimits[m_FPSIndex])) + " >";
      case 5: return "VSYNC           < " + std::string(Settings::GetVSync() ? "ON" : "OFF") + " >";
      case 6: return "SHADOW QUALITY  < " + std::string(qualityNames[static_cast<int>(Settings::GetShadowQuality())]) + " >";
      case 7: return "BLOOM QUALITY   < " + std::string(qualityNames[static_cast<int>(Settings::GetBloomQuality())]) + " >";
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
  float m_MenuTime = 0.0f;
  float m_FrameDelta = 0.0f;
  float m_IntroProgress = 0.0f;
  float m_ScreenReveal = 0.0f;
  float m_HighlightY = 0.0f;
  bool m_HighlightInitialized = false;

  static constexpr std::array<glm::uvec2, 6> m_Resolutions = {
    glm::uvec2(1024, 576), glm::uvec2(1280, 720), glm::uvec2(1600, 900),
    glm::uvec2(1920, 1080), glm::uvec2(2560, 1440), glm::uvec2(3840, 2160)
  };
  static constexpr std::array<uint32_t, 7> m_FPSLimits = {0, 30, 60, 75, 120, 144, 240};
};
