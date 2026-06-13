#include "nemisis/input/InputBindings.hpp"
#include "nemisis/input/InputCommandBuilder.hpp"
#include "nemisis/input/GameplayInputBuffer.hpp"
#include "nemisis/settings/GameSettings.hpp"

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

void testMovementActionsBuildCommand() {
    auto actionMap = nemisis::input::createDefaultActionMap();
    novacore::platform::InputSnapshot snapshot;
    snapshot.setButton(
        {novacore::platform::InputControlKind::KeyboardKey, nemisis::input::key_codes::W},
        true,
        novacore::platform::InputDeviceKind::KeyboardMouse);
    snapshot.setButton(
        {novacore::platform::InputControlKind::KeyboardKey, nemisis::input::key_codes::D},
        true,
        novacore::platform::InputDeviceKind::KeyboardMouse);

    actionMap.update(snapshot);
    const auto command = nemisis::input::buildPlayerInputCommand(actionMap, 42);

    expect(command.tick == 42, "input command keeps tick");
    expect(command.move.x > 0.9F, "right action maps to positive x movement");
    expect(command.move.y > 0.9F, "forward action maps to positive y movement");
}

void testPressedActionsBuildCommand() {
    auto actionMap = nemisis::input::createDefaultActionMap();
    novacore::platform::InputSnapshot snapshot;
    snapshot.setButton(
        {novacore::platform::InputControlKind::KeyboardKey, nemisis::input::key_codes::LeftAlt},
        true,
        novacore::platform::InputDeviceKind::KeyboardMouse);
    snapshot.setButton(
        {novacore::platform::InputControlKind::MouseButton, nemisis::input::mouse_codes::Left},
        true,
        novacore::platform::InputDeviceKind::KeyboardMouse);

    actionMap.update(snapshot);
    const auto command = nemisis::input::buildPlayerInputCommand(actionMap, 7);

    expect(command.dashPressed, "dash pressed maps to input command");
    expect(command.fireHeld, "fire held maps to input command");
    expect(command.device == novacore::platform::InputDeviceKind::KeyboardMouse, "device remains keyboard/mouse");
}

void testHeldActionsBuildCommand() {
    auto actionMap = nemisis::input::createDefaultActionMap();
    novacore::platform::InputSnapshot snapshot;
    snapshot.setButton(
        {novacore::platform::InputControlKind::KeyboardKey, nemisis::input::key_codes::Space},
        true,
        novacore::platform::InputDeviceKind::KeyboardMouse);
    snapshot.setButton(
        {novacore::platform::InputControlKind::KeyboardKey, nemisis::input::key_codes::C},
        true,
        novacore::platform::InputDeviceKind::KeyboardMouse);
    snapshot.setButton(
        {novacore::platform::InputControlKind::KeyboardKey, nemisis::input::key_codes::R},
        true,
        novacore::platform::InputDeviceKind::KeyboardMouse);

    actionMap.update(snapshot);
    const auto command = nemisis::input::buildPlayerInputCommand(actionMap, 8);

    expect(command.jumpPressed && command.jumpHeld, "jump reports pressed and held on first frame");
    expect(command.mantlePressed && command.mantleHeld, "mantle shares jump hold intent");
    expect(command.slidePressed && command.slideHeld, "slide reports pressed and held");
    expect(command.reloadPressed && command.reloadHeld, "reload reports pressed and held");

    snapshot.beginFrame();
    actionMap.update(snapshot);
    const auto held = nemisis::input::buildPlayerInputCommand(actionMap, 9);
    expect(!held.jumpPressed && held.jumpHeld, "held jump does not repeat pressed");
    expect(!held.slidePressed && held.slideHeld, "held slide does not repeat pressed");
    expect(!held.reloadPressed && held.reloadHeld, "held reload does not repeat pressed");
}

void testControllerActionsBuildCommand() {
    auto actionMap = nemisis::input::createDefaultActionMap();
    novacore::platform::InputSnapshot snapshot;
    snapshot.setAxis(
        {novacore::platform::InputControlKind::GamepadAxis, nemisis::input::gamepad_axes::LeftX},
        0.75F,
        novacore::platform::InputDeviceKind::Controller);
    snapshot.setAxis(
        {novacore::platform::InputControlKind::GamepadAxis, nemisis::input::gamepad_axes::LeftY},
        -0.5F,
        novacore::platform::InputDeviceKind::Controller);
    snapshot.setAxis(
        {novacore::platform::InputControlKind::GamepadAxis, nemisis::input::gamepad_axes::RightTrigger},
        1.0F,
        novacore::platform::InputDeviceKind::Controller);

    actionMap.update(snapshot);
    const auto command = nemisis::input::buildPlayerInputCommand(actionMap, 13);

    expect(command.move.x > 0.7F, "left stick x maps to movement x");
    expect(command.move.y > 0.4F, "left stick y maps to movement y");
    expect(command.fireHeld, "right trigger maps to fire");
    expect(command.device == novacore::platform::InputDeviceKind::Controller, "controller input becomes command device");
}

