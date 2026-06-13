#include "nemisis/movement/MovementConfig.hpp"

namespace nemisis::movement {

namespace {

float numberOr(const novacore::core::ConfigDocument& document, const char* key, float fallback) {
    return static_cast<float>(document.numberOr(key, fallback));
}

} // namespace

MovementTuning movementTuningFromConfig(const novacore::core::ConfigDocument& document, MovementTuning fallback) {
    fallback.walkSpeed = numberOr(document, "walk_speed", fallback.walkSpeed);
    fallback.sprintSpeed = numberOr(document, "sprint_speed", fallback.sprintSpeed);
    fallback.tacticalSprintSpeed = numberOr(document, "tactical_sprint_speed", fallback.tacticalSprintSpeed);
    fallback.crouchSpeed = numberOr(document, "crouch_speed", fallback.crouchSpeed);
    fallback.groundAcceleration = numberOr(document, "ground.acceleration", fallback.groundAcceleration);
    fallback.groundDeceleration = numberOr(document, "ground.deceleration", fallback.groundDeceleration);
    fallback.groundFriction = numberOr(document, "ground.friction", fallback.groundFriction);
    fallback.groundStopSpeed = numberOr(document, "ground.stop_speed", fallback.groundStopSpeed);
    fallback.jumpVelocity = numberOr(document, "jump_velocity", fallback.jumpVelocity);
    fallback.coyoteTimeSeconds = numberOr(document, "jump.coyote_time", fallback.coyoteTimeSeconds);
    fallback.jumpBufferSeconds = numberOr(document, "jump.buffer_time", fallback.jumpBufferSeconds);
    fallback.doubleJumpImpulse = numberOr(document, "double_jump_impulse", fallback.doubleJumpImpulse);
    fallback.gravity = numberOr(document, "gravity", fallback.gravity);
    fallback.airAcceleration = numberOr(document, "air_acceleration", fallback.airAcceleration);
    fallback.airMaxSpeed = numberOr(document, "air.max_speed", fallback.airMaxSpeed);
    fallback.airDrag = numberOr(document, "air.drag", fallback.airDrag);

    fallback.slideImpulse = numberOr(document, "slide.initial_impulse", fallback.slideImpulse);
    fallback.slideMaxDurationSeconds = numberOr(document, "slide.max_duration", fallback.slideMaxDurationSeconds);
    fallback.slideFriction = numberOr(document, "slide.friction", fallback.slideFriction);
    fallback.slideSteeringAcceleration = numberOr(document, "slide.steering_acceleration", fallback.slideSteeringAcceleration);
    fallback.slideEndSpeed = numberOr(document, "slide.end_speed", fallback.slideEndSpeed);
    fallback.slideJumpBoost = numberOr(document, "slide.jump_boost", fallback.slideJumpBoost);
    fallback.slideCooldownSeconds = numberOr(document, "slide.cooldown", fallback.slideCooldownSeconds);
    fallback.slideBufferSeconds = numberOr(document, "slide.buffer_time", fallback.slideBufferSeconds);

    fallback.dashImpulse = numberOr(document, "dash.impulse", fallback.dashImpulse);
    fallback.dashDurationSeconds = numberOr(document, "dash.duration", fallback.dashDurationSeconds);
    fallback.dashSteeringAcceleration = numberOr(document, "dash.steering_acceleration", fallback.dashSteeringAcceleration);
    fallback.dashCooldownSeconds = numberOr(document, "dash.cooldown", fallback.dashCooldownSeconds);

    fallback.wallRunSpeed = numberOr(document, "wall_run.speed", fallback.wallRunSpeed);
    fallback.wallRunMaxDurationSeconds = numberOr(document, "wall_run.max_duration", fallback.wallRunMaxDurationSeconds);
    fallback.wallJumpImpulse = numberOr(document, "wall_run.wall_jump_impulse", fallback.wallJumpImpulse);
    fallback.wallRunMinHeight = numberOr(document, "wall_run.min_height", fallback.wallRunMinHeight);
    fallback.wallRunProbeDistance = numberOr(document, "wall_run.probe_distance", fallback.wallRunProbeDistance);

    fallback.mantleMaxRange = numberOr(document, "mantle.max_range", fallback.mantleMaxRange);
    fallback.mantleMaxHeight = numberOr(document, "mantle.max_height", fallback.mantleMaxHeight);
    fallback.mantleDurationSeconds = numberOr(document, "mantle.duration", fallback.mantleDurationSeconds);

    fallback.maxValidatedHorizontalSpeed = numberOr(document, "validation.max_horizontal_speed", fallback.maxValidatedHorizontalSpeed);
    fallback.maxPositionError = numberOr(document, "validation.max_position_error", fallback.maxPositionError);
    return fallback;
}

} // namespace nemisis::movement
