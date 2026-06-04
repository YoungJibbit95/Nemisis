#include "nemisis/movement/MovementSystem.hpp"

#include <algorithm>
#include <cmath>

namespace nemisis::movement {

namespace {

novacore::math::Vec3 horizontalDirection(novacore::math::Vec2 move) {
    const float lengthSquared = move.lengthSquared();
    if (lengthSquared <= 0.0001F) {
        return {};
    }

    const float inverseLength = 1.0F / std::sqrt(lengthSquared);
    return novacore::math::Vec3{move.x * inverseLength, 0.0F, move.y * inverseLength};
}

float consumeCooldown(float value, float fixedDeltaSeconds) {
    return std::max(0.0F, value - fixedDeltaSeconds);
}

} // namespace

MovementSystem::MovementSystem(MovementTuning tuning)
    : tuning_(tuning) {
}

void MovementSystem::setTuning(MovementTuning tuning) {
    tuning_ = tuning;
}

const MovementTuning& MovementSystem::tuning() const {
    return tuning_;
}

PlayerMovementState MovementSystem::simulate(
    PlayerMovementState state,
    const player::PlayerInputCommand& command,
    float fixedDeltaSeconds) const {
    state.dashCooldownRemaining = consumeCooldown(state.dashCooldownRemaining, fixedDeltaSeconds);
    state.slideCooldownRemaining = consumeCooldown(state.slideCooldownRemaining, fixedDeltaSeconds);
    state.wallRunTimeRemaining = consumeCooldown(state.wallRunTimeRemaining, fixedDeltaSeconds);

    const auto direction = horizontalDirection(command.move);
    const bool hasMoveInput = direction.lengthSquared() > 0.0001F;

    float targetSpeed = tuning_.walkSpeed;
    if (command.crouchHeld) {
        targetSpeed = tuning_.crouchSpeed;
    } else if (command.tacticalSprintHeld) {
        targetSpeed = tuning_.tacticalSprintSpeed;
    } else if (command.sprintHeld) {
        targetSpeed = tuning_.sprintSpeed;
    }

    if (hasMoveInput && state.mode == MovementMode::Grounded) {
        state.velocity.x = direction.x * targetSpeed;
        state.velocity.z = direction.z * targetSpeed;
    }

    if (command.slidePressed && state.mode == MovementMode::Grounded && state.slideCooldownRemaining <= 0.0F) {
        state.velocity = state.velocity + (direction * tuning_.slideImpulse);
        state.slideCooldownRemaining = tuning_.slideCooldownSeconds;
        state.mode = MovementMode::Sliding;
    }

    if (command.dashPressed && state.dashCooldownRemaining <= 0.0F) {
        state.velocity = state.velocity + (direction * tuning_.dashImpulse);
        state.dashCooldownRemaining = tuning_.dashCooldownSeconds;
        state.mode = MovementMode::Dashing;
    }

    if (command.jumpPressed && state.mode == MovementMode::Grounded) {
        state.velocity.y = tuning_.jumpVelocity;
        state.hasDoubleJump = true;
        state.mode = MovementMode::Airborne;
    } else if ((command.doubleJumpPressed || command.jumpPressed) && state.mode == MovementMode::Airborne && state.hasDoubleJump) {
        state.velocity.y = tuning_.doubleJumpImpulse;
        state.hasDoubleJump = false;
    }

    if (state.mode != MovementMode::Grounded && state.mode != MovementMode::Mantling) {
        state.velocity.y += tuning_.gravity * fixedDeltaSeconds;
    }

    state.position = state.position + (state.velocity * fixedDeltaSeconds);

    if (state.position.y <= 0.0F) {
        state.position.y = 0.0F;
        state.velocity.y = 0.0F;
        state.hasDoubleJump = true;
        if (state.mode == MovementMode::Airborne || state.mode == MovementMode::Dashing) {
            state.mode = MovementMode::Grounded;
        }
    }

    return state;
}

} // namespace nemisis::movement

