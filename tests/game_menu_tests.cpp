#include "nemisis/input/InputBindings.hpp"
#include "nemisis/settings/GameSettings.hpp"
#include "nemisis/ui/GameMenu.hpp"
#include "nemisis/weapons/WeaponAttachments.hpp"

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

} // namespace

int main() {
    testDirectDevRangeSelection();
    testMenuConfirmAndBack();
    testDebugToggle();
    testDebugPageCycle();
    testMenuTabsSettingsAndLoadoutMutateRuntimeData();

    if (failures > 0) {
        std::cerr << failures << " game menu test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis game menu tests passed\n";
    return EXIT_SUCCESS;
}
