#pragma once

#include "nemisis/player/PlayerInputCommand.hpp"

#include "novacore/platform/InputAction.hpp"

#include <cstdint>

namespace nemisis::input {

[[nodiscard]] player::PlayerInputCommand buildPlayerInputCommand(
    const novacore::platform::InputActionMap& actions,
    std::uint64_t tick);

} // namespace nemisis::input
