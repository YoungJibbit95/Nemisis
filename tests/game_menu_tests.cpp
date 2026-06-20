#include "nemisis/input/InputBindings.hpp"
#include "nemisis/dev/DevTargetRange.hpp"
#include "nemisis/dev/GreyboxWorld.hpp"
#include "nemisis/settings/GameSettings.hpp"
#include "nemisis/ui/GameMenu.hpp"
#include "nemisis/weapons/WeaponAttachments.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

int failures = 0;

void expect(bool condition, std::string_view message) {
    if (condition) {
        return;
    }

    ++failures;
    std::cerr << "[fail] " << message << '\n';
}

void press(novacore::platform::InputActionMap& actions, std::uint16_t key) {
    novacore::platform::InputSnapshot snapshot;
    snapshot.setButton(
        {novacore::platform::InputControlKind::KeyboardKey, key},
        true,
        novacore::platform::InputDeviceKind::KeyboardMouse);
    actions.update(snapshot);
}

void release(novacore::platform::InputActionMap& actions) {
    novacore::platform::InputSnapshot snapshot;
    actions.update(snapshot);
}

void testDirectDevRangeSelection() {
    auto actions = nemisis::input::createDefaultActionMap();
    nemisis::ui::GameMenu menu;

    press(actions, nemisis::input::key_codes::Digit1);
    menu.update(actions);

    expect(menu.screen() == nemisis::ui::GameScreen::Loading, "digit 1 starts dev range loading");
    expect(!menu.gameplayActive(), "loading is not gameplay active yet");
    menu.updateFrame(1.0);
    expect(menu.screen() == nemisis::ui::GameScreen::DevRange, "loading completes into dev range");
    expect(menu.gameplayActive(), "dev range activates gameplay");
}

void testMenuConfirmAndBack() {
    auto actions = nemisis::input::createDefaultActionMap();
    nemisis::ui::GameMenu menu;

    press(actions, nemisis::input::key_codes::E);
    menu.update(actions);
    release(actions);
    menu.update(actions);

    press(actions, nemisis::input::key_codes::Enter);
    menu.update(actions);
    expect(menu.screen() == nemisis::ui::GameScreen::Loading, "confirming gamemode starts loading");
    menu.updateFrame(1.0);
    expect(menu.screen() == nemisis::ui::GameScreen::TeamDeathmatch, "loading completes into TDM");

    release(actions);
    menu.update(actions);
    press(actions, nemisis::input::key_codes::Escape);
    menu.update(actions);
    expect(menu.screen() == nemisis::ui::GameScreen::MainMenu, "escape returns to menu");
}

void testDebugToggle() {
    auto actions = nemisis::input::createDefaultActionMap();
    nemisis::ui::GameMenu menu;

    expect(menu.debugOverlayEnabled(), "debug overlay starts enabled");
    press(actions, nemisis::input::key_codes::F1);
    menu.update(actions);
    expect(!menu.debugOverlayEnabled(), "F1 toggles debug overlay");
}

void testDebugPageCycle() {
    auto actions = nemisis::input::createDefaultActionMap();
    nemisis::ui::GameMenu menu;

    expect(menu.debugPage() == nemisis::ui::DebugPage::Gameplay, "debug page starts at gameplay");
    press(actions, nemisis::input::key_codes::Tab);
    menu.update(actions);
    expect(menu.debugPage() == nemisis::ui::DebugPage::Network, "tab cycles to network page");

    release(actions);
    menu.update(actions);
    press(actions, nemisis::input::key_codes::Tab);
    menu.update(actions);
    expect(menu.debugPage() == nemisis::ui::DebugPage::Assets, "tab cycles to assets page");
}

void testMenuTabsSettingsAndLoadoutMutateRuntimeData() {
    auto actions = nemisis::input::createDefaultActionMap();
    nemisis::ui::GameMenu menu;
    nemisis::settings::GameSettings settings{};
    nemisis::weapons::AttachmentRegistry attachments;
    attachments.registerPrototypeAttachments();
    auto loadout = nemisis::weapons::defaultPrototypeLoadout();

    press(actions, nemisis::input::key_codes::E);
    menu.update(actions, settings, loadout, attachments);
    release(actions);
    menu.update(actions, settings, loadout, attachments);
    press(actions, nemisis::input::key_codes::E);
    menu.update(actions, settings, loadout, attachments);
    expect(menu.tab() == nemisis::ui::MenuTab::Loadout, "E cycles to loadout tab");

    release(actions);
    menu.update(actions, settings, loadout, attachments);
    const auto originalWeapon = loadout.weaponId;
    press(actions, nemisis::input::key_codes::Right);
    menu.update(actions, settings, loadout, attachments);
    expect(loadout.weaponId != originalWeapon, "right arrow cycles selected weapon in loadout tab");

    release(actions);
    menu.update(actions, settings, loadout, attachments);
    press(actions, nemisis::input::key_codes::E);
    menu.update(actions, settings, loadout, attachments);
    release(actions);
    menu.update(actions, settings, loadout, attachments);
    press(actions, nemisis::input::key_codes::E);
    menu.update(actions, settings, loadout, attachments);
    expect(menu.tab() == nemisis::ui::MenuTab::Settings, "E cycles to settings tab");

    release(actions);
    menu.update(actions, settings, loadout, attachments);
    const float originalSensitivity = settings.mouse.sensitivityX;
    press(actions, nemisis::input::key_codes::Right);
    menu.update(actions, settings, loadout, attachments);
    expect(settings.mouse.sensitivityX > originalSensitivity, "right arrow adjusts mouse sensitivity live");
}

