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
    sample.rangeSession.score.targetsEliminated = 3;
    sample.rangeSession.score.shotsFired = 10;
    sample.rangeSession.score.shotsHit = 7;

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
    expect(hasText("ELIMS"), "dev range HUD keeps compact score strip visible");
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

} // namespace

int main() {
    testDirectDevRangeSelection();
    testMenuConfirmAndBack();
    testDebugToggle();
    testDebugPageCycle();
    testMenuTabsSettingsAndLoadoutMutateRuntimeData();
    testPointerNavigationActivatesMenuRows();
    testDevRangeHudUsesPlayableResponsiveLayout();

    if (failures > 0) {
        std::cerr << failures << " game menu test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis game menu tests passed\n";
    return EXIT_SUCCESS;
}
