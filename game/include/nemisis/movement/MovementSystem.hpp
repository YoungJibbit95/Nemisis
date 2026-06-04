#pragma once

#include "nemisis/movement/MovementTuning.hpp"
#include "nemisis/player/PlayerInputCommand.hpp"

#include "novacore/math/Types.hpp"

namespace nemisis::movement {

enum class MovementMode {
    Grounded,
    Airborne,
    Sliding,
    Dashing,
    WallRunning,
    Mantling
};

struct PlayerMovementState final {
    novacore::math::Vec3 position{};
    novacore::math::Vec3 velocity{};
    MovementMode mode = MovementMode::Grounded;
    bool hasDoubleJump = true;
    float dashCooldownRemaining = 0.0F;
    float slideCooldownRemaining = 0.0F;
    float wallRunTimeRemaining = 0.0F;
};

class MovementSystem final {
public:
    explicit MovementSystem(MovementTuning tuning = {});

    void setTuning(MovementTuning tuning);
    [[nodiscard]] const MovementTuning& tuning() const;

    [[nodiscard]] PlayerMovementState simulate(
        PlayerMovementState state,
        const player::PlayerInputCommand& command,
        float fixedDeltaSeconds) const;

private:
    MovementTuning tuning_;
};

} // namespace nemisis::movement

