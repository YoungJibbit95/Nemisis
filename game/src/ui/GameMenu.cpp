#include "nemisis/ui/GameMenu.hpp"

#include "nemisis/input/InputBindings.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cmath>
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

void addLine(
    novacore::render::RenderFrameInfo& frame,
    float x0,
    float y0,
    float x1,
    float y1,
    std::array<float, 4> color) {
    frame.debugLines.push_back(novacore::render::DebugLine{x0, y0, x1, y1, color});
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

[[nodiscard]] std::string fixedTwo(float value) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << value;
    return stream.str();
}

[[nodiscard]] std::string vec3Summary(novacore::math::Vec3 value) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(1)
           << value.x << ", " << value.y << ", " << value.z;
    return stream.str();
}

[[nodiscard]] std::string_view movementModeName(movement::MovementMode mode) {
    switch (mode) {
    case movement::MovementMode::Grounded:
        return "Grounded";
    case movement::MovementMode::Airborne:
        return "Airborne";
    case movement::MovementMode::Sliding:
        return "Sliding";
    case movement::MovementMode::Dashing:
        return "Dashing";
    case movement::MovementMode::WallRunning:
        return "Wall Run";
    case movement::MovementMode::Mantling:
        return "Mantle";
    }
    return "Unknown";
}

[[nodiscard]] std::string_view deviceName(novacore::platform::InputDeviceKind device) {
    switch (device) {
    case novacore::platform::InputDeviceKind::KeyboardMouse:
        return "MKB";
    case novacore::platform::InputDeviceKind::Controller:
        return "Controller";
    }
    return "Unknown";
}

[[nodiscard]] std::string_view yesNo(bool value) {
    return value ? "Yes" : "No";
}

struct MapPoint final {
    float x = 0.0F;
    float y = 0.0F;
};

[[nodiscard]] MapPoint projectTopDown(
    const dev::GreyboxWorld& world,
    novacore::math::Vec3 position,
    float x,
    float y,
    float width,
    float height) {
    const float halfWidth = std::max(1.0F, world.boundsHalfExtents.x);
    const float halfDepth = std::max(1.0F, world.boundsHalfExtents.z);
    return MapPoint{
        x + (width * 0.5F) + ((position.x / halfWidth) * (width * 0.5F)),
        y + (height * 0.5F) - ((position.z / halfDepth) * (height * 0.5F)),
    };
}

void addMapBox(
    novacore::render::RenderFrameInfo& frame,
    const dev::GreyboxWorld& world,
    const dev::GreyboxPrimitive& primitive,
    float x,
    float y,
    float width,
    float height) {
    const auto min = projectTopDown(
        world,
        primitive.center - novacore::math::Vec3{primitive.halfExtents.x, 0.0F, primitive.halfExtents.z},
        x,
        y,
        width,
        height);
    const auto max = projectTopDown(
        world,
        primitive.center + novacore::math::Vec3{primitive.halfExtents.x, 0.0F, primitive.halfExtents.z},
        x,
        y,
        width,
        height);

    const float rectX = std::min(min.x, max.x);
    const float rectY = std::min(min.y, max.y);
    const float rectWidth = std::max(2.0F, std::abs(max.x - min.x));
    const float rectHeight = std::max(2.0F, std::abs(max.y - min.y));
    addRect(frame, rectX, rectY, rectWidth, rectHeight, primitive.color);
}

