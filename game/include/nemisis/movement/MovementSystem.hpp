#pragma once

#include "nemisis/movement/MovementTech.hpp"
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
    novacore::math::Vec3 wallRunNormal{};
    novacore::math::Vec3 wallRunTangent{};
    novacore::math::Vec3 mantleStartPosition{};
    novacore::math::Vec3 mantleTargetPosition{};
    novacore::math::Vec3 mantleNormal{};
    MovementTechState tech{};
    MovementMode mode = MovementMode::Grounded;
    bool hasDoubleJump = true;
    bool hasWallRunContact = false;
    float dashCooldownRemaining = 0.0F;
    float dashTimeRemaining = 0.0F;
    float slideCooldownRemaining = 0.0F;
    float slideTimeRemaining = 0.0F;
    float slideBufferRemaining = 0.0F;
    float wallRunTimeRemaining = 0.0F;
    float mantleTimeRemaining = 0.0F;
    float mantleProgressSeconds = 0.0F;
    float coyoteTimeRemaining = 0.0F;
    float jumpBufferRemaining = 0.0F;
    float groundedTimeSeconds = 0.0F;
    float airborneTimeSeconds = 0.0F;
    float lastHorizontalSpeed = 0.0F;
    float inputMagnitude = 0.0F;
    bool slideHeldConsumed = false;
};

struct WallRunContact final {
    bool available = false;
    novacore::math::Vec3 normal{};
    novacore::math::Vec3 tangent{};
};

struct MantleCandidate final {
    bool available = false;
    novacore::math::Vec3 targetPosition{};
    novacore::math::Vec3 normal{};
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

    [[nodiscard]] PlayerMovementState applyWallRunContact(
        PlayerMovementState state,
        const player::PlayerInputCommand& command,
        WallRunContact contact,
        float fixedDeltaSeconds) const;

    [[nodiscard]] PlayerMovementState applyMantleCandidate(
        PlayerMovementState state,
        const player::PlayerInputCommand& command,
        MantleCandidate candidate,
        float fixedDeltaSeconds) const;

private:
    MovementTuning tuning_;
};

} // namespace nemisis::movement
