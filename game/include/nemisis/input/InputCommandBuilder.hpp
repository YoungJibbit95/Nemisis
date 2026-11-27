#pragma once

#include "nemisis/player/PlayerInputCommand.hpp"
#include "nemisis/settings/GameSettings.hpp"

#include "novacore/platform/InputAction.hpp"

#include <cstdint>

namespace nemisis::input {

struct PlayerLookInput final {
    novacore::math::Vec2 mouseDelta{};
    novacore::math::Vec2 stickPerTick{};
};

[[nodiscard]] PlayerLookInput readPlayerLookInput(
    const novacore::platform::InputActionMap& actions,
    const settings::GameSettings& settings);

[[nodiscard]] player::PlayerInputCommand buildPlayerInputCommandWithLook(
    const novacore::platform::InputActionMap& actions,
    std::uint64_t tick,
    novacore::math::Vec2 look);

[[nodiscard]] player::PlayerInputCommand buildPlayerInputCommand(
    const novacore::platform::InputActionMap& actions,
    std::uint64_t tick);

[[nodiscard]] player::PlayerInputCommand buildPlayerInputCommand(
    const novacore::platform::InputActionMap& actions,
    std::uint64_t tick,
    const settings::GameSettings& settings);

} // namespace nemisis::input