void testControllerDeadzoneDoesNotMove() {
    auto actionMap = nemisis::input::createDefaultActionMap();
    novacore::platform::InputSnapshot snapshot;
    snapshot.setAxis(
        {novacore::platform::InputControlKind::GamepadAxis, nemisis::input::gamepad_axes::LeftX},
        0.04F,
        novacore::platform::InputDeviceKind::Controller);

    actionMap.update(snapshot);
    const auto command = nemisis::input::buildPlayerInputCommand(actionMap, 14);

    expect(command.move.x == 0.0F, "left stick deadzone suppresses movement x");
}

void testMouseLookBuildsCommand() {
    auto actionMap = nemisis::input::createDefaultActionMap();
    novacore::platform::InputSnapshot snapshot;
    snapshot.addAxisDelta(
        {novacore::platform::InputControlKind::MouseAxis, nemisis::input::mouse_axes::X},
        10.0F,
        novacore::platform::InputDeviceKind::KeyboardMouse);
    snapshot.addAxisDelta(
        {novacore::platform::InputControlKind::MouseAxis, nemisis::input::mouse_axes::Y},
        -5.0F,
        novacore::platform::InputDeviceKind::KeyboardMouse);

    actionMap.update(snapshot);
    const auto command = nemisis::input::buildPlayerInputCommand(actionMap, 15);

    expect(command.look.x > 0.9F, "mouse x builds positive look x");
    expect(command.look.y > 0.4F, "mouse y builds positive look y with inverted scale");
}

void testControllerLookBuildsCommand() {
    auto actionMap = nemisis::input::createDefaultActionMap();
    novacore::platform::InputSnapshot snapshot;
    snapshot.setAxis(
        {novacore::platform::InputControlKind::GamepadAxis, nemisis::input::gamepad_axes::RightX},
        0.5F,
        novacore::platform::InputDeviceKind::Controller);
    snapshot.setAxis(
        {novacore::platform::InputControlKind::GamepadAxis, nemisis::input::gamepad_axes::RightY},
        -0.5F,
        novacore::platform::InputDeviceKind::Controller);

    actionMap.update(snapshot);
    const auto command = nemisis::input::buildPlayerInputCommand(actionMap, 16);

    expect(command.look.x > 1.1F, "right stick x builds look x");
    expect(command.look.y > 0.9F, "right stick y builds look y");
    expect(command.device == novacore::platform::InputDeviceKind::Controller, "controller look becomes command device");
}

void testLookSensitivitySettingsScaleCommand() {
    auto actionMap = nemisis::input::createDefaultActionMap();
    novacore::platform::InputSnapshot snapshot;
    snapshot.addAxisDelta(
        {novacore::platform::InputControlKind::MouseAxis, nemisis::input::mouse_axes::X},
        10.0F,
        novacore::platform::InputDeviceKind::KeyboardMouse);
    snapshot.addAxisDelta(
        {novacore::platform::InputControlKind::MouseAxis, nemisis::input::mouse_axes::Y},
        -10.0F,
        novacore::platform::InputDeviceKind::KeyboardMouse);
    snapshot.setButton(
        {novacore::platform::InputControlKind::MouseButton, nemisis::input::mouse_codes::Right},
        true,
        novacore::platform::InputDeviceKind::KeyboardMouse);

    nemisis::settings::GameSettings settings{};
    settings.mouse.sensitivityX = 2.0F;
    settings.mouse.sensitivityY = 1.5F;
    settings.mouse.adsMultiplier = 0.5F;

    actionMap.update(snapshot);
    const auto command = nemisis::input::buildPlayerInputCommand(actionMap, 17, settings);

    expect(command.adsHeld, "settings scale test holds ADS");
    expect(command.look.x > 0.99F && command.look.x < 1.01F, "mouse x is scaled by sensitivity and ADS multiplier");
    expect(command.look.y > 0.74F && command.look.y < 0.76F, "mouse y is scaled by sensitivity and ADS multiplier");
}

