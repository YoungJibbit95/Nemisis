#include "nemisis/input/InputBindings.hpp"

#include <string>
#include <string_view>

namespace nemisis::input {

namespace {

novacore::platform::InputControl keyboard(std::uint16_t code) {
    return novacore::platform::InputControl{
        novacore::platform::InputControlKind::KeyboardKey,
        code,
    };
}

novacore::platform::InputControl mouse(std::uint16_t code) {
    return novacore::platform::InputControl{
        novacore::platform::InputControlKind::MouseButton,
        code,
    };
}

novacore::platform::InputControl mouseAxis(std::uint16_t code) {
    return novacore::platform::InputControl{
        novacore::platform::InputControlKind::MouseAxis,
        code,
    };
}

novacore::platform::InputControl gamepadButton(std::uint16_t code) {
    return novacore::platform::InputControl{
        novacore::platform::InputControlKind::GamepadButton,
        code,
    };
}

novacore::platform::InputControl gamepadAxis(std::uint16_t code) {
    return novacore::platform::InputControl{
        novacore::platform::InputControlKind::GamepadAxis,
        code,
    };
}

void bindButton(novacore::platform::InputActionMap& actionMap, std::string_view action, novacore::platform::InputControl control) {
    actionMap.bind(novacore::platform::InputBinding{
        std::string(action),
        control,
        1.0F,
        0.5F,
    });
}

void bindAxis(
    novacore::platform::InputActionMap& actionMap,
    std::string_view action,
    novacore::platform::InputControl control,
    float scale,
    float threshold) {
    actionMap.bind(novacore::platform::InputBinding{
        std::string(action),
        control,
        scale,
        threshold,
    });
}

} // namespace

novacore::platform::InputActionMap createDefaultActionMap() {
    novacore::platform::InputActionMap actionMap;

    bindButton(actionMap, actions::MoveForward, keyboard(key_codes::W));
    bindButton(actionMap, actions::MoveBackward, keyboard(key_codes::S));
    bindButton(actionMap, actions::MoveLeft, keyboard(key_codes::A));
    bindButton(actionMap, actions::MoveRight, keyboard(key_codes::D));
    bindButton(actionMap, actions::Jump, keyboard(key_codes::Space));
    bindButton(actionMap, actions::DoubleJump, keyboard(key_codes::Space));
    bindButton(actionMap, actions::Dash, keyboard(key_codes::LeftAlt));
    bindButton(actionMap, actions::Slide, keyboard(key_codes::C));
    bindButton(actionMap, actions::Sprint, keyboard(key_codes::LeftShift));
    bindButton(actionMap, actions::TacticalSprint, keyboard(key_codes::LeftShift));
    bindButton(actionMap, actions::Mantle, keyboard(key_codes::Space));
    bindButton(actionMap, actions::Fire, mouse(mouse_codes::Left));
    bindButton(actionMap, actions::Ads, mouse(mouse_codes::Right));
    bindButton(actionMap, actions::Reload, keyboard(key_codes::R));
    bindButton(actionMap, actions::MenuUp, keyboard(key_codes::Up));
    bindButton(actionMap, actions::MenuDown, keyboard(key_codes::Down));
    bindButton(actionMap, actions::MenuConfirm, keyboard(key_codes::Enter));
    bindButton(actionMap, actions::MenuBack, keyboard(key_codes::Escape));
    bindButton(actionMap, actions::ToggleDebug, keyboard(key_codes::F1));
    bindButton(actionMap, actions::DebugNextPage, keyboard(key_codes::Tab));
    bindButton(actionMap, actions::SelectDevRange, keyboard(key_codes::Digit1));
    bindButton(actionMap, actions::SelectTdm, keyboard(key_codes::Digit2));
    bindButton(actionMap, actions::SelectControl, keyboard(key_codes::Digit3));
    bindButton(actionMap, actions::SelectMainMenu, keyboard(key_codes::Escape));
    bindAxis(actionMap, actions::LookRight, mouseAxis(mouse_axes::X), 0.1F, 0.001F);
    bindAxis(actionMap, actions::LookUp, mouseAxis(mouse_axes::Y), -0.1F, 0.001F);

    bindAxis(actionMap, actions::MoveRight, gamepadAxis(gamepad_axes::LeftX), 1.0F, 0.08F);
    bindAxis(actionMap, actions::MoveLeft, gamepadAxis(gamepad_axes::LeftX), -1.0F, 0.08F);
    bindAxis(actionMap, actions::MoveForward, gamepadAxis(gamepad_axes::LeftY), -1.0F, 0.08F);
    bindAxis(actionMap, actions::MoveBackward, gamepadAxis(gamepad_axes::LeftY), 1.0F, 0.08F);
    bindButton(actionMap, actions::Jump, gamepadButton(gamepad_buttons::A));
    bindButton(actionMap, actions::DoubleJump, gamepadButton(gamepad_buttons::A));
    bindButton(actionMap, actions::Dash, gamepadButton(gamepad_buttons::B));
    bindButton(actionMap, actions::Slide, gamepadButton(gamepad_buttons::B));
    bindButton(actionMap, actions::Sprint, gamepadButton(gamepad_buttons::LeftStick));
    bindButton(actionMap, actions::TacticalSprint, gamepadButton(gamepad_buttons::LeftStick));
    bindButton(actionMap, actions::Mantle, gamepadButton(gamepad_buttons::A));
    bindAxis(actionMap, actions::Fire, gamepadAxis(gamepad_axes::RightTrigger), 1.0F, 0.15F);
    bindAxis(actionMap, actions::Ads, gamepadAxis(gamepad_axes::LeftTrigger), 1.0F, 0.15F);
    bindButton(actionMap, actions::Reload, gamepadButton(gamepad_buttons::X));
    bindButton(actionMap, actions::MenuUp, gamepadButton(gamepad_buttons::DPadUp));
    bindButton(actionMap, actions::MenuDown, gamepadButton(gamepad_buttons::DPadDown));
    bindButton(actionMap, actions::MenuConfirm, gamepadButton(gamepad_buttons::A));
    bindButton(actionMap, actions::MenuBack, gamepadButton(gamepad_buttons::B));
    bindButton(actionMap, actions::SelectMainMenu, gamepadButton(gamepad_buttons::B));
    bindButton(actionMap, actions::DebugNextPage, gamepadButton(gamepad_buttons::Start));
    bindAxis(actionMap, actions::LookRight, gamepadAxis(gamepad_axes::RightX), 2.4F, 0.06F);
    bindAxis(actionMap, actions::LookUp, gamepadAxis(gamepad_axes::RightY), -2.0F, 0.06F);

    return actionMap;
}

} // namespace nemisis::input
