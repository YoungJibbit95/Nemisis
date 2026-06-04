#pragma once

namespace nemisis::movement {

struct MovementTuning final {
    float walkSpeed = 5.2F;
    float sprintSpeed = 7.4F;
    float tacticalSprintSpeed = 8.6F;
    float crouchSpeed = 3.1F;
    float jumpVelocity = 5.8F;
    float doubleJumpImpulse = 5.1F;
    float gravity = -24.0F;
    float airAcceleration = 18.0F;

    float slideImpulse = 8.5F;
    float slideFriction = 7.0F;
    float slideCooldownSeconds = 0.25F;

    float dashImpulse = 9.0F;
    float dashCooldownSeconds = 0.8F;

    float wallRunSpeed = 8.2F;
    float wallRunMaxDurationSeconds = 1.35F;
    float wallJumpImpulse = 6.2F;

    float mantleMaxRange = 1.25F;
    float mantleMaxHeight = 1.4F;
    float mantleDurationSeconds = 0.22F;

    float maxValidatedHorizontalSpeed = 13.0F;
    float maxPositionError = 0.35F;
};

} // namespace nemisis::movement

