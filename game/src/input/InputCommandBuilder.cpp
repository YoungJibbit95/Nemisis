#include "nemisis/input/InputCommandBuilder.hpp"

#include "nemisis/input/InputBindings.hpp"

#include <algorithm>
#include <array>
#include <string_view>

namespace nemisis::input {

namespace {

[[nodiscard]] novacore::platform::InputActionState state(
    const novacore::platform::InputActionMap& actions,
    std::string_view action) {
    return actions.stateOrDefault(action);
}

[[nodiscard]] bool down(const novacore::platform::InputActionMap& actions, std::string_view action) {
    return state(actions, action).down;
}

[[nodiscard]] bool pressed(const novacore::platform::InputActionMap& actions, std::string_view action) {
    return state(actions, action).pressed;
}

[[nodiscard]] float activePositiveValue(const novacore::platform::InputActionMap& actions, std::string_view action) {
    const auto current = state(actions, action);
    if (!current.down) {
        return 0.0F;
    }

    return std::max(0.0F, current.value);
}

[[nodiscard]] novacore::platform::InputDeviceKind dominantDevice(
    const novacore::platform::InputActionMap& actions) {
    const std::array<std::string_view, 16> actionNames{
        actions::MoveForward,
        actions::MoveBackward,
        actions::MoveLeft,
        actions::MoveRight,
        actions::LookRight,
        actions::LookUp,
        actions::Jump,
        actions::Dash,
        actions::Slide,
        actions::Sprint,
        actions::TacticalSprint,
        actions::Mantle,
        actions::Fire,
        actions::Ads,
        actions::Reload,
        actions::DoubleJump,
    };

    for (const auto action : actionNames) {
        const auto current = state(actions, action);
        if (current.down || current.pressed || current.released) {
            return current.device;
        }
    }

    return novacore::platform::InputDeviceKind::KeyboardMouse;
}

} // namespace

player::PlayerInputCommand buildPlayerInputCommand(
    const novacore::platform::InputActionMap& actions,
    std::uint64_t tick) {
    return buildPlayerInputCommand(actions, tick, settings::GameSettings{});
}

player::PlayerInputCommand buildPlayerInputCommand(
    const novacore::platform::InputActionMap& actions,
    std::uint64_t tick,
    const settings::GameSettings& settings) {
    player::PlayerInputCommand command{};
    command.tick = tick;

    const float right = activePositiveValue(actions, actions::MoveRight);
    const float left = activePositiveValue(actions, actions::MoveLeft);
    const float forward = activePositiveValue(actions, actions::MoveForward);
    const float backward = activePositiveValue(actions, actions::MoveBackward);

    command.move.x = std::clamp(right - left, -1.0F, 1.0F);
    command.move.y = std::clamp(forward - backward, -1.0F, 1.0F);
    command.jumpPressed = pressed(actions, actions::Jump);
    command.jumpHeld = down(actions, actions::Jump);
    command.doubleJumpPressed = pressed(actions, actions::DoubleJump);
    command.crouchHeld = down(actions, actions::Slide);
    command.slidePressed = pressed(actions, actions::Slide);
    command.slideHeld = down(actions, actions::Slide);
    command.sprintHeld = down(actions, actions::Sprint);
    command.tacticalSprintHeld = down(actions, actions::TacticalSprint);
    command.dashPressed = pressed(actions, actions::Dash);
    command.mantlePressed = pressed(actions, actions::Mantle);
    command.mantleHeld = down(actions, actions::Mantle);

    command.fireHeld = down(actions, actions::Fire);
    command.adsHeld = down(actions, actions::Ads);
    command.reloadPressed = pressed(actions, actions::Reload);
    command.reloadHeld = down(actions, actions::Reload);
    command.device = dominantDevice(actions);

    const bool controller = command.device == novacore::platform::InputDeviceKind::Controller;
    command.look.x = state(actions, actions::LookRight).value *
        settings::effectiveLookScaleX(settings, controller, command.adsHeld);
    command.look.y = state(actions, actions::LookUp).value *
        settings::effectiveLookScaleY(settings, controller, command.adsHeld);

    return command;
}

} // namespace nemisis::input
