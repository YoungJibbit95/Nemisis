#include "nemisis/movement/MovementSystem.hpp"

#include <algorithm>
#include <cmath>

namespace nemisis::movement {

namespace {

struct HorizontalInput final {
    novacore::math::Vec3 direction{};
    float magnitude = 0.0F;
};

[[nodiscard]] float clamp01(float value) {
    return std::clamp(value, 0.0F, 1.0F);
}

[[nodiscard]] float smoothstep01(float value) {
    value = clamp01(value);
    return value * value * (3.0F - (2.0F * value));
}

[[nodiscard]] novacore::math::Vec3 lerp(novacore::math::Vec3 a, novacore::math::Vec3 b, float t) {
    return a + ((b - a) * t);
}

[[nodiscard]] float horizontalSpeed(novacore::math::Vec3 velocity) {
    return std::sqrt((velocity.x * velocity.x) + (velocity.z * velocity.z));
}

[[nodiscard]] float dotHorizontal(novacore::math::Vec3 lhs, novacore::math::Vec3 rhs) {
    return (lhs.x * rhs.x) + (lhs.z * rhs.z);
}

[[nodiscard]] novacore::math::Vec3 horizontalVelocity(novacore::math::Vec3 velocity) {
    return novacore::math::Vec3{velocity.x, 0.0F, velocity.z};
}

[[nodiscard]] novacore::math::Vec3 normalizedOrZero(novacore::math::Vec3 value) {
    const float lengthSquared = value.lengthSquared();
    if (lengthSquared <= 0.0001F) {
        return {};
    }

    const float inverseLength = 1.0F / std::sqrt(lengthSquared);
    return value * inverseLength;
}

[[nodiscard]] HorizontalInput horizontalInput(novacore::math::Vec2 move) {
    const float lengthSquared = move.lengthSquared();
    if (lengthSquared <= 0.0001F) {
        return {};
    }

    const float length = std::sqrt(lengthSquared);
    const float inverseLength = 1.0F / length;
    return HorizontalInput{
        novacore::math::Vec3{move.x * inverseLength, 0.0F, move.y * inverseLength},
        clamp01(length),
    };
}

[[nodiscard]] float consumeCooldown(float value, float fixedDeltaSeconds) {
    return std::max(0.0F, value - fixedDeltaSeconds);
}

[[nodiscard]] float approachFloat(float current, float target, float maxDelta) {
    if (current < target) {
        return std::min(target, current + std::max(0.0F, maxDelta));
    }
    return std::max(target, current - std::max(0.0F, maxDelta));
}

[[nodiscard]] novacore::math::Vec3 approachHorizontalVelocity(
    novacore::math::Vec3 velocity,
    novacore::math::Vec3 target,
    float acceleration,
    float fixedDeltaSeconds) {
    const auto delta = target - horizontalVelocity(velocity);
    const float deltaLength = std::sqrt(delta.lengthSquared());
    if (deltaLength <= 0.0001F) {
        velocity.x = target.x;
        velocity.z = target.z;
        return velocity;
    }

    const float maxStep = std::max(0.0F, acceleration) * fixedDeltaSeconds;
    const auto step = delta * (std::min(deltaLength, maxStep) / deltaLength);
    velocity.x += step.x;
    velocity.z += step.z;
    return velocity;
}

[[nodiscard]] novacore::math::Vec3 applyGroundFriction(
    novacore::math::Vec3 velocity,
    const MovementTuning& tuning,
    float fixedDeltaSeconds) {
    const float speed = horizontalSpeed(velocity);
    if (speed <= 0.001F) {
        velocity.x = 0.0F;
        velocity.z = 0.0F;
        return velocity;
    }

    const float control = std::max(speed, tuning.groundStopSpeed);
    const float drop = control * tuning.groundFriction * fixedDeltaSeconds;
    const float newSpeed = std::max(0.0F, speed - drop);
    const float scale = newSpeed / speed;
    velocity.x *= scale;
    velocity.z *= scale;
    return velocity;
}

[[nodiscard]] novacore::math::Vec3 clampHorizontalSpeed(
    novacore::math::Vec3 velocity,
    float maxSpeed) {
    const float speed = horizontalSpeed(velocity);
    if (speed <= maxSpeed || speed <= 0.0001F) {
        return velocity;
    }

    const float scale = maxSpeed / speed;
    velocity.x *= scale;
    velocity.z *= scale;
    return velocity;
}

[[nodiscard]] novacore::math::Vec3 accelerateAlongDirection(
    novacore::math::Vec3 velocity,
    novacore::math::Vec3 direction,
    float wishSpeed,
    float acceleration,
    float fixedDeltaSeconds) {
    const float speedAlongWish = dotHorizontal(velocity, direction);
    const float speedToAdd = wishSpeed - speedAlongWish;
    if (speedToAdd <= 0.0F) {
        return velocity;
    }
    const float accelerationStep = std::min(speedToAdd, std::max(0.0F, acceleration) * fixedDeltaSeconds);
    velocity.x += direction.x * accelerationStep;
    velocity.z += direction.z * accelerationStep;
    return velocity;
}

[[nodiscard]] novacore::math::Vec3 applyAirControl(
    novacore::math::Vec3 velocity,
    novacore::math::Vec3 direction,
    float inputMagnitude,
    const MovementTuning& tuning,
    float fixedDeltaSeconds) {
    const float speed = horizontalSpeed(velocity);
    if (speed <= 0.001F || direction.lengthSquared() <= 0.0001F) {
        return velocity;
    }

    const auto currentDirection = normalizedOrZero(horizontalVelocity(velocity));
    const float alignment = dotHorizontal(currentDirection, direction);
    if (alignment <= 0.0F) {
        return accelerateAlongDirection(
            velocity,
            direction,
            tuning.airMaxSpeed * inputMagnitude,
            tuning.airCounterAcceleration,
            fixedDeltaSeconds);
    }

    const float control = clamp01(tuning.airControl * inputMagnitude * inputMagnitude * fixedDeltaSeconds);
    const auto controlledDirection = normalizedOrZero(currentDirection + ((direction - currentDirection) * control));
    velocity.x = controlledDirection.x * speed;
    velocity.z = controlledDirection.z * speed;
    return velocity;
}

void beginSlide(
    PlayerMovementState& state,
    novacore::math::Vec3 direction,
    bool slideHeld,
    const MovementTuning& tuning) {
    const auto velocityDirection = normalizedOrZero(horizontalVelocity(state.velocity));
    if (direction.lengthSquared() <= 0.0001F) {
        direction = velocityDirection.lengthSquared() > 0.0001F
            ? velocityDirection
            : novacore::math::Vec3{0.0F, 0.0F, 1.0F};
    }
    if (velocityDirection.lengthSquared() > 0.0001F && dotHorizontal(direction, velocityDirection) < 0.35F) {
        direction = velocityDirection;
    }

    const float entrySpeed = std::max(horizontalSpeed(state.velocity), tuning.slideMinEntrySpeed);
    const float slideSpeed = std::min(tuning.slideMaxSpeed, entrySpeed + tuning.slideImpulse);
    state.velocity.x = direction.x * slideSpeed;
    state.velocity.z = direction.z * slideSpeed;
    state.slideCooldownRemaining = tuning.slideCooldownSeconds;
    state.slideTimeRemaining = tuning.slideMaxDurationSeconds;
    state.slideBufferRemaining = 0.0F;
    state.slideHeldConsumed = slideHeld;
    state.crouched = true;
    state.crouchFraction = 1.0F;
    state.landingRecoveryRemaining = 0.0F;
    state.mode = MovementMode::Sliding;
}

[[nodiscard]] bool isGroundedLike(MovementMode mode) {
    return mode == MovementMode::Grounded ||
        mode == MovementMode::Sliding ||
        mode == MovementMode::Dashing;
}

[[nodiscard]] novacore::math::Vec3 chooseWallRunTangent(
    novacore::math::Vec3 tangent,
    novacore::math::Vec3 velocity,
    novacore::math::Vec3 inputDirection) {
    tangent = normalizedOrZero(tangent);
    if (tangent.lengthSquared() <= 0.0001F) {
        return {};
    }

    const auto preferred = inputDirection.lengthSquared() > 0.0001F ? inputDirection : normalizedOrZero(horizontalVelocity(velocity));
    if ((preferred.x * tangent.x) + (preferred.z * tangent.z) < 0.0F) {
        tangent = tangent * -1.0F;
    }
    return tangent;
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
    fixedDeltaSeconds = std::clamp(fixedDeltaSeconds, 0.0F, 1.0F / 30.0F);
    beginMovementTechFrame(state.tech, fixedDeltaSeconds);
    state.landedThisTick = false;
    state.dashCooldownRemaining = consumeCooldown(state.dashCooldownRemaining, fixedDeltaSeconds);
    state.dashTimeRemaining = consumeCooldown(state.dashTimeRemaining, fixedDeltaSeconds);
    state.slideCooldownRemaining = consumeCooldown(state.slideCooldownRemaining, fixedDeltaSeconds);
    state.slideTimeRemaining = consumeCooldown(state.slideTimeRemaining, fixedDeltaSeconds);
    state.slideBufferRemaining = consumeCooldown(state.slideBufferRemaining, fixedDeltaSeconds);
    state.wallRunTimeRemaining = consumeCooldown(state.wallRunTimeRemaining, fixedDeltaSeconds);
    state.wallRunContactGraceRemaining = consumeCooldown(state.wallRunContactGraceRemaining, fixedDeltaSeconds);
    state.wallRunDetachCooldownRemaining = consumeCooldown(state.wallRunDetachCooldownRemaining, fixedDeltaSeconds);
    state.mantleTimeRemaining = consumeCooldown(state.mantleTimeRemaining, fixedDeltaSeconds);
    state.landingRecoveryRemaining = consumeCooldown(state.landingRecoveryRemaining, fixedDeltaSeconds);
    state.jumpBufferRemaining = command.jumpPressed
        ? tuning_.jumpBufferSeconds
        : consumeCooldown(state.jumpBufferRemaining, fixedDeltaSeconds);
    state.doubleJumpBufferRemaining = command.doubleJumpPressed
        ? tuning_.doubleJumpBufferSeconds
        : consumeCooldown(state.doubleJumpBufferRemaining, fixedDeltaSeconds);
    state.coyoteTimeRemaining = isGroundedLike(state.mode)
        ? tuning_.coyoteTimeSeconds
        : consumeCooldown(state.coyoteTimeRemaining, fixedDeltaSeconds);
    if (state.mode == MovementMode::Grounded && state.position.y <= 0.001F && state.velocity.y <= 0.001F) {
        state.groundJumpAvailable = true;
        state.hasDoubleJump = true;
    }
    if (!command.slideHeld) {
        state.slideHeldConsumed = false;
    }

    const bool wantsCrouch = command.crouchHeld || state.mode == MovementMode::Sliding;
    const float crouchDuration = wantsCrouch ? tuning_.crouchEnterSeconds : tuning_.crouchExitSeconds;
    const float crouchStep = crouchDuration > 0.0001F ? fixedDeltaSeconds / crouchDuration : 1.0F;
    state.crouchFraction = approachFloat(state.crouchFraction, wantsCrouch ? 1.0F : 0.0F, crouchStep);
    state.crouched = state.crouchFraction > 0.001F;

    const auto input = horizontalInput(command.move);
    const auto direction = input.direction;
    const bool hasMoveInput = input.magnitude > 0.001F;
    state.inputMagnitude = input.magnitude;
    const bool bufferedJump = state.jumpBufferRemaining > 0.0F;
    const float currentHorizontalSpeed = horizontalSpeed(state.velocity);
    const bool sprintSlideIntent = command.slideHeld &&
        !state.slideHeldConsumed &&
        (command.sprintHeld || command.tacticalSprintHeld || currentHorizontalSpeed >= tuning_.walkSpeed * 0.80F);
    if (command.slidePressed || sprintSlideIntent) {
        state.slideBufferRemaining = tuning_.slideBufferSeconds;
    }

    float targetSpeed = tuning_.walkSpeed;
    if (state.crouched) {
        targetSpeed = tuning_.crouchSpeed;
    } else if (command.tacticalSprintHeld) {
        targetSpeed = tuning_.tacticalSprintSpeed;
    } else if (command.sprintHeld) {
        targetSpeed = tuning_.sprintSpeed;
    }
    targetSpeed *= input.magnitude;

    if (state.mode == MovementMode::Dashing && state.dashTimeRemaining <= 0.0F) {
        state.mode = state.position.y <= 0.001F ? MovementMode::Grounded : MovementMode::Airborne;
    }
    if (state.mode == MovementMode::Sliding &&
        (state.slideTimeRemaining <= 0.0F || horizontalSpeed(state.velocity) <= tuning_.slideEndSpeed)) {
        state.mode = state.position.y <= 0.001F ? MovementMode::Grounded : MovementMode::Airborne;
        state.crouched = command.crouchHeld;
    }
    if (state.mode == MovementMode::WallRunning && state.wallRunTimeRemaining <= 0.0F) {
        stopGravityBoots(state.tech);
        state.wallRunDetachCooldownRemaining = std::max(
            state.wallRunDetachCooldownRemaining,
            tuning_.wallRunDetachCooldownSeconds);
        state.mode = MovementMode::Airborne;
        state.hasWallRunContact = false;
    }
    if (state.mode == MovementMode::Mantling) {
        const float duration = std::max(0.001F, tuning_.mantleDurationSeconds);
        state.mantleProgressSeconds = std::min(duration, state.mantleProgressSeconds + fixedDeltaSeconds);
        const float t = smoothstep01(state.mantleProgressSeconds / duration);
        const auto previousPosition = state.position;
        const float arc = std::sin(t * 3.1415926535F) * 0.16F;
        state.position = lerp(state.mantleStartPosition, state.mantleTargetPosition, t) +
            novacore::math::Vec3{0.0F, arc, 0.0F};
        state.velocity = fixedDeltaSeconds > 0.0F
            ? (state.position - previousPosition) * (1.0F / fixedDeltaSeconds)
            : novacore::math::Vec3{};
        if (state.mantleTimeRemaining <= 0.0F) {
            state.position = state.mantleTargetPosition;
            state.velocity = {};
            state.mode = MovementMode::Grounded;
            state.groundJumpAvailable = true;
            state.hasDoubleJump = true;
            state.coyoteTimeRemaining = tuning_.coyoteTimeSeconds;
            state.doubleJumpBufferRemaining = 0.0F;
            state.groundedTimeSeconds = std::max(state.groundedTimeSeconds, fixedDeltaSeconds);
            state.airborneTimeSeconds = 0.0F;
        } else {
            state.lastHorizontalSpeed = 0.0F;
            return state;
        }
    }

    if (state.mode == MovementMode::Grounded) {
        if (hasMoveInput) {
            const auto targetVelocity = direction * targetSpeed;
            const auto currentDirection = normalizedOrZero(horizontalVelocity(state.velocity));
            const float alignment = currentDirection.lengthSquared() > 0.0001F
                ? dotHorizontal(currentDirection, direction)
                : 1.0F;
            float acceleration = alignment < 0.35F
                ? tuning_.groundTurnAcceleration
                : tuning_.groundAcceleration;
            if (currentHorizontalSpeed > targetSpeed + 0.01F) {
                acceleration = std::max(acceleration, tuning_.groundDeceleration);
            }
            if (state.landingRecoveryRemaining > 0.0F) {
                acceleration *= tuning_.hardLandingControlScale;
            }
            state.velocity = approachHorizontalVelocity(
                state.velocity,
                targetVelocity,
                acceleration,
                fixedDeltaSeconds);
        } else {
            state.velocity = applyGroundFriction(state.velocity, tuning_, fixedDeltaSeconds);
        }
    } else if (state.mode == MovementMode::Airborne) {
        if (hasMoveInput) {
            const float wishSpeed = std::min(targetSpeed, tuning_.airMaxSpeed);
            state.velocity = accelerateAlongDirection(
                state.velocity,
                direction,
                wishSpeed,
                tuning_.airAcceleration,
                fixedDeltaSeconds);
            state.velocity = applyAirControl(
                state.velocity,
                direction,
                input.magnitude,
                tuning_,
                fixedDeltaSeconds);
        }
        const float drag = std::clamp(1.0F - (tuning_.airDrag * fixedDeltaSeconds), 0.0F, 1.0F);
        state.velocity.x *= drag;
        state.velocity.z *= drag;
    } else if (state.mode == MovementMode::Sliding) {
        if (hasMoveInput) {
            const auto desired = direction * std::max(horizontalSpeed(state.velocity), tuning_.slideEndSpeed);
            state.velocity = approachHorizontalVelocity(
                state.velocity,
                desired,
                tuning_.slideSteeringAcceleration,
                fixedDeltaSeconds);
        }
        const float speed = horizontalSpeed(state.velocity);
        if (speed > 0.001F) {
            const float newSpeed = std::max(0.0F, speed - (tuning_.slideFriction * fixedDeltaSeconds));
            const float scale = newSpeed / speed;
            state.velocity.x *= scale;
            state.velocity.z *= scale;
        }
    } else if (state.mode == MovementMode::Dashing && hasMoveInput) {
        const auto desired = direction * std::max(horizontalSpeed(state.velocity), tuning_.sprintSpeed);
        state.velocity = approachHorizontalVelocity(
            state.velocity,
            desired,
            tuning_.dashSteeringAcceleration,
            fixedDeltaSeconds);
    } else if (state.mode == MovementMode::WallRunning) {
        const auto tangent = chooseWallRunTangent(state.wallRunTangent, state.velocity, direction);
        if (tangent.lengthSquared() > 0.0001F) {
            const float tangentSpeed = std::abs(dotHorizontal(state.velocity, tangent));
            const float targetWallSpeed = std::clamp(
                std::max(tangentSpeed, tuning_.wallRunSpeed),
                tuning_.wallRunSpeed,
                tuning_.wallRunMaxSpeed);
            state.velocity = approachHorizontalVelocity(
                state.velocity,
                tangent * targetWallSpeed,
                tuning_.wallRunAcceleration,
                fixedDeltaSeconds);
        }
        state.velocity.y = approachFloat(
            state.velocity.y,
            tuning_.wallRunVerticalTarget,
            tuning_.wallRunVerticalAcceleration * fixedDeltaSeconds);
    }

    if (state.slideBufferRemaining > 0.0F &&
        state.mode == MovementMode::Grounded &&
        state.slideCooldownRemaining <= 0.0F &&
        currentHorizontalSpeed >= tuning_.slideMinEntrySpeed) {
        const auto slideDirection = hasMoveInput
            ? direction
            : normalizedOrZero(horizontalVelocity(state.velocity));
        beginSlide(state, slideDirection, command.slideHeld, tuning_);
    }

    if (command.dashPressed &&
        state.dashCooldownRemaining <= 0.0F &&
        state.mode != MovementMode::Sliding &&
        state.mode != MovementMode::WallRunning &&
        state.mode != MovementMode::Mantling) {
        const auto dashDirection = hasMoveInput
            ? direction
            : normalizedOrZero(horizontalVelocity(state.velocity));
        const auto impulseDirection = dashDirection.lengthSquared() > 0.0001F
            ? dashDirection
            : novacore::math::Vec3{0.0F, 0.0F, 1.0F};
        state.velocity = state.velocity + (impulseDirection * tuning_.dashImpulse);
        state.dashCooldownRemaining = tuning_.dashCooldownSeconds;
        state.dashTimeRemaining = tuning_.dashDurationSeconds;
        state.mode = MovementMode::Dashing;
    }

    const bool canUseGroundJump = state.groundJumpAvailable &&
        (isGroundedLike(state.mode) || state.coyoteTimeRemaining > 0.0F);
    const bool canUseAirJump = state.mode == MovementMode::Airborne &&
        state.hasDoubleJump &&
        state.airborneTimeSeconds >= tuning_.doubleJumpMinAirborneSeconds &&
        state.coyoteTimeRemaining <= 0.0F;
    if (command.jumpPressed &&
        state.mode == MovementMode::Airborne &&
        !canUseGroundJump &&
        state.hasDoubleJump) {
        state.doubleJumpBufferRemaining = std::max(
            state.doubleJumpBufferRemaining,
            tuning_.doubleJumpBufferSeconds);
    }

    const bool wantsAirJump =
        state.doubleJumpBufferRemaining > 0.0F ||
        command.doubleJumpPressed ||
        (command.jumpPressed && canUseAirJump);
    if (command.jumpPressed && state.mode == MovementMode::WallRunning) {
        const auto tangent = chooseWallRunTangent(state.wallRunTangent, state.velocity, direction);
        const auto normal = normalizedOrZero(state.wallRunNormal);
        state.velocity =
            (tangent * (tuning_.wallRunSpeed * 0.74F)) +
            (normal * tuning_.wallJumpImpulse) +
            novacore::math::Vec3{0.0F, tuning_.doubleJumpImpulse, 0.0F};
        state.wallRunTimeRemaining = 0.0F;
        state.wallRunContactGraceRemaining = 0.0F;
        state.wallRunDetachCooldownRemaining = tuning_.wallRunDetachCooldownSeconds;
        state.jumpBufferRemaining = 0.0F;
        state.doubleJumpBufferRemaining = 0.0F;
        state.coyoteTimeRemaining = 0.0F;
        state.groundJumpAvailable = false;
        state.hasDoubleJump = true;
        state.hasWallRunContact = false;
        triggerWallJumpDetach(state.tech);
        state.mode = MovementMode::Airborne;
    } else if (wantsAirJump && canUseAirJump) {
        triggerDoubleJumpPlatform(state.tech, state.position);
        state.velocity.y = tuning_.doubleJumpImpulse;
        state.hasDoubleJump = false;
        state.groundJumpAvailable = false;
        state.jumpBufferRemaining = 0.0F;
        state.doubleJumpBufferRemaining = 0.0F;
    } else if (bufferedJump && canUseGroundJump) {
        state.velocity.y = tuning_.jumpVelocity;
        if (state.mode == MovementMode::Sliding) {
            const auto slideDirection = normalizedOrZero(horizontalVelocity(state.velocity));
            const float boostedSpeed = std::min(
                tuning_.slideMaxSpeed,
                horizontalSpeed(state.velocity) + tuning_.slideJumpBoost);
            state.velocity.x = slideDirection.x * boostedSpeed;
            state.velocity.z = slideDirection.z * boostedSpeed;
            state.slideTimeRemaining = 0.0F;
        }
        state.jumpBufferRemaining = 0.0F;
        state.doubleJumpBufferRemaining = 0.0F;
        state.coyoteTimeRemaining = 0.0F;
        state.groundJumpAvailable = false;
        state.hasDoubleJump = true;
        state.mode = MovementMode::Airborne;
    } else if ((command.mantlePressed || command.mantleHeld) && state.mode == MovementMode::Airborne) {
        triggerMantleReach(state.tech);
    }

    if (state.mode != MovementMode::Grounded &&
        state.mode != MovementMode::Mantling &&
        state.mode != MovementMode::WallRunning) {
        state.velocity.y += tuning_.gravity * fixedDeltaSeconds;
    }

    const bool wasAirborneBeforeIntegration =
        state.mode == MovementMode::Airborne || state.mode == MovementMode::WallRunning;
    const float impactSpeed = std::max(0.0F, -state.velocity.y);
    state.position = state.position + (state.velocity * fixedDeltaSeconds);

    if (state.position.y <= 0.0F) {
        const bool consumeBufferedLandingJump =
            state.jumpBufferRemaining > 0.0F &&
            state.mode != MovementMode::Mantling &&
            state.mode != MovementMode::WallRunning;
        state.position.y = 0.0F;
        if (consumeBufferedLandingJump) {
            state.velocity.y = tuning_.jumpVelocity;
            state.groundJumpAvailable = false;
            state.hasDoubleJump = true;
            state.coyoteTimeRemaining = 0.0F;
            state.jumpBufferRemaining = 0.0F;
            state.doubleJumpBufferRemaining = 0.0F;
            state.mantleTimeRemaining = 0.0F;
            state.wallRunTimeRemaining = 0.0F;
            state.wallRunContactGraceRemaining = 0.0F;
            stopGravityBoots(state.tech);
            state.mode = MovementMode::Airborne;
            state.groundedTimeSeconds = 0.0F;
            state.airborneTimeSeconds = fixedDeltaSeconds;
        } else {
            state.landedThisTick = wasAirborneBeforeIntegration;
            if (state.landedThisTick) {
                state.lastLandingSpeed = impactSpeed;
                if (impactSpeed >= tuning_.hardLandingSpeed) {
                    const float severity = clamp01(
                        (impactSpeed - tuning_.hardLandingSpeed) /
                        std::max(0.01F, tuning_.hardLandingSpeed));
                    state.landingRecoveryRemaining = tuning_.hardLandingRecoverySeconds * (0.55F + (0.45F * severity));
                }
            }
            state.velocity.y = 0.0F;
            state.groundJumpAvailable = true;
            state.hasDoubleJump = true;
            state.coyoteTimeRemaining = tuning_.coyoteTimeSeconds;
            state.jumpBufferRemaining = 0.0F;
            state.doubleJumpBufferRemaining = 0.0F;
            state.mantleTimeRemaining = 0.0F;
            state.wallRunContactGraceRemaining = 0.0F;
            stopGravityBoots(state.tech);
            state.hasWallRunContact = false;
            if (state.mode == MovementMode::Airborne ||
                state.mode == MovementMode::Dashing ||
                state.mode == MovementMode::WallRunning) {
                state.mode = MovementMode::Grounded;
            }
            if (state.slideBufferRemaining > 0.0F &&
                state.slideCooldownRemaining <= 0.0F &&
                horizontalSpeed(state.velocity) >= tuning_.slideMinEntrySpeed) {
                beginSlide(state, direction, command.slideHeld, tuning_);
            }
            state.groundedTimeSeconds += fixedDeltaSeconds;
            state.airborneTimeSeconds = 0.0F;
        }
    } else {
        state.airborneTimeSeconds += fixedDeltaSeconds;
        state.groundedTimeSeconds = 0.0F;
    }

    state.velocity = clampHorizontalSpeed(state.velocity, tuning_.maxValidatedHorizontalSpeed);
    state.lastHorizontalSpeed = horizontalSpeed(state.velocity);
    return state;
}

PlayerMovementState MovementSystem::applyWallRunContact(
    PlayerMovementState state,
    const player::PlayerInputCommand& command,
    WallRunContact contact,
    float fixedDeltaSeconds) const {
    fixedDeltaSeconds = std::clamp(fixedDeltaSeconds, 0.0F, 1.0F / 30.0F);
    const auto input = horizontalInput(command.move);
    const bool contactGraceActive =
        state.mode == MovementMode::WallRunning &&
        state.wallRunContactGraceRemaining > 0.0F &&
        state.wallRunNormal.lengthSquared() > 0.0001F &&
        state.wallRunTangent.lengthSquared() > 0.0001F;
    const bool usableContact = contact.available || contactGraceActive;
    const auto requestedNormal = normalizedOrZero(contact.available ? contact.normal : state.wallRunNormal);
    const float movementSpeed = horizontalSpeed(state.velocity);
    const float outwardSpeed = dotHorizontal(state.velocity, requestedNormal);
    const bool enteringWallRun = state.mode != MovementMode::WallRunning;
    const bool hasWallRunInput = input.magnitude > 0.12F || movementSpeed >= tuning_.wallRunEntryMinSpeed;
    const bool entryVelocityAllowed = !enteringWallRun ||
        (movementSpeed >= tuning_.wallRunEntryMinSpeed && outwardSpeed <= tuning_.wallRunMaxOutwardSpeed);
    const bool wantsWallRun = usableContact &&
        state.wallRunDetachCooldownRemaining <= 0.0F &&
        hasWallRunInput &&
        entryVelocityAllowed &&
        state.position.y >= tuning_.wallRunMinHeight &&
        state.mode != MovementMode::Grounded &&
        state.mode != MovementMode::Sliding &&
        state.mode != MovementMode::Mantling;

    if (!wantsWallRun) {
        if (state.mode == MovementMode::WallRunning) {
            stopGravityBoots(state.tech);
            state.wallRunDetachCooldownRemaining = std::max(
                state.wallRunDetachCooldownRemaining,
                tuning_.wallRunDetachCooldownSeconds * 0.50F);
            state.mode = MovementMode::Airborne;
        }
        state.hasWallRunContact = false;
        return state;
    }

    const auto contactNormal = contact.available ? contact.normal : state.wallRunNormal;
    const auto contactTangent = contact.available ? contact.tangent : state.wallRunTangent;
    const auto tangent = chooseWallRunTangent(contactTangent, state.velocity, input.direction);
    if (tangent.lengthSquared() <= 0.0001F) {
        return state;
    }

    state.mode = MovementMode::WallRunning;
    state.hasWallRunContact = true;
    state.wallRunNormal = normalizedOrZero(contactNormal);
    state.wallRunTangent = tangent;
    if (contact.available) {
        state.wallRunContactGraceRemaining = tuning_.wallRunContactGraceSeconds;
    }
    if (enteringWallRun) {
        triggerWallRunGravityTech(state.tech, state.wallRunNormal);
    } else {
        keepGravityBootsActive(state.tech, state.wallRunNormal);
    }
    if (state.wallRunTimeRemaining <= 0.0F) {
        state.wallRunTimeRemaining = tuning_.wallRunMaxDurationSeconds;
    }

    const float tangentSpeed = std::abs(dotHorizontal(state.velocity, tangent));
    const float targetSpeed = std::clamp(
        std::max(tangentSpeed, tuning_.wallRunSpeed),
        tuning_.wallRunSpeed,
        tuning_.wallRunMaxSpeed);
    state.velocity = approachHorizontalVelocity(
        state.velocity,
        tangent * targetSpeed,
        tuning_.wallRunAcceleration,
        fixedDeltaSeconds);
    state.velocity.y = approachFloat(
        state.velocity.y,
        tuning_.wallRunVerticalTarget,
        tuning_.wallRunVerticalAcceleration * fixedDeltaSeconds);
    state.hasDoubleJump = true;
    state.groundJumpAvailable = false;
    state.lastHorizontalSpeed = horizontalSpeed(state.velocity);
    return state;
}

PlayerMovementState MovementSystem::applyMantleCandidate(
    PlayerMovementState state,
    const player::PlayerInputCommand& command,
    MantleCandidate candidate,
    float fixedDeltaSeconds) const {
    (void)fixedDeltaSeconds;
    if (!(command.mantlePressed || command.mantleHeld || command.jumpPressed || command.jumpHeld) || !candidate.available) {
        return state;
    }

    state.mantleStartPosition = state.position;
    state.mantleTargetPosition = candidate.targetPosition;
    state.mantleNormal = normalizedOrZero(candidate.normal);
    state.velocity = {};
    state.mode = MovementMode::Mantling;
    state.hasDoubleJump = true;
    state.groundJumpAvailable = true;
    state.hasWallRunContact = false;
    state.wallRunTimeRemaining = 0.0F;
    state.mantleTimeRemaining = tuning_.mantleDurationSeconds;
    state.mantleProgressSeconds = 0.0F;
    state.coyoteTimeRemaining = tuning_.coyoteTimeSeconds;
    state.jumpBufferRemaining = 0.0F;
    state.doubleJumpBufferRemaining = 0.0F;
    state.airborneTimeSeconds = 0.0F;
    state.groundedTimeSeconds = 0.0F;
    state.lastHorizontalSpeed = 0.0F;
    stopGravityBoots(state.tech);
    triggerMantleClimb(state.tech, candidate.targetPosition, candidate.normal);
    return state;
}

} // namespace nemisis::movement