void testPointerNavigationActivatesMenuRows() {
    auto actions = nemisis::input::createDefaultActionMap();
    nemisis::ui::GameMenu menu;
    nemisis::settings::GameSettings settings{};
    nemisis::weapons::AttachmentRegistry attachments;
    attachments.registerPrototypeAttachments();
    auto loadout = nemisis::weapons::defaultPrototypeLoadout();

    menu.update(actions, settings, loadout, attachments, nemisis::ui::MenuPointerState{
        430.0F,
        152.0F,
        true,
        true,
        true,
        false,
    });
    expect(menu.tab() == nemisis::ui::MenuTab::Loadout, "pointer click selects loadout tab");

    const auto originalWeapon = loadout.weaponId;
    menu.update(actions, settings, loadout, attachments, nemisis::ui::MenuPointerState{
        420.0F,
        232.0F,
        true,
        true,
        true,
        false,
    });
    expect(loadout.weaponId != originalWeapon, "pointer click activates selected loadout row");

    menu.update(actions, settings, loadout, attachments, nemisis::ui::MenuPointerState{
        770.0F,
        152.0F,
        true,
        true,
        true,
        false,
    });
    expect(menu.tab() == nemisis::ui::MenuTab::Settings, "pointer click selects settings tab");

    const float originalSensitivity = settings.mouse.sensitivityX;
    menu.update(actions, settings, loadout, attachments, nemisis::ui::MenuPointerState{
        440.0F,
        232.0F,
        true,
        true,
        true,
        false,
    });
    expect(settings.mouse.sensitivityX > originalSensitivity, "pointer click adjusts settings row");

    menu.showMainMenu(nemisis::ui::MenuTab::Play);
    menu.update(actions, settings, loadout, attachments, nemisis::ui::MenuPointerState{
        470.0F,
        232.0F,
        true,
        true,
        true,
        false,
    });
    expect(menu.screen() == nemisis::ui::GameScreen::Loading, "pointer click starts firing range loading");
}

void testDevRangeHudUsesPlayableResponsiveLayout() {
    nemisis::ui::GameMenu menu;
    menu.showDevRange();

    nemisis::dev::DevSandboxSample sample{};
    sample.playerHealth.health = 114.0F;
    sample.playerHealth.maxHealth = 150.0F;
    sample.weapon.weaponId = "ar_01";
    sample.weapon.ammoInMagazine = 24;
    sample.weapon.adsAlpha = 0.35F;
    sample.targetRange = nemisis::dev::makeDefaultDevTargetRange();
    sample.targetRange.lanes[1].pressure01 = 0.64F;
    sample.targetRange.lanes[1].pressureActive = true;
    sample.rangeSession.score.targetsEliminated = 3;
    sample.rangeSession.score.shotsFired = 10;
    sample.rangeSession.score.shotsHit = 7;
    sample.rangeSession.drill.score = 1240;
    sample.rangeSession.drill.timeRemainingSeconds = 42.5F;
    sample.rangeSession.drill.latestTtkSeconds = 0.38F;
    sample.rangeSession.drill.recoilControlScore = 86.0F;
    nemisis::dev::ensureDevRangeLaneScore(sample.rangeSession, 1U, "center_20m", "CENTER 20M");

    nemisis::weapons::AttachmentRegistry attachments;
    attachments.registerPrototypeAttachments();
    auto loadout = nemisis::weapons::defaultPrototypeLoadout();
    nemisis::weapons::AttachmentBuildSummary attachmentSummary{};
    attachmentSummary.effectiveWeapon.id = "ar_01";
    attachmentSummary.effectiveWeapon.displayName = "NOVA RIFLE";
    attachmentSummary.effectiveMagazineSize = 30;

    novacore::render::RenderBackendFrameStats backendStats{};
    backendStats.swapchainWidth = 1920;
    backendStats.swapchainHeight = 1080;
    novacore::render::RenderFrameInfo frame{};
    menu.appendRenderCommands(
        frame,
        sample,
        nemisis::dev::GreyboxWorld{},
        "Vulkan",
        "Vulkan 1.4 test",
        0,
        {},
        {},
        backendStats,
        {},
        {},
        loadout,
        attachments,
        attachmentSummary,
        {});

    const auto hasText = [&frame](std::string_view text) {
        return std::any_of(
            frame.debugTexts.begin(),
            frame.debugTexts.end(),
            [text](const novacore::render::DebugText& command) {
                return command.text.find(text) != std::string::npos;
            });
    };

    expect(hasText("OPERATOR"), "dev range HUD keeps player health anchored as operator panel");
    expect(hasText("NOVA RIFLE"), "dev range HUD keeps loadout weapon visible");
    expect(hasText("DRILL"), "dev range HUD keeps timed drill strip visible");
    expect(hasText("PRECISION"), "dev range HUD exposes active drill variant");
    expect(hasText("ACCURACY"), "dev range HUD exposes active drill objective");
    expect(hasText("CTRL"), "dev range HUD exposes recoil-control scoring");
    expect(hasText("TTK"), "dev range HUD exposes measured TTK panel data");
    expect(hasText("CENTER 20M"), "dev range HUD exposes active lane breakdown");
    expect(hasText("PRESS 64%"), "dev range HUD exposes active lane pressure");
    expect(hasText("DEBUG Gameplay"), "debug overlay renders as compact gameplay panel");
    expect(!hasText("TARGET LANE"), "normal dev range HUD no longer renders the large target lane debug panel");
    expect(
        std::any_of(
            frame.debugRects.begin(),
            frame.debugRects.end(),
            [](const novacore::render::DebugRect& rect) {
                return rect.x > 1200.0F && rect.y > 890.0F && rect.width > 400.0F;
            }),
        "loadout panel anchors to the bottom-right after 1920x1080 scaling");
}

