#include "nemisis/ui/GameMenu.hpp"

#include "nemisis/input/InputBindings.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

namespace nemisis::ui {

namespace {

constexpr std::array<std::string_view, 3> kMainMenuItems{
    "1 DEV SHOOTING RANGE",
    "2 TEAM DEATHMATCH",
    "3 CONTROL",
};

[[nodiscard]] novacore::platform::InputActionState state(
    const novacore::platform::InputActionMap& actions,
    std::string_view action) {
    return actions.stateOrDefault(action);
}

[[nodiscard]] bool pressed(const novacore::platform::InputActionMap& actions, std::string_view action) {
    return state(actions, action).pressed;
}

void addRect(
    novacore::render::RenderFrameInfo& frame,
    float x,
    float y,
    float width,
    float height,
    std::array<float, 4> color) {
    frame.debugRects.push_back(novacore::render::DebugRect{x, y, width, height, color});
}

void addText(
    novacore::render::RenderFrameInfo& frame,
    float x,
    float y,
    float scale,
    std::array<float, 4> color,
    std::string text) {
    frame.debugTexts.push_back(novacore::render::DebugText{x, y, scale, color, std::move(text)});
}

void addHeader(novacore::render::RenderFrameInfo& frame, std::string_view title, std::string_view subtitle) {
    addText(frame, 54.0F, 42.0F, 5.0F, {0.88F, 0.94F, 1.0F, 1.0F}, "NEMISIS");
    addText(frame, 58.0F, 90.0F, 2.0F, {0.62F, 0.78F, 0.86F, 1.0F}, std::string(title));
    addText(frame, 58.0F, 116.0F, 2.0F, {0.46F, 0.56F, 0.62F, 1.0F}, std::string(subtitle));
}

void addMetric(
    novacore::render::RenderFrameInfo& frame,
    float x,
    float y,
    std::string label,
    std::string value) {
    addText(frame, x, y, 2.0F, {0.48F, 0.58F, 0.64F, 1.0F}, std::move(label));
    addText(frame, x + 168.0F, y, 2.0F, {0.9F, 0.96F, 0.98F, 1.0F}, std::move(value));
}

[[nodiscard]] std::string fixedOne(float value) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(1) << value;
    return stream.str();
}

} // namespace

void GameMenu::update(const novacore::platform::InputActionMap& actions) {
    if (pressed(actions, input::actions::ToggleDebug)) {
        debugOverlayEnabled_ = !debugOverlayEnabled_;
    }

    if (pressed(actions, input::actions::SelectDevRange)) {
        setScreen(GameScreen::DevRange);
        return;
    }
    if (pressed(actions, input::actions::SelectTdm)) {
        setScreen(GameScreen::TeamDeathmatch);
        return;
    }
    if (pressed(actions, input::actions::SelectControl)) {
        setScreen(GameScreen::Control);
        return;
    }

    if (screen_ != GameScreen::MainMenu) {
        if (pressed(actions, input::actions::MenuBack) || pressed(actions, input::actions::SelectMainMenu)) {
            setScreen(GameScreen::MainMenu);
        }
        return;
    }

    if (pressed(actions, input::actions::MenuDown)) {
        selectedIndex_ = (selectedIndex_ + 1) % kMainMenuItems.size();
    }
    if (pressed(actions, input::actions::MenuUp)) {
        selectedIndex_ = selectedIndex_ == 0 ? kMainMenuItems.size() - 1 : selectedIndex_ - 1;
    }
    if (pressed(actions, input::actions::MenuConfirm)) {
        activateSelection();
    }
}

