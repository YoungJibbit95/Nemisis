#pragma once

#include "nemisis/player/PlayerComponents.hpp"
#include "nemisis/player/PlayerInputCommand.hpp"

#include "novacore/math/Types.hpp"

namespace nemisis::player {

struct PlayerViewVectors final {
    novacore::math::Vec3 forward{0.0F, 0.0F, 1.0F};
    novacore::math::Vec3 horizontalForward{0.0F, 0.0F, 1.0F};
    novacore::math::Vec3 horizontalRight{1.0F, 0.0F, 0.0F};
};

void applyLook(PlayerViewComponent& view, novacore::math::Vec2 lookDegrees);

[[nodiscard]] PlayerViewVectors viewVectors(const PlayerViewComponent& view);

[[nodiscard]] PlayerInputCommand commandRelativeToView(
    PlayerInputCommand command,
    const PlayerViewComponent& view);

} // namespace nemisis::player
