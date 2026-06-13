#pragma once

#include "nemisis/player/PlayerInputCommand.hpp"

#include "novacore/platform/InputAction.hpp"

namespace nemisis::input {

struct GameplayInputBufferState final {
    bool jumpPressed = false;
    bool doubleJumpPressed = false;
    bool slidePressed = false;
    bool dashPressed = false;
    bool mantlePressed = false;
    bool reloadPressed = false;
    bool pickupWeaponPressed = false;
    bool switchWeaponPrimaryPressed = false;
    bool switchWeaponSmgPressed = false;
    bool switchWeaponSidearmPressed = false;
};

class GameplayInputBuffer final {
public:
    void clear();
    void captureFrameEdges(const novacore::platform::InputActionMap& actions);
    void consumeInto(player::PlayerInputCommand& command);

    [[nodiscard]] const GameplayInputBufferState& state() const;
    [[nodiscard]] bool hasPendingEdges() const;

private:
    GameplayInputBufferState pending_{};
};

} // namespace nemisis::input
