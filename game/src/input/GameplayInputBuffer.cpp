#include "nemisis/input/GameplayInputBuffer.hpp"

#include "nemisis/input/InputBindings.hpp"

namespace nemisis::input {

namespace {

[[nodiscard]] bool pressed(
    const novacore::platform::InputActionMap& actions,
    std::string_view action) {
    return actions.stateOrDefault(action).pressed;
}

void mergeEdge(bool& pending, bool current) {
    pending = pending || current;
}

void consumeEdge(bool& pending, bool& destination) {
    if (!pending) {
        return;
    }

    destination = true;
    pending = false;
}

} // namespace

void GameplayInputBuffer::clear() {
    pending_ = {};
}

void GameplayInputBuffer::captureFrameEdges(const novacore::platform::InputActionMap& actions) {
    mergeEdge(pending_.jumpPressed, pressed(actions, actions::Jump));
    mergeEdge(pending_.doubleJumpPressed, pressed(actions, actions::DoubleJump));
    mergeEdge(pending_.slidePressed, pressed(actions, actions::Slide));
    mergeEdge(pending_.dashPressed, pressed(actions, actions::Dash));
    mergeEdge(pending_.mantlePressed, pressed(actions, actions::Mantle));
    mergeEdge(pending_.reloadPressed, pressed(actions, actions::Reload));
    mergeEdge(pending_.pickupWeaponPressed, pressed(actions, actions::PickupWeapon));
    mergeEdge(pending_.switchWeaponPrimaryPressed, pressed(actions, actions::SwitchWeaponPrimary));
    mergeEdge(pending_.switchWeaponSmgPressed, pressed(actions, actions::SwitchWeaponSmg));
    mergeEdge(pending_.switchWeaponSidearmPressed, pressed(actions, actions::SwitchWeaponSidearm));
}

void GameplayInputBuffer::consumeInto(player::PlayerInputCommand& command) {
    consumeEdge(pending_.jumpPressed, command.jumpPressed);
    consumeEdge(pending_.doubleJumpPressed, command.doubleJumpPressed);
    consumeEdge(pending_.slidePressed, command.slidePressed);
    consumeEdge(pending_.dashPressed, command.dashPressed);
    consumeEdge(pending_.mantlePressed, command.mantlePressed);
    consumeEdge(pending_.reloadPressed, command.reloadPressed);
    consumeEdge(pending_.pickupWeaponPressed, command.pickupWeaponPressed);
    consumeEdge(pending_.switchWeaponPrimaryPressed, command.switchWeaponPrimaryPressed);
    consumeEdge(pending_.switchWeaponSmgPressed, command.switchWeaponSmgPressed);
    consumeEdge(pending_.switchWeaponSidearmPressed, command.switchWeaponSidearmPressed);
}

const GameplayInputBufferState& GameplayInputBuffer::state() const {
    return pending_;
}

bool GameplayInputBuffer::hasPendingEdges() const {
    return pending_.jumpPressed ||
        pending_.doubleJumpPressed ||
        pending_.slidePressed ||
        pending_.dashPressed ||
        pending_.mantlePressed ||
        pending_.reloadPressed ||
        pending_.pickupWeaponPressed ||
        pending_.switchWeaponPrimaryPressed ||
        pending_.switchWeaponSmgPressed ||
        pending_.switchWeaponSidearmPressed;
}

} // namespace nemisis::input