void addDevRangeMap(
    novacore::render::RenderFrameInfo& frame,
    const dev::GreyboxWorld& world,
    const dev::DevSandboxSample& sample) {
    constexpr float mapX = 58.0F;
    constexpr float mapY = 190.0F;
    constexpr float mapWidth = 520.0F;
    constexpr float mapHeight = 338.0F;
    constexpr float kPi = 3.14159265358979323846F;

    addRect(frame, mapX, mapY, mapWidth, mapHeight, {0.020F, 0.030F, 0.034F, 0.96F});
    addText(frame, mapX + 16.0F, mapY + 16.0F, 2.0F, {0.68F, 0.82F, 0.86F, 1.0F}, "GREYBOX DEV RANGE");

    for (int column = -4; column <= 4; ++column) {
        const float worldX = static_cast<float>(column) * (world.boundsHalfExtents.x / 4.0F);
        const auto a = projectTopDown(world, {worldX, 0.0F, -world.boundsHalfExtents.z}, mapX, mapY, mapWidth, mapHeight);
        const auto b = projectTopDown(world, {worldX, 0.0F, world.boundsHalfExtents.z}, mapX, mapY, mapWidth, mapHeight);
        addLine(frame, a.x, a.y, b.x, b.y, {0.10F, 0.14F, 0.15F, 1.0F});
    }
    for (int row = -4; row <= 4; ++row) {
        const float worldZ = static_cast<float>(row) * (world.boundsHalfExtents.z / 4.0F);
        const auto a = projectTopDown(world, {-world.boundsHalfExtents.x, 0.0F, worldZ}, mapX, mapY, mapWidth, mapHeight);
        const auto b = projectTopDown(world, {world.boundsHalfExtents.x, 0.0F, worldZ}, mapX, mapY, mapWidth, mapHeight);
        addLine(frame, a.x, a.y, b.x, b.y, {0.10F, 0.14F, 0.15F, 1.0F});
    }

    for (const auto& primitive : world.primitives) {
        if (primitive.kind == dev::GreyboxPrimitiveKind::Floor) {
            continue;
        }
        addMapBox(frame, world, primitive, mapX, mapY, mapWidth, mapHeight);
    }

    const auto player = projectTopDown(world, sample.position, mapX, mapY, mapWidth, mapHeight);
    addRect(frame, player.x - 5.0F, player.y - 5.0F, 10.0F, 10.0F, {0.05F, 0.82F, 0.95F, 1.0F});

    const float yaw = sample.view.yawDegrees * (kPi / 180.0F);
    const auto lookEnd = projectTopDown(
        world,
        sample.position + novacore::math::Vec3{std::sin(yaw), 0.0F, std::cos(yaw)} * 5.5F,
        mapX,
        mapY,
        mapWidth,
        mapHeight);
    addLine(frame, player.x, player.y, lookEnd.x, lookEnd.y, {0.72F, 0.95F, 1.0F, 1.0F});

    const auto target = projectTopDown(world, sample.target.position, mapX, mapY, mapWidth, mapHeight);
    const auto targetColor = sample.target.eliminated
        ? std::array<float, 4>{0.18F, 0.18F, 0.18F, 1.0F}
        : std::array<float, 4>{0.95F, 0.16F, 0.10F, 1.0F};
    addRect(frame, target.x - 8.0F, target.y - 8.0F, 16.0F, 16.0F, targetColor);
    addLine(frame, player.x, player.y, target.x, target.y, {0.42F, 0.19F, 0.16F, 0.75F});
}

} // namespace