void testNetworkDebugOverlayIncludesPredictionTelemetry() {
    auto actions = nemisis::input::createDefaultActionMap();
    nemisis::ui::GameMenu menu;
    menu.showDevRange();

    press(actions, nemisis::input::key_codes::Tab);
    menu.update(actions);
    expect(menu.debugPage() == nemisis::ui::DebugPage::Network, "tab selects network debug page");

    nemisis::dev::DevSandboxSample sample{};
    sample.targetRange = nemisis::dev::makeDefaultDevTargetRange();
    sample.network.pendingCommandCount = 2;
    sample.netBridge.sentCommandPackets = 8;
    sample.netBridge.receivedAckPackets = 7;
    sample.netBridge.lastAcknowledgedTick = 42;
    sample.prediction.storedSamples = 5;
    sample.prediction.unacknowledgedTickSpan = 3;
    sample.prediction.hasLatestError = true;
    sample.prediction.latestError.positionErrorMeters = 0.19F;
    sample.prediction.latestError.exceedsCorrectionThreshold = true;
    sample.snapshots.storedSnapshots = 4;
    sample.snapshots.newestServerTick = 45;
    sample.snapshots.hasNewestServerTick = true;

    nemisis::weapons::AttachmentRegistry attachments;
    attachments.registerPrototypeAttachments();
    auto loadout = nemisis::weapons::defaultPrototypeLoadout();
    nemisis::weapons::AttachmentBuildSummary attachmentSummary{};
    attachmentSummary.effectiveWeapon.id = "ar_01";
    attachmentSummary.effectiveWeapon.displayName = "NOVA RIFLE";
    attachmentSummary.effectiveMagazineSize = 30;

    novacore::render::RenderBackendFrameStats backendStats{};
    backendStats.swapchainWidth = 1280;
    backendStats.swapchainHeight = 720;
    nemisis::dev::DevRangeRenderSceneStats sceneStats{};
    sceneStats.worldBoxCount = 50;
    sceneStats.meshInstanceCount = 36;

    novacore::render::RenderFrameInfo frame{};
    menu.appendRenderCommands(
        frame,
        sample,
        nemisis::dev::GreyboxWorld{},
        "Vulkan",
        "Vulkan 1.4 test",
        0,
        {},
        {},
        backendStats,
        sceneStats,
        {},
        loadout,
        attachments,
        attachmentSummary,
        {});

    const auto hasText = [&frame](std::string_view text) {
        return std::any_of(
            frame.debugTexts.begin(),
            frame.debugTexts.end(),
            [text](const novacore::render::DebugText& command) {
                return command.text.find(text) != std::string::npos;
            });
    };

    expect(hasText("DEBUG Network"), "network debug overlay renders selected page");
    expect(hasText("PRED"), "network debug overlay labels prediction history");
    expect(hasText("5/3"), "network debug overlay shows prediction sample/span telemetry");
    expect(hasText("ERR"), "network debug overlay labels prediction error");
    expect(hasText("0.19m"), "network debug overlay shows latest prediction error magnitude");
    expect(hasText("SNAP"), "network debug overlay labels snapshot interpolation");
    expect(hasText("4 n45"), "network debug overlay shows snapshot count and newest tick");
}

} // namespace

int main() {
    testDirectDevRangeSelection();
    testMenuConfirmAndBack();
    testDebugToggle();
    testDebugPageCycle();
    testMenuTabsSettingsAndLoadoutMutateRuntimeData();
    testPointerNavigationActivatesMenuRows();
    testDevRangeHudUsesPlayableResponsiveLayout();
    testNetworkDebugOverlayIncludesPredictionTelemetry();

    if (failures > 0) {
        std::cerr << failures << " game menu test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis game menu tests passed\n";
    return EXIT_SUCCESS;
}
