#pragma once

namespace nemisis::movement {

struct MovementTuning final {
    float walkSpeed = 5.2F;
    float sprintSpeed = 7.4F;
    float tacticalSprintSpeed = 8.6F;
    float crouchSpeed = 3.1F;
    float groundAcceleration = 46.0F;
    float groundTurnAcceleration = 68.0F;
    float groundDeceleration = 38.0F;
    float groundFriction = 9.5F;
    float groundStopSpeed = 1.25F;
    float jumpVelocity = 7.35F;
    float coyoteTimeSeconds = 0.10F;
    float jumpBufferSeconds = 0.12F;
    float doubleJumpImpulse = 7.15F;
    float doubleJumpMinAirborneSeconds = 0.0F;
    float doubleJumpBufferSeconds = 0.16F;
    float gravity = -22.0F;
    float airAcceleration = 17.0F;
    float airCounterAcceleration = 26.0F;
    float airControl = 0.42F;
    float airMaxSpeed = 8.2F;
    float airDrag = 0.08F;

    float slideImpulse = 2.6F;
    float slideMinEntrySpeed = 4.2F;
    float slideMaxSpeed = 11.5F;
    float slideMaxDurationSeconds = 0.9F;
    float slideFriction = 7.0F;
    float slideSteeringAcceleration = 8.0F;
    float slideEndSpeed = 4.0F;
    float slideJumpBoost = 2.5F;
    float slideCooldownSeconds = 0.25F;
    float slideBufferSeconds = 0.16F;

    float dashImpulse = 9.0F;
    float dashDurationSeconds = 0.18F;
    float dashSteeringAcceleration = 3.0F;
    float dashCooldownSeconds = 0.8F;

    float wallRunSpeed = 8.2F;
    float wallRunMaxSpeed = 10.6F;
    float wallRunAcceleration = 24.0F;
    float wallRunEntryMinSpeed = 4.0F;
    float wallRunMaxOutwardSpeed = 1.35F;
    float wallRunVerticalTarget = -0.35F;
    float wallRunVerticalAcceleration = 11.0F;
    float wallRunMaxDurationSeconds = 1.35F;
    float wallJumpImpulse = 6.2F;
    float wallRunMinHeight = 0.35F;
    float wallRunProbeDistance = 0.82F;
    float wallRunContactGraceSeconds = 0.12F;
    float wallRunDetachCooldownSeconds = 0.16F;

    float mantleMaxRange = 1.25F;
    float mantleMaxHeight = 1.4F;
    float mantleDurationSeconds = 0.22F;

    float crouchEnterSeconds = 0.08F;
    float crouchExitSeconds = 0.11F;
    float hardLandingSpeed = 8.5F;
    float hardLandingRecoverySeconds = 0.16F;
    float hardLandingControlScale = 0.68F;

    float maxValidatedHorizontalSpeed = 13.0F;
    float maxPositionError = 0.35F;
};

} // namespace nemisis::movement
