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
    fallback.jumpVelocity = numberOr(document, "jump_velocity", fallback.jumpVelocity);
    fallback.doubleJumpImpulse = numberOr(document, "double_jump_impulse", fallback.doubleJumpImpulse);
    fallback.gravity = numberOr(document, "gravity", fallback.gravity);
    fallback.airAcceleration = numberOr(document, "air_acceleration", fallback.airAcceleration);

    fallback.slideImpulse = numberOr(document, "slide.initial_impulse", fallback.slideImpulse);
    fallback.slideFriction = numberOr(document, "slide.friction", fallback.slideFriction);
    fallback.slideCooldownSeconds = numberOr(document, "slide.cooldown", fallback.slideCooldownSeconds);

    fallback.dashImpulse = numberOr(document, "dash.impulse", fallback.dashImpulse);
    fallback.dashCooldownSeconds = numberOr(document, "dash.cooldown", fallback.dashCooldownSeconds);

    fallback.wallRunSpeed = numberOr(document, "wall_run.speed", fallback.wallRunSpeed);
    fallback.wallRunMaxDurationSeconds = numberOr(document, "wall_run.max_duration", fallback.wallRunMaxDurationSeconds);
    fallback.wallJumpImpulse = numberOr(document, "wall_run.wall_jump_impulse", fallback.wallJumpImpulse);

    fallback.mantleMaxRange = numberOr(document, "mantle.max_range", fallback.mantleMaxRange);
    fallback.mantleMaxHeight = numberOr(document, "mantle.max_height", fallback.mantleMaxHeight);
    fallback.mantleDurationSeconds = numberOr(document, "mantle.duration", fallback.mantleDurationSeconds);

    fallback.maxValidatedHorizontalSpeed = numberOr(document, "validation.max_horizontal_speed", fallback.maxValidatedHorizontalSpeed);
    fallback.maxPositionError = numberOr(document, "validation.max_position_error", fallback.maxPositionError);
    return fallback;
}

} // namespace nemisis::movement
