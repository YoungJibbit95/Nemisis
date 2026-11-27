#include "nemisis/input/InputBindings.hpp"
#include "nemisis/input/InputCommandBuilder.hpp"
#include "nemisis/input/GameplayInputBuffer.hpp"
#include "nemisis/settings/GameSettings.hpp"
#include "novacore/core/FixedStep.hpp"

#include <cmath>
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
        // Exercise the raw SDL right-click that Window forwards into the input snapshot.
        {novacore::platform::InputControlKind::MouseButton, 3},
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

void testDevRangeUtilityActionsAreBound() {
    auto actionMap = nemisis::input::createDefaultActionMap();
    novacore::platform::InputSnapshot snapshot;
    snapshot.setButton(
        {novacore::platform::InputControlKind::KeyboardKey, nemisis::input::key_codes::V},
        true,
        novacore::platform::InputDeviceKind::KeyboardMouse);

    actionMap.update(snapshot);
    expect(
        actionMap.stateOrDefault(nemisis::input::actions::CycleDrillVariant).pressed,
        "cycle drill variant is bound for dev range gameplay");
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
    buffer.captureFrame(actionMap);
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

    buffer.captureFrame(actionMap);
    command = buffer.consumeTick(actionMap, 19);
    expect(command.jumpPressed, "buffer restores jump edge for the next fixed tick");
    expect(command.doubleJumpPressed, "buffer restores shared double-jump edge for the next fixed tick");
    expect(command.reloadPressed, "buffer restores reload edge for the next fixed tick");
    expect(!buffer.hasPendingEdges(), "gameplay input buffer consumes latched edges exactly once");

    const auto secondCommand = buffer.consumeTick(actionMap, 20);
    expect(!secondCommand.jumpPressed && !secondCommand.reloadPressed, "consumed input buffer does not repeat edges");
}

void testMouseDisplacementAcrossRenderRates() {
    using namespace nemisis::input;
    using namespace novacore::platform;
    for (const int framesPerSecond : {30, 60, 144, 240}) {
        auto actions = createDefaultActionMap();
        GameplayInputBuffer buffer;
        InputSnapshot snapshot;
        novacore::core::FixedStepAccumulator clock({60.0, 0.25});
        novacore::math::Vec2 total{};
        const auto advance = [&](double seconds) {
            clock.advance(seconds);
            while (clock.shouldStep()) {
                const auto command = buffer.consumeTick(actions, clock.consumedSteps());
                total.x += command.look.x;
                total.y += command.look.y;
                clock.consumeStep();
            }
        };

        // Follow Application's order: advance pending fixed ticks, then poll this frame's input.
        // The same 240 horizontal and -120 vertical counts arrive over one second.
        int sentCounts = 0;
        for (int frame = 1; frame <= framesPerSecond; ++frame) {
            advance(1.0 / framesPerSecond);
            snapshot.beginFrame();
            const int nextCounts = (240 * frame) / framesPerSecond;
            const float dx = static_cast<float>(nextCounts - sentCounts);
            sentCounts = nextCounts;
            snapshot.addAxisDelta({InputControlKind::MouseAxis, mouse_axes::X}, dx, InputDeviceKind::KeyboardMouse);
            snapshot.addAxisDelta({InputControlKind::MouseAxis, mouse_axes::Y}, -dx * 0.5F, InputDeviceKind::KeyboardMouse);
            actions.update(snapshot);
            buffer.captureFrame(actions);
        }
        advance(0.10); // Last poll is followed by several catch-up ticks.
        expect(std::abs(total.x - 24.0F) < 0.0001F && std::abs(total.y - 12.0F) < 0.0001F,
               "physical mouse displacement is consumed exactly once at 30/60/144/240 Hz and through a hitch");
    }
}