void GameMenu::update(const novacore::platform::InputActionMap& actions) {
    if (pressed(actions, input::actions::ToggleDebug)) {
        debugOverlayEnabled_ = !debugOverlayEnabled_;
    }
    if (debugOverlayEnabled_ && pressed(actions, input::actions::DebugNextPage)) {
        cycleDebugPage();
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
    const nemisis::dev::GreyboxWorld& greyboxWorld,
    std::string_view rendererBackend,
    std::size_t queuedAssets) const {
    addRect(frame, 0.0F, 0.0F, 1280.0F, 720.0F, {0.0F, 0.0F, 0.0F, 0.05F});
    addRect(frame, 38.0F, 30.0F, 510.0F, 146.0F, {0.025F, 0.045F, 0.055F, 0.92F});

    if (screen_ == GameScreen::MainMenu) {
        addHeader(frame, "MAIN MENU", "ARROWS/ENTER OR 1 2 3  -  F1 DEBUG  TAB PAGE");
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
        addHeader(frame, "DEV SHOOTING RANGE", "ESC BACK  F1 DEBUG  TAB PAGE  MOUSE/RIGHT STICK LOOK");
        addDevRangeMap(frame, greyboxWorld, sample);
        addRect(frame, 633.0F, 340.0F, 14.0F, 2.0F, {0.95F, 0.98F, 1.0F, 1.0F});
        addRect(frame, 640.0F, 333.0F, 2.0F, 14.0F, {0.95F, 0.98F, 1.0F, 1.0F});
        addRect(frame, 838.0F, 212.0F, 250.0F, 318.0F, {0.018F, 0.026F, 0.030F, 0.92F});
        addText(frame, 858.0F, 234.0F, 2.0F, {0.65F, 0.80F, 0.84F, 1.0F}, "TARGET LANE");
        addLine(frame, 864.0F, 498.0F, 1020.0F, 302.0F, {0.16F, 0.24F, 0.25F, 1.0F});
        addLine(frame, 1052.0F, 498.0F, 1020.0F, 302.0F, {0.16F, 0.24F, 0.25F, 1.0F});
        addLine(frame, 864.0F, 498.0F, 1052.0F, 498.0F, {0.16F, 0.24F, 0.25F, 1.0F});
        addRect(frame, 934.0F, 304.0F, 82.0F, 138.0F, {0.40F, 0.08F, 0.06F, 0.9F});
        addRect(frame, 952.0F, 270.0F, 46.0F, 46.0F, {0.82F, 0.18F, 0.14F, 0.95F});
        addRect(frame, 820.0F, 506.0F, 118.0F, 44.0F, {0.10F, 0.13F, 0.13F, 0.95F});
        addRect(frame, 940.0F, 518.0F, 150.0F, 28.0F, {0.19F, 0.22F, 0.22F, 0.95F});
        const float hpRatio = std::clamp(sample.target.health / sample.target.maxHealth, 0.0F, 1.0F);
        addRect(frame, 820.0F, 552.0F, 260.0F, 18.0F, {0.16F, 0.06F, 0.05F, 1.0F});
        addRect(frame, 820.0F, 552.0F, 260.0F * hpRatio, 18.0F, {0.85F, 0.14F, 0.10F, 1.0F});
        addText(frame, 820.0F, 530.0F, 2.0F, {0.92F, 0.94F, 0.95F, 1.0F}, "TARGET HP " + fixedOne(sample.target.health));
    } else {
        const bool tdm = screen_ == GameScreen::TeamDeathmatch;
        addHeader(frame, tdm ? "TEAM DEATHMATCH" : "CONTROL", "PLACEHOLDER MODE  -  ESC BACK");
        addRect(frame, 76.0F, 236.0F, 560.0F, 120.0F, {0.06F, 0.08F, 0.10F, 0.9F});
        addText(frame, 98.0F, 272.0F, 3.0F, {0.86F, 0.92F, 0.95F, 1.0F}, tdm ? "MATCH FLOW NEXT" : "OBJECTIVES NEXT");
        addText(frame, 100.0F, 322.0F, 2.0F, {0.52F, 0.62F, 0.68F, 1.0F}, "SPAWNS TEAMS SCOREBOARD SERVER AUTH");
    }

    if (debugOverlayEnabled_) {
        addRect(frame, 30.0F, 578.0F, 1060.0F, 110.0F, {0.015F, 0.025F, 0.030F, 0.92F});
        addText(
            frame,
            48.0F,
            594.0F,
            2.0F,
            {0.72F, 0.90F, 0.95F, 1.0F},
            "DEBUG " + std::string(debugPageName()) + "  TAB/START");

        switch (debugPage_) {
        case DebugPage::Gameplay:
            addMetric(frame, 48.0F, 624.0F, "SCREEN", std::string(screenName()));
            addMetric(frame, 48.0F, 650.0F, "MODE", std::string(movementModeName(sample.movementMode)));
            addMetric(frame, 386.0F, 624.0F, "TICK", std::to_string(sample.tick));
            addMetric(frame, 386.0F, 650.0F, "INPUT", std::string(deviceName(sample.command.device)));
            addMetric(frame, 674.0F, 624.0F, "POS", vec3Summary(sample.position));
            addMetric(frame, 674.0F, 650.0F, "VEL", vec3Summary(sample.velocity));
            break;
        case DebugPage::Network:
            addMetric(frame, 48.0F, 624.0F, "CMD TX", std::to_string(sample.netBridge.sentCommandPackets));
            addMetric(frame, 48.0F, 650.0F, "CMD RX", std::to_string(sample.netBridge.receivedCommandPackets));
            addMetric(frame, 386.0F, 624.0F, "ACK TX", std::to_string(sample.netBridge.sentAckPackets));
            addMetric(frame, 386.0F, 650.0F, "ACK RX", std::to_string(sample.netBridge.receivedAckPackets));
            addMetric(frame, 674.0F, 624.0F, "PENDING", std::to_string(sample.network.pendingCommandCount));
            addMetric(frame, 674.0F, 650.0F, "ACK TICK", std::to_string(sample.netBridge.lastAcknowledgedTick));
            break;
        case DebugPage::Assets:
            addMetric(frame, 48.0F, 624.0F, "RENDERER", std::string(rendererBackend));
            addMetric(frame, 48.0F, 650.0F, "ASSETS", std::to_string(queuedAssets));
            addMetric(frame, 386.0F, 624.0F, "AMMO", std::to_string(sample.weapon.ammoInMagazine));
            addMetric(frame, 386.0F, 650.0F, "RELOAD", std::string(yesNo(sample.weapon.reloading)));
            addMetric(frame, 674.0F, 624.0F, "SHOT", std::string(yesNo(sample.hasShot)));
            addMetric(frame, 674.0F, 650.0F, "DMG/RANGE", fixedOne(sample.shot.damage) + " / " + fixedTwo(sample.shot.rangeMeters));
            break;
        }
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

DebugPage GameMenu::debugPage() const {
    return debugPage_;
}

std::string GameMenu::title() const {
    std::string title = "Nemisis - " + std::string(screenName());
    if (debugOverlayEnabled_) {
        title += " [Debug: ";
        title += debugPageName();
        title += "]";
    }
    return title;
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

std::string_view GameMenu::debugPageName() const {
    switch (debugPage_) {
    case DebugPage::Gameplay:
        return "Gameplay";
    case DebugPage::Network:
        return "Network";
    case DebugPage::Assets:
        return "Assets";
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

void GameMenu::cycleDebugPage() {
    switch (debugPage_) {
    case DebugPage::Gameplay:
        debugPage_ = DebugPage::Network;
        break;
    case DebugPage::Network:
        debugPage_ = DebugPage::Assets;
        break;
    case DebugPage::Assets:
        debugPage_ = DebugPage::Gameplay;
        break;
    }
}

} // namespace nemisis::ui
