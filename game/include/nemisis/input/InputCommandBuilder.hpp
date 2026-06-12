#pragma once

#include "nemisis/player/PlayerInputCommand.hpp"
#include "nemisis/settings/GameSettings.hpp"

#include "novacore/platform/InputAction.hpp"

#include <cstdint>

namespace nemisis::input {

[[nodiscard]] player::PlayerInputCommand buildPlayerInputCommand(
    const novacore::platform::InputActionMap& actions,
    std::uint64_t tick);

[[nodiscard]] player::PlayerInputCommand buildPlayerInputCommand(
    const novacore::platform::InputActionMap& actions,
    std::uint64_t tick,
    const settings::GameSettings& settings);

} // namespace nemisis::input
