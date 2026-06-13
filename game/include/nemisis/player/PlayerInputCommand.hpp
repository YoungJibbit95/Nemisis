#pragma once

#include "novacore/math/Types.hpp"
#include "novacore/platform/Input.hpp"

#include <cstdint>

namespace nemisis::player {

struct PlayerInputCommand final {
    std::uint64_t tick = 0;
    novacore::math::Vec2 move{};
    novacore::math::Vec2 look{};

    bool jumpPressed = false;
    bool jumpHeld = false;
    bool doubleJumpPressed = false;
    bool crouchHeld = false;
    bool slidePressed = false;
    bool slideHeld = false;
    bool sprintHeld = false;
    bool tacticalSprintHeld = false;
    bool dashPressed = false;
    bool mantlePressed = false;
    bool mantleHeld = false;

    bool fireHeld = false;
    bool adsHeld = false;
    bool reloadPressed = false;
    bool reloadHeld = false;
    bool pickupWeaponPressed = false;
    bool switchWeaponPrimaryPressed = false;
    bool switchWeaponSmgPressed = false;
    bool switchWeaponSidearmPressed = false;

    novacore::platform::InputDeviceKind device = novacore::platform::InputDeviceKind::KeyboardMouse;
};

} // namespace nemisis::player