void GameMenu::appendRenderCommands(
    novacore::render::RenderFrameInfo& frame,
    const nemisis::dev::DevSandboxSample& sample,
    std::string_view rendererBackend,
    std::size_t queuedAssets) const {
    addRect(frame, 0.0F, 0.0F, 1280.0F, 720.0F, {0.0F, 0.0F, 0.0F, 0.05F});
    addRect(frame, 38.0F, 30.0F, 510.0F, 146.0F, {0.025F, 0.045F, 0.055F, 0.92F});

    if (screen_ == GameScreen::MainMenu) {
        addHeader(frame, "MAIN MENU", "ARROWS/ENTER OR 1 2 3  -  F1 DEBUG");
        for (std::size_t index = 0; index < kMainMenuItems.size(); ++index) {
            const float y = 230.0F + static_cast<float>(index) * 72.0F;
            const bool selected = index == selectedIndex_;
            addRect(
                frame,
                70.0F,
                y - 17.0F,
                560.0F,
                48.0F,
                selected ? std::array<float, 4>{0.0F, 0.35F, 0.48F, 0.95F}
                         : std::array<float, 4>{0.055F, 0.075F, 0.085F, 0.88F});
            addText(
                frame,
                90.0F,
                y,
                3.0F,
                selected ? std::array<float, 4>{1.0F, 1.0F, 1.0F, 1.0F}
                         : std::array<float, 4>{0.64F, 0.74F, 0.78F, 1.0F},
                std::string(selected ? "> " : "  ") + std::string(kMainMenuItems[index]));
        }
        addText(frame, 72.0F, 570.0F, 2.0F, {0.55F, 0.62F, 0.66F, 1.0F}, "WINDOW IS LIVE - SDL DEBUG RENDER PATH");
    } else if (screen_ == GameScreen::DevRange) {
        addHeader(frame, "DEV SHOOTING RANGE", "ESC BACK  F1 DEBUG  MOUSE/RIGHT STICK LOOK");
        addRect(frame, 633.0F, 340.0F, 14.0F, 2.0F, {0.95F, 0.98F, 1.0F, 1.0F});
        addRect(frame, 640.0F, 333.0F, 2.0F, 14.0F, {0.95F, 0.98F, 1.0F, 1.0F});
        addRect(frame, 920.0F, 235.0F, 80.0F, 140.0F, {0.40F, 0.08F, 0.06F, 0.9F});
        addRect(frame, 937.0F, 204.0F, 46.0F, 46.0F, {0.82F, 0.18F, 0.14F, 0.95F});
        const float hpRatio = std::clamp(sample.target.health / sample.target.maxHealth, 0.0F, 1.0F);
        addRect(frame, 820.0F, 442.0F, 260.0F, 18.0F, {0.16F, 0.06F, 0.05F, 1.0F});
        addRect(frame, 820.0F, 442.0F, 260.0F * hpRatio, 18.0F, {0.85F, 0.14F, 0.10F, 1.0F});
        addText(frame, 820.0F, 472.0F, 2.0F, {0.92F, 0.94F, 0.95F, 1.0F}, "TARGET HP " + fixedOne(sample.target.health));
    } else {
        const bool tdm = screen_ == GameScreen::TeamDeathmatch;
        addHeader(frame, tdm ? "TEAM DEATHMATCH" : "CONTROL", "PLACEHOLDER MODE  -  ESC BACK");
        addRect(frame, 76.0F, 236.0F, 560.0F, 120.0F, {0.06F, 0.08F, 0.10F, 0.9F});
        addText(frame, 98.0F, 272.0F, 3.0F, {0.86F, 0.92F, 0.95F, 1.0F}, tdm ? "MATCH FLOW NEXT" : "OBJECTIVES NEXT");
        addText(frame, 100.0F, 322.0F, 2.0F, {0.52F, 0.62F, 0.68F, 1.0F}, "SPAWNS TEAMS SCOREBOARD SERVER AUTH");
    }

    if (debugOverlayEnabled_) {
        addRect(frame, 30.0F, 612.0F, 930.0F, 76.0F, {0.015F, 0.025F, 0.030F, 0.92F});
        addMetric(frame, 48.0F, 628.0F, "SCREEN", std::string(screenName()));
        addMetric(frame, 48.0F, 654.0F, "RENDERER", std::string(rendererBackend));
        addMetric(frame, 386.0F, 628.0F, "TICK", std::to_string(sample.tick));
        addMetric(frame, 386.0F, 654.0F, "ACK", std::to_string(sample.netBridge.lastAcknowledgedTick));
        addMetric(frame, 674.0F, 628.0F, "ASSETS", std::to_string(queuedAssets));
        addMetric(frame, 674.0F, 654.0F, "AMMO", std::to_string(sample.weapon.ammoInMagazine));
    }
}

GameScreen GameMenu::screen() const {
    return screen_;
}

bool GameMenu::gameplayActive() const {
    return screen_ == GameScreen::DevRange;
}

bool GameMenu::debugOverlayEnabled() const {
    return debugOverlayEnabled_;
}

std::string GameMenu::title() const {
    return "Nemisis - " + std::string(screenName()) + (debugOverlayEnabled_ ? " [F1 Debug]" : "");
}

std::string_view GameMenu::screenName() const {
    switch (screen_) {
    case GameScreen::MainMenu:
        return "Main Menu";
    case GameScreen::DevRange:
        return "Dev Range";
    case GameScreen::TeamDeathmatch:
        return "Team Deathmatch";
    case GameScreen::Control:
        return "Control";
    }
    return "Unknown";
}

std::array<float, 4> GameMenu::clearColor() const {
    switch (screen_) {
    case GameScreen::MainMenu:
        return {0.018F, 0.030F, 0.035F, 1.0F};
    case GameScreen::DevRange:
        return {0.028F, 0.040F, 0.052F, 1.0F};
    case GameScreen::TeamDeathmatch:
        return {0.035F, 0.025F, 0.022F, 1.0F};
    case GameScreen::Control:
        return {0.020F, 0.030F, 0.045F, 1.0F};
    }
    return {0.018F, 0.030F, 0.035F, 1.0F};
}

void GameMenu::activateSelection() {
    switch (selectedIndex_) {
    case 0:
        setScreen(GameScreen::DevRange);
        break;
    case 1:
        setScreen(GameScreen::TeamDeathmatch);
        break;
    case 2:
        setScreen(GameScreen::Control);
        break;
    default:
        setScreen(GameScreen::MainMenu);
        break;
    }
}

void GameMenu::setScreen(GameScreen screen) {
    screen_ = screen;
    if (screen_ == GameScreen::MainMenu) {
        selectedIndex_ = 0;
    }
}

} // namespace nemisis::ui