void testWeaponInteractionActionsBuildCommand() {
    auto actionMap = nemisis::input::createDefaultActionMap();
    novacore::platform::InputSnapshot snapshot;
    snapshot.setButton(
        {novacore::platform::InputControlKind::KeyboardKey, nemisis::input::key_codes::F},
        true,
        novacore::platform::InputDeviceKind::KeyboardMouse);
    snapshot.setButton(
        {novacore::platform::InputControlKind::KeyboardKey, nemisis::input::key_codes::Digit4},
        true,
        novacore::platform::InputDeviceKind::KeyboardMouse);
    snapshot.setButton(
        {novacore::platform::InputControlKind::KeyboardKey, nemisis::input::key_codes::Digit5},
        true,
        novacore::platform::InputDeviceKind::KeyboardMouse);
    snapshot.setButton(
        {novacore::platform::InputControlKind::KeyboardKey, nemisis::input::key_codes::Digit6},
        true,
        novacore::platform::InputDeviceKind::KeyboardMouse);

    actionMap.update(snapshot);
    const auto command = nemisis::input::buildPlayerInputCommand(actionMap, 18);

    expect(command.pickupWeaponPressed, "pickup weapon maps to input command");
    expect(command.switchWeaponPrimaryPressed, "primary weapon switch maps to input command");
    expect(command.switchWeaponSmgPressed, "smg weapon switch maps to input command");
    expect(command.switchWeaponSidearmPressed, "sidearm weapon switch maps to input command");
}

void testGameplayInputBufferCarriesEdgesAcrossFrameBeforeFixedTick() {
    auto actionMap = nemisis::input::createDefaultActionMap();
    nemisis::input::GameplayInputBuffer buffer;
    novacore::platform::InputSnapshot snapshot;
    snapshot.setButton(
        {novacore::platform::InputControlKind::KeyboardKey, nemisis::input::key_codes::Space},
        true,
        novacore::platform::InputDeviceKind::KeyboardMouse);
    snapshot.setButton(
        {novacore::platform::InputControlKind::KeyboardKey, nemisis::input::key_codes::R},
        true,
        novacore::platform::InputDeviceKind::KeyboardMouse);

    actionMap.update(snapshot);
    buffer.captureFrameEdges(actionMap);
    expect(buffer.hasPendingEdges(), "gameplay input buffer captures jump/reload frame edges");

    snapshot.beginFrame();
    snapshot.setButton(
        {novacore::platform::InputControlKind::KeyboardKey, nemisis::input::key_codes::Space},
        false,
        novacore::platform::InputDeviceKind::KeyboardMouse);
    snapshot.setButton(
        {novacore::platform::InputControlKind::KeyboardKey, nemisis::input::key_codes::R},
        false,
        novacore::platform::InputDeviceKind::KeyboardMouse);
    actionMap.update(snapshot);

    auto command = nemisis::input::buildPlayerInputCommand(actionMap, 19);
    expect(!command.jumpPressed && !command.reloadPressed, "base command no longer sees one-frame edge after release");

    buffer.consumeInto(command);
    expect(command.jumpPressed, "buffer restores jump edge for the next fixed tick");
    expect(command.doubleJumpPressed, "buffer restores shared double-jump edge for the next fixed tick");
    expect(command.reloadPressed, "buffer restores reload edge for the next fixed tick");
    expect(!buffer.hasPendingEdges(), "gameplay input buffer consumes latched edges exactly once");

    auto secondCommand = nemisis::input::buildPlayerInputCommand(actionMap, 20);
    buffer.consumeInto(secondCommand);
    expect(!secondCommand.jumpPressed && !secondCommand.reloadPressed, "consumed input buffer does not repeat edges");
}

} // namespace

int main() {
    testMovementActionsBuildCommand();
    testPressedActionsBuildCommand();
    testHeldActionsBuildCommand();
    testControllerActionsBuildCommand();
    testControllerDeadzoneDoesNotMove();
    testMouseLookBuildsCommand();
    testControllerLookBuildsCommand();
    testLookSensitivitySettingsScaleCommand();
    testWeaponInteractionActionsBuildCommand();
    testGameplayInputBufferCarriesEdgesAcrossFrameBeforeFixedTick();

    if (failures > 0) {
        std::cerr << failures << " input command test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis input command tests passed\n";
    return EXIT_SUCCESS;
}