void testMixedLookSourcesAndShortTriggerTap() {
    using namespace nemisis::input;
    using namespace novacore::platform;
    auto actions = createDefaultActionMap();
    InputSnapshot snapshot;
    GameplayInputBuffer buffer;
    nemisis::settings::GameSettings settings{};
    settings.mouse.sensitivityX = 2.0F;
    settings.mouse.adsMultiplier = 0.5F;
    settings.mouse.invertY = true;
    settings.controller.lookSensitivityX = 3.0F;
    const InputControl stick{InputControlKind::GamepadAxis, gamepad_axes::RightX};
    const InputControl ads{InputControlKind::MouseButton, mouse_codes::Right};
    const InputControl fire{InputControlKind::MouseButton, mouse_codes::Left};
    const InputControl mouseX{InputControlKind::MouseAxis, mouse_axes::X};
    const InputControl mouseY{InputControlKind::MouseAxis, mouse_axes::Y};

    snapshot.setAxis(stick, 0.5F, InputDeviceKind::Controller);
    snapshot.addAxisDelta(mouseX, 10.0F, InputDeviceKind::KeyboardMouse);
    snapshot.addAxisDelta(mouseY, 4.0F, InputDeviceKind::KeyboardMouse);
    snapshot.setButton(ads, true, InputDeviceKind::KeyboardMouse);
    snapshot.setButton(fire, true, InputDeviceKind::KeyboardMouse);
    actions.update(snapshot);
    buffer.captureFrame(actions, settings);

    snapshot.beginFrame();
    snapshot.addAxisDelta(mouseX, -3.0F, InputDeviceKind::KeyboardMouse);
    snapshot.setButton(ads, false, InputDeviceKind::KeyboardMouse);
    snapshot.setButton(fire, false, InputDeviceKind::KeyboardMouse);
    actions.update(snapshot);
    buffer.captureFrame(actions, settings);

    const auto first = buffer.consumeTick(actions, 1);
    const auto second = buffer.consumeTick(actions, 2);
    // Mouse x: 1.0 under ADS and -0.6 after release. Stick x: 0.5 * 2.4 * 3 per tick.
    expect(std::abs(first.look.x - 4.0F) < 0.0001F && std::abs(first.look.y - 0.2F) < 0.0001F,
           "mouse and stick can share an axis and use their own sensitivity and ADS timing");
    expect(std::abs(second.look.x - 3.6F) < 0.0001F && second.look.y == 0.0F,
           "only the held stick repeats during catch-up");
    expect(first.fireHeld && !second.fireHeld && !first.adsHeld,
           "a completed trigger tap survives until one simulation tick and ADS releases normally");

    snapshot.beginFrame();
    snapshot.setAxis(stick, 0.0F, InputDeviceKind::Controller);
    actions.update(snapshot);
    buffer.captureFrame(actions, settings);
    expect(buffer.consumeTick(actions, 3).look.x == 0.0F, "releasing the stick stops its turn");

    snapshot.beginFrame();
    snapshot.setAxis(stick, 0.5F, InputDeviceKind::Controller);
    snapshot.addAxisDelta(mouseX, 10.0F, InputDeviceKind::KeyboardMouse);
    snapshot.setButton(fire, true, InputDeviceKind::KeyboardMouse);
    snapshot.setButton(fire, false, InputDeviceKind::KeyboardMouse);
    actions.update(snapshot);
    buffer.captureFrame(actions, settings);
    expect(buffer.consumeTick(actions, 4).fireHeld && !buffer.consumeTick(actions, 5).fireHeld,
           "a press and release within the same event poll also fires for just one tick");

    snapshot.beginFrame();
    snapshot.addAxisDelta(mouseX, 5.0F, InputDeviceKind::KeyboardMouse);
    snapshot.setButton(fire, true, InputDeviceKind::KeyboardMouse);
    snapshot.setButton(fire, false, InputDeviceKind::KeyboardMouse);
    actions.update(snapshot);
    buffer.captureFrame(actions, settings);
    buffer.clear();
    const auto afterClear = buffer.consumeTick(actions, 6);
    expect(afterClear.look.lengthSquared() == 0.0F && !afterClear.fireHeld,
           "clearing gameplay input discards pending look and tap at a menu or map boundary");
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
    testDevRangeUtilityActionsAreBound();
    testGameplayInputBufferCarriesEdgesAcrossFrameBeforeFixedTick();
    testMouseDisplacementAcrossRenderRates();
    testMixedLookSourcesAndShortTriggerTap();

    if (failures > 0) {
        std::cerr << failures << " input command test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis input command tests passed\n";
    return EXIT_SUCCESS;
}
