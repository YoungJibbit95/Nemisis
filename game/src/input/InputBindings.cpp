#include "nemisis/input/InputBindings.hpp"

#include <string>

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

void bindButton(novacore::platform::InputActionMap& actionMap, std::string_view action, novacore::platform::InputControl control) {
    actionMap.bind(novacore::platform::InputBinding{
        std::string(action),
        control,
        1.0F,
        0.5F,
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

    return actionMap;
}

} // namespace nemisis::input
