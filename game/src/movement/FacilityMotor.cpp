#include "nemisis/movement/FacilityMotor.hpp"

namespace nemisis::movement {
namespace {
[[nodiscard]] bool canReceiveGroundCollision(movement::MovementMode mode) {
    return mode == movement::MovementMode::Airborne ||
        mode == movement::MovementMode::Dashing ||
        mode == movement::MovementMode::WallRunning ||
        mode == movement::MovementMode::Mantling;
}

[[nodiscard]] bool shouldDisableGroundSnapForJumpArc(
    const movement::PlayerMovementState& state,
    const player::PlayerInputCommand& command) {
    return state.velocity.y > 0.05F ||
        command.jumpPressed ||
        command.doubleJumpPressed ||
        state.mode == movement::MovementMode::WallRunning;
}

[[nodiscard]] bool acceptsStableGroundContact(
    const movement::PlayerMovementState& state,
    const player::PlayerInputCommand& command,
    const dev::GreyboxCollisionResult& collision) {
    if (!collision.grounded) {
        return false;
    }
    if (state.mode == movement::MovementMode::Mantling) {
        return false;
    }
    if (state.mode == movement::MovementMode::WallRunning) {
        return false;
    }
    if ((command.jumpPressed || command.doubleJumpPressed) && state.velocity.y > -0.25F) {
        return false;
    }
    return state.velocity.y <= 0.08F;
}

}

dev::GreyboxCollisionResult stepFacilityMotor(PlayerMovementState& state, const MovementSystem& motor,
    const dev::GreyboxWorld& world, const novacore::physics::PhysicsWorld& collisionWorld,
    player::PlayerInputCommand movementCommand, novacore::math::Vec3 viewForward, float dt) {
    dev::GreyboxCollisionResult collisionSample{};
    const auto previousMovementPosition = state.position;

    if (state.crouched && !movementCommand.crouchHeld &&
        state.mode != movement::MovementMode::Sliding &&
        !dev::canStandFromCrouch(collisionWorld, previousMovementPosition, state.crouchFraction)) {
        movementCommand.crouchHeld = true;
    }
    state = motor.simulate(state, movementCommand,
        dt, viewForward);
    dev::GreyboxCollisionQuery collisionQuery{};
    collisionQuery.position = state.position;
    collisionQuery.previousPosition = previousMovementPosition;
    collisionQuery.height = dev::playerCapsuleHeight(state.crouchFraction);
    collisionQuery.mantleForward = viewForward;
    collisionQuery.mantleMaxDistance = motor.tuning().mantleMaxRange;
    collisionQuery.mantleMinHeight = collisionQuery.maxStepHeight + 0.02F;
    collisionQuery.mantleMaxHeight = motor.tuning().mantleMaxHeight;
    collisionQuery.wallProbeDistance = motor.tuning().wallRunProbeDistance;
    collisionQuery.useSweep = state.mode != movement::MovementMode::Mantling;
    collisionQuery.maxSweepIterations = 4;
    collisionQuery.enableGroundSnap =
        state.mode != movement::MovementMode::Mantling &&
        !shouldDisableGroundSnapForJumpArc(state, movementCommand);
    collisionQuery.enableStepUp =
        state.mode == movement::MovementMode::Grounded ||
        state.mode == movement::MovementMode::Sliding ||
        state.mode == movement::MovementMode::Dashing;
    collisionSample = dev::resolveGreyboxPlayerCollision(world, collisionWorld, collisionQuery);
    const bool mantleActivationWindow =
        state.mode == movement::MovementMode::Airborne &&
        state.velocity.y <= motor.tuning().jumpVelocity * 0.50F;
    if ((movementCommand.mantlePressed ||
            movementCommand.mantleHeld ||
            movementCommand.jumpPressed ||
            movementCommand.jumpHeld) &&
        !collisionSample.ceilingHit && collisionSample.mantleCandidate &&
        mantleActivationWindow) {
        state = motor.applyMantleCandidate(
            state,
            movementCommand,
            movement::MantleCandidate{
                true,
                collisionSample.mantleTargetPosition,
                collisionSample.mantleNormal,
            },
            dt);
        collisionQuery.position = state.position;
        collisionQuery.previousPosition = state.position;
        collisionQuery.useSweep = false;
        collisionQuery.enableGroundSnap = false;
        collisionQuery.enableStepUp = false;
        collisionSample = dev::resolveGreyboxPlayerCollision(world, collisionWorld, collisionQuery);
        collisionSample.position = state.position;
    }
    const bool activeMantleInterpolation =
        state.mode == movement::MovementMode::Mantling &&
        state.mantleTimeRemaining > 0.0F;
    const bool stableGroundContact = acceptsStableGroundContact(state, movementCommand, collisionSample);
    if (!activeMantleInterpolation) {
        state.position = collisionSample.position;
        state = motor.applyWallRunContact(
            state, movementCommand,
            {collisionSample.nearWallRunSurface && !stableGroundContact,
                collisionSample.wallNormal, collisionSample.wallTangent},
            dt);
    } else {
        collisionSample.position = state.position;
    }
    const bool wallRunning = state.mode == movement::MovementMode::WallRunning;
    if (!activeMantleInterpolation) {
        state.velocity = dev::clipGreyboxVelocity(state.velocity, collisionSample, wallRunning);
    }
    if (stableGroundContact && state.velocity.y < 0.0F && !activeMantleInterpolation) {
        state.velocity.y = 0.0F;
    }
    if (stableGroundContact && !wallRunning && !activeMantleInterpolation) {
        state.groundJumpAvailable = true;
        state.hasDoubleJump = true;
        state.coyoteTimeRemaining = motor.tuning().coyoteTimeSeconds;
        state.airborneTimeSeconds = 0.0F;
        if (canReceiveGroundCollision(state.mode) &&
            (state.mode != movement::MovementMode::Mantling ||
                state.mantleTimeRemaining <= 0.0F)) {
            state.mode = movement::MovementMode::Grounded;
            state.wallRunTimeRemaining = 0.0F;
            state.hasWallRunContact = false;
        }
    } else if (!wallRunning &&
        (state.mode == movement::MovementMode::Grounded ||
            state.mode == movement::MovementMode::Sliding ||
            state.mode == movement::MovementMode::Dashing)) {
        state.mode = movement::MovementMode::Airborne;
        state.groundedTimeSeconds = 0.0F;
    }
    return collisionSample;
}
}
