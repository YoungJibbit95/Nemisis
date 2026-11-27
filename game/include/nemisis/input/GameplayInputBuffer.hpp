#pragma once

#include "nemisis/player/PlayerInputCommand.hpp"
#include "nemisis/settings/GameSettings.hpp"

#include "novacore/platform/InputAction.hpp"

namespace nemisis::input {

struct GameplayInputBufferState final {
    novacore::math::Vec2 mouseLookDelta{};
    bool firePressed = false;
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
    void captureFrame(const novacore::platform::InputActionMap& actions, const settings::GameSettings& settings = {});
    [[nodiscard]] player::PlayerInputCommand consumeTick(
        const novacore::platform::InputActionMap& actions, std::uint64_t tick);

    [[nodiscard]] const GameplayInputBufferState& state() const;
    [[nodiscard]] bool hasPendingEdges() const;

private:
    GameplayInputBufferState pending_{};
    novacore::math::Vec2 stickLookPerTick_{};
};

} // namespace nemisis::input
