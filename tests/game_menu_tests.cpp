#include "nemisis/input/InputBindings.hpp"
#include "nemisis/ui/GameMenu.hpp"

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

    expect(menu.screen() == nemisis::ui::GameScreen::DevRange, "digit 1 opens dev range");
    expect(menu.gameplayActive(), "dev range activates gameplay");
}

void testMenuConfirmAndBack() {
    auto actions = nemisis::input::createDefaultActionMap();
    nemisis::ui::GameMenu menu;

    press(actions, nemisis::input::key_codes::Down);
    menu.update(actions);
    release(actions);
    menu.update(actions);

    press(actions, nemisis::input::key_codes::Enter);
    menu.update(actions);
    expect(menu.screen() == nemisis::ui::GameScreen::TeamDeathmatch, "down enter opens TDM");

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

} // namespace

int main() {
    testDirectDevRangeSelection();
    testMenuConfirmAndBack();
    testDebugToggle();

    if (failures > 0) {
        std::cerr << failures << " game menu test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis game menu tests passed\n";
    return EXIT_SUCCESS;
}
