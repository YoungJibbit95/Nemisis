#include "nemisis/ui/GameMenu.hpp"

#include "nemisis/input/InputBindings.hpp"
#include "nemisis/ui/UiCanvas.hpp"
#include "nemisis/weapons/WeaponMetrics.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

namespace nemisis::ui {

namespace {

constexpr std::array<MenuTab, 6> kTabs{
    MenuTab::Play,
    MenuTab::Gamemodes,
    MenuTab::Loadout,
    MenuTab::Character,
    MenuTab::Settings,
    MenuTab::Account,
};

constexpr std::array<std::string_view, 4> kWeaponCycle{
    "ar_01",
    "smg_01",
    "sidearm_01",
    "shotgun_01",
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
    appendUiRect(frame, UiRect{x, y, width, height}, color);
}

void addLine(
    novacore::render::RenderFrameInfo& frame,
    float x0,
    float y0,
    float x1,
    float y1,
    std::array<float, 4> color) {
    appendUiLine(frame, x0, y0, x1, y1, color);
}

void addText(
    novacore::render::RenderFrameInfo& frame,
    float x,
    float y,
    float scale,
    std::array<float, 4> color,
    std::string text) {
    appendUiText(frame, x, y, scale, color, std::move(text));
}

void addImage(
    novacore::render::RenderFrameInfo& frame,
    UiRect rect,
    std::string assetId,
    std::array<float, 4> tint = {0.12F, 0.24F, 0.28F, 0.95F}) {
    UiCanvas canvas;
    canvas.beginFrame();
    canvas.image(rect, std::move(assetId), tint);
    canvas.appendToRenderFrame(frame);
}

void addProgress(
    novacore::render::RenderFrameInfo& frame,
    UiRect rect,
    float value,
    std::array<float, 4> background,
    std::array<float, 4> foreground) {
    UiCanvas canvas;
    canvas.beginFrame();
    canvas.progressBar(rect, value, background, foreground);
    canvas.appendToRenderFrame(frame);
}

void addCrosshair(
    novacore::render::RenderFrameInfo& frame,
    float centerX,
    float centerY,
    float gap,
    float length,
    std::array<float, 4> color) {
    UiCanvas canvas;
    canvas.beginFrame();
    canvas.crosshair(centerX, centerY, gap, length, color);
    canvas.appendToRenderFrame(frame);
}

void addHeader(novacore::render::RenderFrameInfo& frame, std::string_view title, std::string_view subtitle) {
    addText(frame, 54.0F, 42.0F, 5.0F, palette::TextPrimary, "NEMISIS");
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
    addText(frame, x + 168.0F, y, 2.0F, palette::TextPrimary, std::move(value));
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

[[nodiscard]] std::string percent(float value) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(0) << (value * 100.0F) << "%";
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

[[nodiscard]] std::string_view groundKindName(dev::GreyboxPrimitiveKind kind) {
    switch (kind) {
    case dev::GreyboxPrimitiveKind::Floor:
        return "Floor";
    case dev::GreyboxPrimitiveKind::Wall:
        return "Wall";
    case dev::GreyboxPrimitiveKind::Ramp:
        return "Ramp";
    case dev::GreyboxPrimitiveKind::Cover:
        return "Cover";
    case dev::GreyboxPrimitiveKind::Spawn:
        return "Spawn";
    case dev::GreyboxPrimitiveKind::RangeMarker:
        return "Marker";
    case dev::GreyboxPrimitiveKind::Target:
        return "Target";
    }
    return "Unknown";
}

[[nodiscard]] std::string shortId(std::string value, std::size_t maxChars = 17) {
    if (value.size() <= maxChars) {
        return value;
    }
    return value.substr(0, maxChars);
}

[[nodiscard]] std::string_view weaponClassName(weapons::WeaponClass weaponClass) {
    switch (weaponClass) {
    case weapons::WeaponClass::AssaultRifle:
        return "Assault Rifle";
    case weapons::WeaponClass::Smg:
        return "SMG";
    case weapons::WeaponClass::Shotgun:
        return "Shotgun";
    case weapons::WeaponClass::Sidearm:
        return "Sidearm";
    }
    return "Weapon";
}

[[nodiscard]] std::string selectedAttachmentName(
    const weapons::WeaponLoadout& loadout,
    weapons::AttachmentSlot slot,
    const weapons::AttachmentRegistry& attachments) {
    const auto id = weapons::selectedAttachmentId(loadout, slot);
    if (id.empty()) {
        return "None";
    }
    const auto* definition = attachments.find(id);
    if (definition == nullptr) {
        return std::string(id);
    }
    return definition->displayName;
}

[[nodiscard]] std::string selectedAttachmentDesc(
    const weapons::WeaponLoadout& loadout,
    weapons::AttachmentSlot slot,
    const weapons::AttachmentRegistry& attachments) {
    const auto id = weapons::selectedAttachmentId(loadout, slot);
    const auto* definition = attachments.find(id);
    if (definition == nullptr) {
        return "No attachment selected";
    }
    return definition->shortDescription;
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
    addRect(frame, player.x - 5.0F, player.y - 5.0F, 10.0F, 10.0F, palette::Accent);

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
        : palette::Danger;
    addRect(frame, target.x - 8.0F, target.y - 8.0F, 16.0F, 16.0F, targetColor);
    addLine(frame, player.x, player.y, target.x, target.y, {0.42F, 0.19F, 0.16F, 0.75F});
}

void addTopTabs(novacore::render::RenderFrameInfo& frame, MenuTab selectedTab) {
    constexpr float startX = 52.0F;
    constexpr float y = 142.0F;
    constexpr float tabWidth = 164.0F;
    constexpr float tabHeight = 38.0F;
    for (std::size_t index = 0; index < kTabs.size(); ++index) {
        const auto tab = kTabs[index];
        const bool selected = tab == selectedTab;
        const float x = startX + static_cast<float>(index) * (tabWidth + 8.0F);
        addRect(
            frame,
            x,
            y,
            tabWidth,
            tabHeight,
            selected ? palette::AccentSoft : std::array<float, 4>{0.035F, 0.052F, 0.060F, 0.92F});
        addText(
            frame,
            x + 13.0F,
            y + 11.0F,
            2.0F,
            selected ? palette::TextPrimary : palette::TextSecondary,
            std::string(selected ? "> " : "") + std::string([tab]() -> std::string_view {
                switch (tab) {
                case MenuTab::Play:
                    return "PLAY";
                case MenuTab::Gamemodes:
                    return "MODES";
                case MenuTab::Loadout:
                    return "LOADOUT";
                case MenuTab::Character:
                    return "OPERATOR";
                case MenuTab::Settings:
                    return "SETTINGS";
                case MenuTab::Account:
                    return "ACCOUNT";
                }
                return "TAB";
            }()));
    }
}

void addSelectableRow(
    novacore::render::RenderFrameInfo& frame,
    float x,
    float y,
    float width,
    std::string title,
    std::string value,
    bool selected,
    std::array<float, 4> accent = palette::Accent) {
    addRect(
        frame,
        x,
        y,
        width,
        44.0F,
        selected ? std::array<float, 4>{accent[0] * 0.42F, accent[1] * 0.42F, accent[2] * 0.42F, 0.95F}
                 : std::array<float, 4>{0.035F, 0.050F, 0.057F, 0.88F});
    addText(frame, x + 16.0F, y + 13.0F, 2.0F, selected ? palette::TextPrimary : palette::TextSecondary, std::move(title));
    addText(frame, x + width - 270.0F, y + 13.0F, 2.0F, palette::TextPrimary, std::move(value));
}

void addStatBar(
    novacore::render::RenderFrameInfo& frame,
    float x,
    float y,
    std::string label,
    float value,
    std::array<float, 4> color = palette::Accent) {
    addText(frame, x, y, 1.0F, palette::TextSecondary, std::move(label));
    addProgress(frame, {x + 118.0F, y - 2.0F, 180.0F, 10.0F}, value, {0.06F, 0.08F, 0.09F, 1.0F}, color);
}

void renderPlayTab(
    novacore::render::RenderFrameInfo& frame,
    std::size_t selectedIndex,
    const weapons::AttachmentBuildSummary& buildSummary) {
    addImage(frame, {66.0F, 216.0F, 310.0F, 180.0F}, "assets/ui/icons/firing_range.svg");
    addSelectableRow(frame, 416.0F, 216.0F, 520.0F, "Firing Range", "Load live Vulkan range", selectedIndex == 0U);
    addSelectableRow(frame, 416.0F, 272.0F, 520.0F, "Gamemode Browser", "TDM / Control prototypes", selectedIndex == 1U);
    addSelectableRow(frame, 416.0F, 328.0F, 520.0F, "Loadout Editor", "Attachments and weapon tuning", selectedIndex == 2U);

    addRect(frame, 66.0F, 430.0F, 870.0F, 126.0F, palette::Panel);
    addText(frame, 88.0F, 452.0F, 2.0F, palette::Accent, "CURRENT READY CHECK");
    addMetric(frame, 88.0F, 486.0F, "EFFECTIVE MAG", std::to_string(buildSummary.effectiveMagazineSize));
    addMetric(frame, 402.0F, 486.0F, "ADS", fixedTwo(buildSummary.effectiveWeapon.adsTimeSeconds) + "s");
    addMetric(frame, 650.0F, 486.0F, "RELOAD", fixedTwo(buildSummary.effectiveWeapon.reloadTimeSeconds) + "s");
    addText(frame, 88.0F, 526.0F, 2.0F, palette::TextSecondary, "Loading screens, UI tabs, player camera, weapon runtime and attachment build are live systems now.");
}

void renderGamemodesTab(novacore::render::RenderFrameInfo& frame, std::size_t selectedIndex) {
    addSelectableRow(frame, 66.0F, 222.0F, 500.0F, "Team Deathmatch", "6v6 sandbox shell", selectedIndex == 0U, {0.95F, 0.45F, 0.25F, 1.0F});
    addSelectableRow(frame, 66.0F, 282.0F, 500.0F, "Control", "3-zone objective shell", selectedIndex == 1U, {0.35F, 0.72F, 1.0F, 1.0F});

    addRect(frame, 610.0F, 222.0F, 430.0F, 210.0F, palette::Panel);
    addText(frame, 632.0F, 246.0F, 3.0F, palette::TextPrimary, selectedIndex == 0U ? "TEAM DEATHMATCH" : "CONTROL");
    addText(frame, 634.0F, 300.0F, 2.0F, palette::TextSecondary, "Match lifecycle, score events and spawn rules are next.");
    addText(frame, 634.0F, 334.0F, 2.0F, palette::TextSecondary, "This tab already routes through the same loading flow as the range.");
}

void renderLoadoutTab(
    novacore::render::RenderFrameInfo& frame,
    std::size_t selectedIndex,
    const weapons::WeaponLoadout& loadout,
    const weapons::AttachmentRegistry& attachments,
    const weapons::AttachmentBuildSummary& buildSummary) {
    addImage(frame, {66.0F, 216.0F, 268.0F, 156.0F}, "assets/ui/icons/loadout.svg");
    addSelectableRow(frame, 370.0F, 216.0F, 570.0F, "Weapon", loadout.weaponId, selectedIndex == 0U, palette::Warning);

    for (std::size_t slotIndex = 0; slotIndex < weapons::kAttachmentSlotCount; ++slotIndex) {
        const auto slot = static_cast<weapons::AttachmentSlot>(slotIndex);
        const float y = 278.0F + static_cast<float>(slotIndex) * 48.0F;
        addSelectableRow(
            frame,
            370.0F,
            y,
            570.0F,
            std::string(weapons::attachmentSlotName(slot)),
            selectedAttachmentName(loadout, slot, attachments),
            selectedIndex == slotIndex + 1U,
            palette::Accent);
    }

    addRect(frame, 66.0F, 402.0F, 268.0F, 148.0F, palette::Panel);
    const auto selectedSlot = selectedIndex == 0U
        ? weapons::AttachmentSlot::Optic
        : static_cast<weapons::AttachmentSlot>(std::min<std::size_t>(selectedIndex - 1U, weapons::kAttachmentSlotCount - 1U));
    addText(frame, 86.0F, 426.0F, 2.0F, palette::Accent, std::string(weapons::attachmentSlotName(selectedSlot)));
    addText(frame, 86.0F, 462.0F, 2.0F, palette::TextPrimary, selectedAttachmentName(loadout, selectedSlot, attachments));
    addText(frame, 86.0F, 498.0F, 1.0F, palette::TextSecondary, shortId(selectedAttachmentDesc(loadout, selectedSlot, attachments), 34));

    addRect(frame, 970.0F, 216.0F, 230.0F, 294.0F, palette::Panel);
    addText(frame, 990.0F, 240.0F, 2.0F, palette::TextPrimary, "EFFECTIVE BUILD");
    addStatBar(frame, 990.0F, 286.0F, "Range", std::clamp(buildSummary.effectiveWeapon.maxRangeMeters / 100.0F, 0.0F, 1.0F));
    addStatBar(frame, 990.0F, 326.0F, "Control", std::clamp(1.0F - buildSummary.effectiveWeapon.recoilPitchPerShotDegrees * 4.0F, 0.0F, 1.0F), palette::Success);
    addStatBar(frame, 990.0F, 366.0F, "ADS", std::clamp(1.0F - buildSummary.effectiveWeapon.adsTimeSeconds, 0.0F, 1.0F), palette::Warning);
    addStatBar(frame, 990.0F, 406.0F, "Mobility", std::clamp(0.50F + buildSummary.mobilityModifier, 0.0F, 1.0F), palette::Accent);
    addMetric(frame, 990.0F, 456.0F, "MAG", std::to_string(buildSummary.effectiveMagazineSize));
}

void renderCharacterTab(novacore::render::RenderFrameInfo& frame, std::size_t selectedIndex) {
    addImage(frame, {72.0F, 216.0F, 260.0F, 260.0F}, "assets/ui/icons/operator.svg", {0.10F, 0.20F, 0.24F, 0.96F});
    addSelectableRow(frame, 370.0F, 216.0F, 600.0F, "Operator", "Vanguard Pilot", selectedIndex == 0U, palette::Accent);
    addSelectableRow(frame, 370.0F, 274.0F, 600.0F, "Helmet", "Pilot visor / US tactical shell", selectedIndex == 1U, palette::Accent);
    addSelectableRow(frame, 370.0F, 332.0F, 600.0F, "Armor", "Sci-fi plate carrier", selectedIndex == 2U, palette::Accent);
    addSelectableRow(frame, 370.0F, 390.0F, 600.0F, "Rig", "First-person arms socket pass", selectedIndex == 3U, palette::Accent);
    addRect(frame, 370.0F, 466.0F, 600.0F, 76.0F, palette::Panel);
    addText(frame, 390.0F, 490.0F, 2.0F, palette::TextSecondary, "Asset worker is building the first A1 soldier/pilot hybrid blockout in parallel.");
}

void renderSettingsTab(
    novacore::render::RenderFrameInfo& frame,
    std::size_t selectedIndex,
    const settings::GameSettings& settings) {
    addImage(frame, {72.0F, 216.0F, 220.0F, 152.0F}, "assets/ui/icons/settings.svg");
    addSelectableRow(frame, 340.0F, 216.0F, 620.0F, "Mouse Sensitivity", fixedTwo(settings.mouse.sensitivityX), selectedIndex == 0U, palette::Accent);
    addSelectableRow(frame, 340.0F, 272.0F, 620.0F, "Controller Sensitivity", fixedTwo(settings.controller.lookSensitivityX), selectedIndex == 1U, palette::Accent);
    addSelectableRow(frame, 340.0F, 328.0F, 620.0F, "HUD Scale", fixedTwo(settings.video.hudScale), selectedIndex == 2U, palette::Accent);
    addSelectableRow(frame, 340.0F, 384.0F, 620.0F, "Aim Assist", std::string(yesNo(settings.controller.aimAssistEnabled)), selectedIndex == 3U, palette::Warning);
    addSelectableRow(frame, 340.0F, 440.0F, 620.0F, "Damage Numbers", std::string(yesNo(settings.gameplay.showDamageNumbers)), selectedIndex == 4U, palette::Warning);
    addRect(frame, 72.0F, 400.0F, 220.0F, 96.0F, palette::Panel);
    addText(frame, 90.0F, 424.0F, 2.0F, palette::TextPrimary, "LEFT/RIGHT");
    addText(frame, 90.0F, 456.0F, 1.0F, palette::TextSecondary, "Adjusts values live.");
}

void renderAccountTab(
    novacore::render::RenderFrameInfo& frame,
    std::size_t selectedIndex,
    const player::AccountStats& accountStats) {
    (void)selectedIndex;
    addRect(frame, 72.0F, 218.0F, 460.0F, 238.0F, palette::Panel);
    addText(frame, 94.0F, 244.0F, 3.0F, palette::TextPrimary, accountStats.accountName);
    addMetric(frame, 96.0F, 304.0F, "LEVEL", std::to_string(accountStats.level));
    addMetric(frame, 96.0F, 338.0F, "K/D", fixedTwo(player::killDeathRatio(accountStats)));
    addMetric(frame, 96.0F, 372.0F, "WIN RATE", percent(player::winRate(accountStats)));
    addMetric(frame, 96.0F, 406.0F, "DAMAGE/MATCH", fixedOne(accountStats.damagePerMatch));

    addRect(frame, 574.0F, 218.0F, 470.0F, 238.0F, palette::Panel);
    addText(frame, 596.0F, 244.0F, 2.0F, palette::Accent, "BEST WEAPON");
    addMetric(frame, 596.0F, 290.0F, "NAME", accountStats.bestWeapon.displayName);
    addMetric(frame, 596.0F, 324.0F, "ELIMS", std::to_string(accountStats.bestWeapon.eliminations));
    addMetric(frame, 596.0F, 358.0F, "ACCURACY", percent(player::weaponAccuracy(accountStats.bestWeapon)));
    addText(frame, 596.0F, 410.0F, 2.0F, palette::Warning, "BEST OPERATOR  " + accountStats.bestOperator.displayName);
}

void renderMainMenuShell(
    novacore::render::RenderFrameInfo& frame,
    MenuTab tab,
    std::size_t selectedIndex,
    const settings::GameSettings& settings,
    const weapons::WeaponLoadout& loadout,
    const weapons::AttachmentRegistry& attachments,
    const weapons::AttachmentBuildSummary& attachmentSummary,
    const player::AccountStats& accountStats) {
    addHeader(frame, "MAIN MENU", "Q/E OR LB/RB TABS  -  LEFT/RIGHT ADJUST  -  ENTER/A CONFIRM  -  1 RANGE");
    addTopTabs(frame, tab);

    switch (tab) {
    case MenuTab::Play:
        renderPlayTab(frame, selectedIndex, attachmentSummary);
        break;
    case MenuTab::Gamemodes:
        renderGamemodesTab(frame, selectedIndex);
        break;
    case MenuTab::Loadout:
        renderLoadoutTab(frame, selectedIndex, loadout, attachments, attachmentSummary);
        break;
    case MenuTab::Character:
        renderCharacterTab(frame, selectedIndex);
        break;
    case MenuTab::Settings:
        renderSettingsTab(frame, selectedIndex, settings);
        break;
    case MenuTab::Account:
        renderAccountTab(frame, selectedIndex, accountStats);
        break;
    }
}

void renderLoadingScreen(
    novacore::render::RenderFrameInfo& frame,
    GameScreen target,
    float progress,
    const weapons::AttachmentBuildSummary& attachmentSummary) {
    addRect(frame, 0.0F, 0.0F, 1280.0F, 720.0F, {0.005F, 0.012F, 0.014F, 1.0F});
    addImage(frame, {120.0F, 104.0F, 760.0F, 356.0F}, "assets/ui/loading/dev_range_loading.svg", {0.08F, 0.20F, 0.24F, 0.98F});
    addText(frame, 930.0F, 128.0F, 4.0F, palette::TextPrimary, target == GameScreen::DevRange ? "FIRING RANGE" : "MATCH SHELL");
    addText(frame, 932.0F, 184.0F, 2.0F, palette::TextSecondary, "Streaming greybox, weapons, input, UI and debug layers.");
    addMetric(frame, 932.0F, 252.0F, "WEAPON", attachmentSummary.effectiveWeapon.displayName);
    addMetric(frame, 932.0F, 286.0F, "CLASS", std::string(weaponClassName(attachmentSummary.effectiveWeapon.weaponClass)));
    addMetric(frame, 932.0F, 320.0F, "MAG", std::to_string(attachmentSummary.effectiveMagazineSize));
    addProgress(frame, {120.0F, 510.0F, 990.0F, 20.0F}, progress, {0.05F, 0.07F, 0.075F, 1.0F}, palette::Accent);
    addText(frame, 120.0F, 552.0F, 2.0F, palette::TextSecondary, "Design target: readable BO-like gunfights, Apex-lite motion, server-authoritative spine.");
}

void renderDevRangeHud(
    novacore::render::RenderFrameInfo& frame,
    const dev::DevSandboxSample& sample,
    const weapons::AttachmentBuildSummary& attachmentSummary,
    const settings::GameSettings& settings) {
    addHeader(frame, "DEV SHOOTING RANGE", "ESC BACK  F1 DEBUG  TAB PAGE  MOUSE/RIGHT STICK LOOK");
    addCrosshair(
        frame,
        640.0F,
        340.0F,
        7.0F + (sample.fire.movementSpreadDegrees * 2.0F),
        12.0F,
        sample.targetHit.hit ? palette::Danger : palette::TextPrimary);

    addRect(frame, 814.0F, 34.0F, 390.0F, 116.0F, palette::Panel);
    addText(frame, 836.0F, 58.0F, 2.0F, palette::Accent, attachmentSummary.effectiveWeapon.displayName);
    addMetric(frame, 836.0F, 92.0F, "AMMO", std::to_string(sample.weapon.ammoInMagazine) + " / " + std::to_string(attachmentSummary.effectiveMagazineSize));
    addMetric(frame, 836.0F, 122.0F, "ADS", fixedOne(sample.weapon.adsAlpha) + "  recoil " + fixedOne(sample.weapon.recoilPitchOffsetDegrees));
    addMetric(frame, 1030.0F, 92.0F, "HUD", fixedTwo(settings.video.hudScale));
    addMetric(frame, 1030.0F, 122.0F, "DMG", std::string(yesNo(settings.gameplay.showDamageNumbers)));

    const float healthRatio = sample.playerHealth.maxHealth > 0.0F
        ? std::clamp(sample.playerHealth.health / sample.playerHealth.maxHealth, 0.0F, 1.0F)
        : 0.0F;
    addRect(frame, 58.0F, 34.0F, 330.0F, 116.0F, palette::Panel);
    addText(frame, 80.0F, 58.0F, 2.0F, palette::TextPrimary, "PLAYER");
    addProgress(frame, {80.0F, 94.0F, 210.0F, 14.0F}, healthRatio, {0.11F, 0.05F, 0.05F, 1.0F}, healthRatio > 0.35F ? palette::Success : palette::Danger);
    addMetric(frame, 80.0F, 122.0F, "HP", fixedOne(sample.playerHealth.health) + " / " + fixedOne(sample.playerHealth.maxHealth));

    addRect(frame, 414.0F, 34.0F, 360.0F, 116.0F, palette::Panel);
    addText(frame, 436.0F, 58.0F, 2.0F, palette::Accent, "RANGE SESSION");
    addMetric(frame, 436.0F, 92.0F, "ELIMS", std::to_string(sample.rangeSession.score.targetsEliminated));
    addMetric(frame, 436.0F, 122.0F, "ACC", percent(dev::devRangeAccuracy(sample.rangeSession.score)));
    addMetric(frame, 610.0F, 92.0F, "STREAK", std::to_string(sample.rangeSession.score.currentStreak));
    addMetric(frame, 610.0F, 122.0F, "BEST", std::to_string(sample.rangeSession.score.bestStreak));

    if (sample.fire.fired) {
        addText(
            frame,
            sample.targetHit.hit ? 598.0F : 586.0F,
            292.0F,
            3.0F,
            sample.targetHit.hit ? palette::Danger : palette::Warning,
            sample.targetHit.hit ? "HIT" : "MISS");
    }
    if (sample.targetHit.hit && settings.gameplay.showDamageNumbers) {
        addText(frame, 674.0F, 286.0F, 2.0F, palette::Warning, "-" + fixedOne(sample.targetHit.damageApplied));
    }
    if (!sample.rangeSession.eventText.empty()) {
        addText(frame, 544.0F, 250.0F, 3.0F, palette::Accent, sample.rangeSession.eventText);
    }
    if (sample.rangeSession.targetRespawnSeconds > 0.0F) {
        addText(frame, 530.0F, 286.0F, 2.0F, palette::Warning, "TARGET RESPAWN " + fixedOne(sample.rangeSession.targetRespawnSeconds));
    }
    addText(frame, 58.0F, 164.0F, 1.0F, palette::TextSecondary, "P / Y resets range  -  Q/E or LB/RB tabs in menu");
}

void renderDevRangePanels(
    novacore::render::RenderFrameInfo& frame,
    const dev::DevSandboxSample& sample,
    const dev::GreyboxWorld& greyboxWorld) {
    addDevRangeMap(frame, greyboxWorld, sample);
    addRect(frame, 838.0F, 212.0F, 250.0F, 318.0F, {0.018F, 0.026F, 0.030F, 0.92F});
    addText(frame, 858.0F, 234.0F, 2.0F, {0.65F, 0.80F, 0.84F, 1.0F}, "TARGET LANE");
    addLine(frame, 864.0F, 498.0F, 1020.0F, 302.0F, {0.16F, 0.24F, 0.25F, 1.0F});
    addLine(frame, 1052.0F, 498.0F, 1020.0F, 302.0F, {0.16F, 0.24F, 0.25F, 1.0F});
    addLine(frame, 864.0F, 498.0F, 1052.0F, 498.0F, {0.16F, 0.24F, 0.25F, 1.0F});
    addRect(frame, 934.0F, 304.0F, 82.0F, 138.0F, {0.40F, 0.08F, 0.06F, 0.9F});
    addRect(frame, 952.0F, 270.0F, 46.0F, 46.0F, {0.82F, 0.18F, 0.14F, 0.95F});
    const float hpRatio = std::clamp(sample.target.health / sample.target.maxHealth, 0.0F, 1.0F);
    addProgress(frame, {820.0F, 552.0F, 260.0F, 18.0F}, hpRatio, {0.16F, 0.06F, 0.05F, 1.0F}, palette::Danger);
    addText(frame, 820.0F, 530.0F, 2.0F, palette::TextPrimary, "TARGET HP " + fixedOne(sample.target.health));
}

void renderPlaceholderMode(novacore::render::RenderFrameInfo& frame, GameScreen screen) {
    const bool tdm = screen == GameScreen::TeamDeathmatch;
    addHeader(frame, tdm ? "TEAM DEATHMATCH" : "CONTROL", "PLACEHOLDER MODE  -  ESC BACK");
    addRect(frame, 76.0F, 236.0F, 640.0F, 148.0F, palette::Panel);
    addText(frame, 98.0F, 272.0F, 3.0F, palette::TextPrimary, tdm ? "MATCH FLOW NEXT" : "OBJECTIVES NEXT");
    addText(frame, 100.0F, 322.0F, 2.0F, palette::TextSecondary, "SPAWNS TEAMS SCOREBOARD SERVER AUTH");
    addText(frame, 100.0F, 352.0F, 2.0F, palette::Accent, "This screen already uses loading, tab shell, and gamepad menu navigation.");
}

void renderDebugOverlay(
    novacore::render::RenderFrameInfo& frame,
    DebugPage debugPage,
    std::string_view debugPageName,
    std::string_view screenName,
    const dev::DevSandboxSample& sample,
    std::string_view rendererBackend,
    std::string_view vulkanSummary,
    std::size_t queuedAssets,
    const assets::DevAssetBindingSummary& assetSummary,
    const novacore::render::MeshResourceStats& meshStats,
    const dev::DevRangeRenderSceneStats& sceneStats) {
    addRect(frame, 30.0F, 558.0F, 1210.0F, 146.0F, {0.015F, 0.025F, 0.030F, 0.92F});
    addText(frame, 48.0F, 574.0F, 2.0F, {0.72F, 0.90F, 0.95F, 1.0F}, "DEBUG " + std::string(debugPageName) + "  TAB/START");

    switch (debugPage) {
    case DebugPage::Gameplay:
        addMetric(frame, 48.0F, 604.0F, "SCREEN", std::string(screenName));
        addMetric(frame, 48.0F, 630.0F, "MODE", std::string(movementModeName(sample.movementMode)));
        addMetric(frame, 48.0F, 656.0F, "INPUT", std::string(deviceName(sample.command.device)));
        addMetric(frame, 386.0F, 604.0F, "TICK", std::to_string(sample.tick));
        addMetric(frame, 386.0F, 630.0F, "WEAPON", shortId(sample.weapon.weaponId, 20));
        addMetric(frame, 386.0F, 656.0F, "AMMO", std::to_string(sample.weapon.ammoInMagazine) + " / shot " + std::to_string(sample.weapon.shotIndex));
        addMetric(frame, 674.0F, 604.0F, "POS", vec3Summary(sample.position));
        addMetric(frame, 674.0F, 630.0F, "ADS", fixedOne(sample.weapon.adsAlpha) + " recoil " + fixedOne(sample.weapon.recoilPitchOffsetDegrees));
        addMetric(frame, 674.0F, 656.0F, "SPREAD", fixedOne(sample.fire.movementSpreadDegrees));
        addMetric(frame, 928.0F, 604.0F, "HP", fixedOne(sample.playerHealth.health) + "/" + fixedOne(sample.playerHealth.maxHealth));
        addMetric(frame, 928.0F, 630.0F, "RANGE", std::to_string(sample.rangeSession.score.targetsEliminated) + " elim " + percent(dev::devRangeAccuracy(sample.rangeSession.score)));
        addMetric(frame, 928.0F, 656.0F, "KCC", std::to_string(sample.collision.hitCount) + " " + std::string(sample.collision.onRamp ? "Ramp" : sample.collision.stepped ? "Step" : yesNo(sample.collision.blocked)));
        break;
    case DebugPage::Network:
        addMetric(frame, 48.0F, 604.0F, "CMD TX", std::to_string(sample.netBridge.sentCommandPackets));
        addMetric(frame, 48.0F, 630.0F, "CMD RX", std::to_string(sample.netBridge.receivedCommandPackets));
        addMetric(frame, 386.0F, 604.0F, "ACK TX", std::to_string(sample.netBridge.sentAckPackets));
        addMetric(frame, 386.0F, 630.0F, "ACK RX", std::to_string(sample.netBridge.receivedAckPackets));
        addMetric(frame, 674.0F, 604.0F, "PENDING", std::to_string(sample.network.pendingCommandCount));
        addMetric(frame, 674.0F, 630.0F, "ACK TICK", std::to_string(sample.netBridge.lastAcknowledgedTick));
        addMetric(frame, 928.0F, 604.0F, "SCENE", std::to_string(sceneStats.worldBoxCount) + "B " + std::to_string(sceneStats.meshInstanceCount) + "M " + std::to_string(sceneStats.worldLineCount) + "L");
        addMetric(frame, 928.0F, 630.0F, "SKIPPED", std::to_string(sceneStats.skippedMeshInstanceCount));
        break;
    case DebugPage::Assets:
        addMetric(frame, 48.0F, 604.0F, "RENDERER", std::string(rendererBackend));
        addMetric(frame, 48.0F, 630.0F, "VULKAN", std::string(vulkanSummary).substr(0, 34));
        addMetric(frame, 386.0F, 604.0F, "MESH CPU", std::to_string(meshStats.registeredResources) + "/" + std::to_string(assetSummary.requiredAssetCount));
        addMetric(frame, 386.0F, 630.0F, "GPU", std::to_string(meshStats.residentResources) + " RES / " + std::to_string(meshStats.pendingUploadResources) + " PEND");
        addMetric(frame, 674.0F, 604.0F, "P / V", std::to_string(meshStats.totalPrimitives) + " / " + std::to_string(meshStats.totalVertices));
        addMetric(frame, 674.0F, 630.0F, "I / Q", std::to_string(meshStats.totalIndices) + " / " + std::to_string(meshStats.uploadQueueLength + queuedAssets));
        addMetric(frame, 928.0F, 604.0F, "FAILED", std::to_string(meshStats.failedResources));
        addMetric(frame, 928.0F, 630.0F, "DEFER", std::to_string(meshStats.deferredDestroyCount));
        break;
    }
}

} // namespace

void GameMenu::update(const novacore::platform::InputActionMap& actions) {
    settings::GameSettings settings;
    weapons::AttachmentRegistry attachments;
    attachments.registerPrototypeAttachments();
    auto loadout = weapons::defaultPrototypeLoadout();
    update(actions, settings, loadout, attachments);
}

void GameMenu::update(
    const novacore::platform::InputActionMap& actions,
    settings::GameSettings& settings,
    weapons::WeaponLoadout& loadout,
    const weapons::AttachmentRegistry& attachments) {
    if (pressed(actions, input::actions::ToggleDebug)) {
        debugOverlayEnabled_ = !debugOverlayEnabled_;
    }
    if (debugOverlayEnabled_ && pressed(actions, input::actions::DebugNextPage)) {
        cycleDebugPage();
    }

    if (screen_ == GameScreen::Loading) {
        if (pressed(actions, input::actions::MenuBack) || pressed(actions, input::actions::SelectMainMenu)) {
            setScreen(GameScreen::MainMenu);
        }
        return;
    }

    if (pressed(actions, input::actions::SelectDevRange)) {
        startLoading(GameScreen::DevRange);
        return;
    }
    if (pressed(actions, input::actions::SelectTdm)) {
        startLoading(GameScreen::TeamDeathmatch);
        return;
    }
    if (pressed(actions, input::actions::SelectControl)) {
        startLoading(GameScreen::Control);
        return;
    }

    if (screen_ != GameScreen::MainMenu) {
        if (pressed(actions, input::actions::MenuBack) || pressed(actions, input::actions::SelectMainMenu)) {
            setScreen(GameScreen::MainMenu);
        }
        return;
    }

    if (pressed(actions, input::actions::MenuDown)) {
        adjustSelection(1);
    }
    if (pressed(actions, input::actions::MenuUp)) {
        adjustSelection(-1);
    }

    const bool previousTab = pressed(actions, input::actions::MenuPrevTab);
    const bool nextTab = pressed(actions, input::actions::MenuNextTab);
    if (previousTab || nextTab) {
        const int direction = nextTab ? 1 : -1;
        const auto current = static_cast<int>(std::distance(kTabs.begin(), std::find(kTabs.begin(), kTabs.end(), tab_)));
        const int next = (current + direction + static_cast<int>(kTabs.size())) % static_cast<int>(kTabs.size());
        setTab(kTabs[static_cast<std::size_t>(next)]);
        return;
    }

    const bool left = pressed(actions, input::actions::MenuLeft);
    const bool right = pressed(actions, input::actions::MenuRight);
    if (left || right) {
        const int direction = right ? 1 : -1;
        if (tab_ == MenuTab::Settings || tab_ == MenuTab::Loadout) {
            adjustCurrentValue(direction, settings, loadout, attachments);
        } else {
            const auto current = static_cast<int>(std::distance(kTabs.begin(), std::find(kTabs.begin(), kTabs.end(), tab_)));
            const int next = (current + direction + static_cast<int>(kTabs.size())) % static_cast<int>(kTabs.size());
            setTab(kTabs[static_cast<std::size_t>(next)]);
        }
    }

    if (pressed(actions, input::actions::MenuConfirm)) {
        activateSelection(settings, loadout, attachments);
    }
}

void GameMenu::updateFrame(double deltaSeconds) {
    if (screen_ != GameScreen::Loading) {
        return;
    }

    loadingProgress_ = std::min(1.0F, loadingProgress_ + static_cast<float>(std::max(0.0, deltaSeconds) * 1.75));
    if (loadingProgress_ >= 1.0F) {
        setScreen(loadingTarget_);
    }
}

void GameMenu::showDevRange() {
    setScreen(GameScreen::DevRange);
}

void GameMenu::appendRenderCommands(
    novacore::render::RenderFrameInfo& frame,
    const nemisis::dev::DevSandboxSample& sample,
    const nemisis::dev::GreyboxWorld& greyboxWorld,
    std::string_view rendererBackend,
    std::string_view vulkanSummary,
    std::size_t queuedAssets,
    const nemisis::assets::DevAssetBindingSummary& assetSummary,
    const novacore::render::MeshResourceStats& meshStats,
    const nemisis::dev::DevRangeRenderSceneStats& sceneStats,
    const settings::GameSettings& settings,
    const weapons::WeaponLoadout& loadout,
    const weapons::AttachmentRegistry& attachments,
    const weapons::AttachmentBuildSummary& attachmentSummary,
    const player::AccountStats& accountStats) const {
    addRect(frame, 0.0F, 0.0F, 1280.0F, 720.0F, {0.0F, 0.0F, 0.0F, 0.05F});

    if (screen_ == GameScreen::Loading) {
        renderLoadingScreen(frame, loadingTarget_, loadingProgress_, attachmentSummary);
    } else if (screen_ == GameScreen::MainMenu) {
        renderMainMenuShell(frame, tab_, selectedIndex_, settings, loadout, attachments, attachmentSummary, accountStats);
    } else if (screen_ == GameScreen::DevRange) {
        renderDevRangeHud(frame, sample, attachmentSummary, settings);
        renderDevRangePanels(frame, sample, greyboxWorld);
    } else {
        renderPlaceholderMode(frame, screen_);
    }

    if (debugOverlayEnabled_) {
        renderDebugOverlay(
            frame,
            debugPage_,
            debugPageName(),
            screenName(),
            sample,
            rendererBackend,
            vulkanSummary,
            queuedAssets,
            assetSummary,
            meshStats,
            sceneStats);
    }
}

GameScreen GameMenu::screen() const {
    return screen_;
}

MenuTab GameMenu::tab() const {
    return tab_;
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

float GameMenu::loadingProgress() const {
    return loadingProgress_;
}

std::size_t GameMenu::selectedIndex() const {
    return selectedIndex_;
}

weapons::AttachmentSlot GameMenu::selectedAttachmentSlot() const {
    if (selectedIndex_ == 0U) {
        return weapons::AttachmentSlot::Optic;
    }
    return static_cast<weapons::AttachmentSlot>(
        std::min<std::size_t>(selectedIndex_ - 1U, weapons::kAttachmentSlotCount - 1U));
}

std::string GameMenu::title() const {
    std::string title = "Nemisis - " + std::string(screenName());
    if (screen_ == GameScreen::MainMenu) {
        title += " / ";
        title += tabName();
    }
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
    case GameScreen::Loading:
        return "Loading";
    case GameScreen::DevRange:
        return "Dev Range";
    case GameScreen::TeamDeathmatch:
        return "Team Deathmatch";
    case GameScreen::Control:
        return "Control";
    }
    return "Unknown";
}

std::string_view GameMenu::tabName() const {
    switch (tab_) {
    case MenuTab::Play:
        return "Play";
    case MenuTab::Gamemodes:
        return "Gamemodes";
    case MenuTab::Loadout:
        return "Loadout";
    case MenuTab::Character:
        return "Character";
    case MenuTab::Settings:
        return "Settings";
    case MenuTab::Account:
        return "Account";
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
        return {0.016F, 0.024F, 0.028F, 1.0F};
    case GameScreen::Loading:
        return {0.005F, 0.012F, 0.014F, 1.0F};
    case GameScreen::DevRange:
        return {0.028F, 0.040F, 0.052F, 1.0F};
    case GameScreen::TeamDeathmatch:
        return {0.035F, 0.025F, 0.022F, 1.0F};
    case GameScreen::Control:
        return {0.020F, 0.030F, 0.045F, 1.0F};
    }
    return {0.018F, 0.030F, 0.035F, 1.0F};
}

void GameMenu::activateSelection(
    settings::GameSettings& settings,
    weapons::WeaponLoadout& loadout,
    const weapons::AttachmentRegistry& attachments) {
    switch (tab_) {
    case MenuTab::Play:
        if (selectedIndex_ == 0U) {
            startLoading(GameScreen::DevRange);
        } else if (selectedIndex_ == 1U) {
            setTab(MenuTab::Gamemodes);
        } else {
            setTab(MenuTab::Loadout);
        }
        break;
    case MenuTab::Gamemodes:
        startLoading(selectedIndex_ == 0U ? GameScreen::TeamDeathmatch : GameScreen::Control);
        break;
    case MenuTab::Loadout:
    case MenuTab::Settings:
        adjustCurrentValue(1, settings, loadout, attachments);
        break;
    case MenuTab::Character:
        break;
    case MenuTab::Account:
        break;
    }
}

void GameMenu::setScreen(GameScreen screen) {
    screen_ = screen;
    if (screen_ == GameScreen::MainMenu) {
        loadingProgress_ = 0.0F;
    }
}

void GameMenu::startLoading(GameScreen target) {
    loadingTarget_ = target;
    loadingProgress_ = 0.0F;
    screen_ = GameScreen::Loading;
}

void GameMenu::setTab(MenuTab tab) {
    tab_ = tab;
    selectedIndex_ = 0U;
}

void GameMenu::adjustSelection(int delta) {
    const auto count = itemCountForTab();
    if (count == 0U) {
        selectedIndex_ = 0U;
        return;
    }

    const auto current = static_cast<int>(selectedIndex_);
    const auto next = (current + delta + static_cast<int>(count)) % static_cast<int>(count);
    selectedIndex_ = static_cast<std::size_t>(next);
}

void GameMenu::adjustCurrentValue(
    int direction,
    settings::GameSettings& settings,
    weapons::WeaponLoadout& loadout,
    const weapons::AttachmentRegistry& attachments) {
    if (tab_ == MenuTab::Settings) {
        switch (selectedIndex_) {
        case 0:
            settings::adjustMouseSensitivity(settings, 0.05F * static_cast<float>(direction));
            break;
        case 1:
            settings::adjustControllerSensitivity(settings, 0.05F * static_cast<float>(direction));
            break;
        case 2:
            settings::adjustHudScale(settings, 0.05F * static_cast<float>(direction));
            break;
        case 3:
            settings::toggleAimAssist(settings);
            break;
        case 4:
            settings::toggleDamageNumbers(settings);
            break;
        default:
            break;
        }
        return;
    }

    if (tab_ != MenuTab::Loadout) {
        return;
    }

    if (selectedIndex_ == 0U) {
        auto it = std::find(kWeaponCycle.begin(), kWeaponCycle.end(), loadout.weaponId);
        if (it == kWeaponCycle.end()) {
            it = kWeaponCycle.begin();
        }
        const int index = static_cast<int>(std::distance(kWeaponCycle.begin(), it));
        const int next = (index + direction + static_cast<int>(kWeaponCycle.size())) % static_cast<int>(kWeaponCycle.size());
        loadout.weaponId = std::string(kWeaponCycle[static_cast<std::size_t>(next)]);
        return;
    }

    const auto slot = selectedAttachmentSlot();
    const auto slotAttachments = attachments.attachmentsForSlot(slot);
    if (slotAttachments.empty()) {
        return;
    }

    const auto currentId = weapons::selectedAttachmentId(loadout, slot);
    auto it = std::find_if(slotAttachments.begin(), slotAttachments.end(), [currentId](const weapons::AttachmentDefinition* attachment) {
        return attachment != nullptr && attachment->id == currentId;
    });
    if (it == slotAttachments.end()) {
        it = slotAttachments.begin();
    }

    const int index = static_cast<int>(std::distance(slotAttachments.begin(), it));
    const int next = (index + direction + static_cast<int>(slotAttachments.size())) % static_cast<int>(slotAttachments.size());
    weapons::setAttachment(loadout, slot, slotAttachments[static_cast<std::size_t>(next)]->id);
}

std::size_t GameMenu::itemCountForTab() const {
    switch (tab_) {
    case MenuTab::Play:
        return 3U;
    case MenuTab::Gamemodes:
        return 2U;
    case MenuTab::Loadout:
        return 1U + weapons::kAttachmentSlotCount;
    case MenuTab::Character:
        return 4U;
    case MenuTab::Settings:
        return 5U;
    case MenuTab::Account:
        return 4U;
    }
    return 1U;
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
