#include "nemisis/input/InputBindings.hpp"
#include "nemisis/input/InputCommandBuilder.hpp"

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

} // namespace

int main() {
    testMovementActionsBuildCommand();
    testPressedActionsBuildCommand();
    testControllerActionsBuildCommand();
    testControllerDeadzoneDoesNotMove();
    testMouseLookBuildsCommand();
    testControllerLookBuildsCommand();

    if (failures > 0) {
        std::cerr << failures << " input command test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis input command tests passed\n";
    return EXIT_SUCCESS;
}
