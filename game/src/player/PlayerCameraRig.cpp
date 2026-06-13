#include "nemisis/player/PlayerCameraRig.hpp"

#include <algorithm>
#include <cmath>

namespace nemisis::player {

namespace {

constexpr float kPi = 3.14159265358979323846F;

[[nodiscard]] float clamp01(float value) {
    return std::clamp(value, 0.0F, 1.0F);
}

[[nodiscard]] float wrapDegrees(float value) {
    while (value >= 180.0F) {
        value -= 360.0F;
    }
    while (value < -180.0F) {
        value += 360.0F;
    }
    return value;
}

[[nodiscard]] float shortestAngleDelta(float from, float to) {
    return wrapDegrees(to - from);
}

[[nodiscard]] float smoothFactor(float sharpness, float dt) {
    if (sharpness <= 0.0F) {
        return 1.0F;
    }
    return 1.0F - std::exp(-sharpness * std::max(0.0F, dt));
}

[[nodiscard]] float lerp(float a, float b, float t) {
    return a + ((b - a) * t);
}

[[nodiscard]] novacore::math::Vec3 lerpVec3(novacore::math::Vec3 a, novacore::math::Vec3 b, float t) {
    return novacore::math::Vec3{
        lerp(a.x, b.x, t),
        lerp(a.y, b.y, t),
        lerp(a.z, b.z, t),
    };
}

[[nodiscard]] bool isSliding(movement::MovementMode mode) {
    return mode == movement::MovementMode::Sliding;
}

[[nodiscard]] bool isDashing(movement::MovementMode mode) {
    return mode == movement::MovementMode::Dashing;
}

[[nodiscard]] bool isAirborne(movement::MovementMode mode) {
    return mode == movement::MovementMode::Airborne;
}

[[nodiscard]] bool isWallRunning(movement::MovementMode mode) {
    return mode == movement::MovementMode::WallRunning;
}

[[nodiscard]] float movementSpeed01(const CameraRigInput& input) {
    constexpr float kFastReferenceSpeed = 8.6F;
    return clamp01(cameraRigHorizontalSpeed(input.playerVelocity) / kFastReferenceSpeed);
}

[[nodiscard]] float targetEyeHeight(const CameraRigInput& input, const CameraRigTuning& tuning) {
    if (input.crouchHeld || isSliding(input.movementMode)) {
        return tuning.crouchEyeHeight;
    }
    return tuning.eyeHeight;
}

[[nodiscard]] float targetFov(const CameraRigInput& input, const CameraRigTuning& tuning, float speed01) {
    float target = tuning.baseVerticalFovDegrees;
    if (input.tacticalSprintHeld) {
        target += tuning.tacticalSprintFovKickDegrees * speed01;
    } else if (input.sprintHeld) {
        target += tuning.sprintFovKickDegrees * speed01;
    }
    if (isSliding(input.movementMode)) {
        target += tuning.slideFovKickDegrees;
    }
    if (isDashing(input.movementMode)) {
        target += tuning.dashFovKickDegrees;
    }
    if (isWallRunning(input.movementMode)) {
        target += tuning.wallRunFovKickDegrees;
    }
    if (isAirborne(input.movementMode)) {
        target += tuning.airborneFovKickDegrees;
    }

    const float adsMultiplier = std::clamp(input.weapon.adsAlpha, 0.0F, 1.0F);
    const float weaponFovMultiplier = std::clamp(input.adsHeld ? 0.82F : 1.0F, 0.70F, 1.0F);
    target = lerp(target, target * weaponFovMultiplier, adsMultiplier);
    return target;
}

[[nodiscard]] float targetRoll(const CameraRigInput& input, const CameraRigTuning& tuning) {
    float target = 0.0F;
    if (isSliding(input.movementMode)) {
        target += tuning.slideRollDegrees;
    }
    if (isDashing(input.movementMode)) {
        const float direction = input.playerVelocity.x >= 0.0F ? 1.0F : -1.0F;
        target += tuning.dashRollDegrees * direction;
    }
    if (isWallRunning(input.movementMode)) {
        const float direction = input.playerVelocity.x >= 0.0F ? 1.0F : -1.0F;
        target += tuning.wallRunRollDegrees * direction;
    }
    const float strafeSway = std::clamp(input.playerVelocity.x * 0.10F, -1.0F, 1.0F);
    target += strafeSway * tuning.velocitySwayDegrees;
    return target;
}

[[nodiscard]] novacore::math::Vec3 targetHeadBob(
    const CameraRigInput& input,
    const CameraRigTuning& tuning,
    float speed01,
    float phase) {
    if (isAirborne(input.movementMode) || isDashing(input.movementMode) || speed01 <= 0.02F) {
        return {};
    }

    const bool sprinting = input.sprintHeld || input.tacticalSprintHeld;
    const float amplitude = isSliding(input.movementMode)
        ? tuning.bobSlideAmplitude
        : sprinting
            ? tuning.bobSprintAmplitude
            : tuning.bobWalkAmplitude;
    return novacore::math::Vec3{
        std::sin(phase * 0.5F) * amplitude * 0.38F * speed01,
        std::abs(std::sin(phase)) * amplitude * speed01,
        0.0F,
    };
}

[[nodiscard]] novacore::math::Vec3 targetWeaponSway(
    const CameraRigInput& input,
    const CameraRigTuning& tuning,
    float speed01) {
    const float adsDamp = 1.0F - (std::clamp(input.weapon.adsAlpha, 0.0F, 1.0F) * 0.68F);
    return novacore::math::Vec3{
        std::clamp(-input.playerVelocity.x * tuning.weaponSwayScale, -0.09F, 0.09F) * adsDamp,
        (isSliding(input.movementMode) ? -0.05F : 0.0F) - (speed01 * 0.025F * adsDamp),
        std::clamp(-cameraRigHorizontalSpeed(input.playerVelocity) * 0.004F, -0.05F, 0.0F) * adsDamp,
    };
}

[[nodiscard]] PlayerViewComponent targetVisualView(
    const CameraRigInput& input,
    const CameraRigTuning& tuning,
    novacore::math::Vec3 headBob) {
    PlayerViewComponent view = input.authoredView;
    view.pitchDegrees = std::clamp(
        view.pitchDegrees -
            (input.weapon.recoilPitchOffsetDegrees * tuning.recoilPitchVisualScale) +
            (headBob.y * 14.0F),
        -tuning.maxVisualPitchDegrees,
        tuning.maxVisualPitchDegrees);
    view.yawDegrees = wrapDegrees(
        view.yawDegrees +
        (input.weapon.recoilYawOffsetDegrees * tuning.recoilYawVisualScale) +
        std::clamp(-input.playerVelocity.x * 0.018F, -0.22F, 0.22F));
    return view;
}

} // namespace

void resetCameraRig(CameraRigState& state) {
    state = CameraRigState{};
}

float cameraRigHorizontalSpeed(novacore::math::Vec3 velocity) {
    return std::sqrt((velocity.x * velocity.x) + (velocity.z * velocity.z));
}

CameraRigFrame updateCameraRig(
    CameraRigState& state,
    const CameraRigInput& input,
    const CameraRigTuning& tuning) {
    const float dt = std::max(0.0F, input.fixedDeltaSeconds);
    const float speed01 = movementSpeed01(input);
    const float frequency = (input.sprintHeld || input.tacticalSprintHeld)
        ? tuning.bobSprintFrequency
        : tuning.bobWalkFrequency;
    state.bobPhase += dt * frequency * (0.25F + (speed01 * 0.95F)) * (2.0F * kPi);
    if (state.bobPhase > 4096.0F) {
        state.bobPhase = std::fmod(state.bobPhase, 2.0F * kPi);
    }

    const auto targetBob = targetHeadBob(input, tuning, speed01, state.bobPhase);
    const auto targetSway = targetWeaponSway(input, tuning, speed01);
    const auto targetPosition =
        input.playerPosition +
        novacore::math::Vec3{0.0F, targetEyeHeight(input, tuning), 0.0F} +
        targetBob;
    const auto targetView = targetVisualView(input, tuning, targetBob);
    const float desiredFov = targetFov(input, tuning, speed01);
    const float desiredRoll = targetRoll(input, tuning);

    if (!state.initialized) {
        state.initialized = true;
        state.position = targetPosition;
        state.view = targetView;
        state.headBobOffset = targetBob;
        state.weaponSwayOffset = targetSway;
        state.rollDegrees = desiredRoll;
        state.verticalFovDegrees = desiredFov;
        state.speed01 = speed01;
        state.adsAlpha = input.weapon.adsAlpha;
    } else {
        const float positionT = smoothFactor(tuning.positionSharpness, dt);
        const float viewT = smoothFactor(tuning.viewSharpness, dt);
        const float fovT = smoothFactor(tuning.fovSharpness, dt);
        state.position = lerpVec3(state.position, targetPosition, positionT);
        state.headBobOffset = lerpVec3(state.headBobOffset, targetBob, positionT);
        state.weaponSwayOffset = lerpVec3(state.weaponSwayOffset, targetSway, positionT);
        state.view.yawDegrees = wrapDegrees(
            state.view.yawDegrees +
            shortestAngleDelta(state.view.yawDegrees, targetView.yawDegrees) * viewT);
        state.view.pitchDegrees = lerp(state.view.pitchDegrees, targetView.pitchDegrees, viewT);
        state.rollDegrees = lerp(state.rollDegrees, desiredRoll, viewT);
        state.verticalFovDegrees = lerp(state.verticalFovDegrees, desiredFov, fovT);
        state.speed01 = lerp(state.speed01, speed01, positionT);
        state.adsAlpha = lerp(state.adsAlpha, input.weapon.adsAlpha, fovT);
    }

    return CameraRigFrame{
        state.position,
        state.view,
        state.headBobOffset,
        state.weaponSwayOffset,
        state.rollDegrees,
        state.verticalFovDegrees,
        state.speed01,
        state.adsAlpha,
    };
}

} // namespace nemisis::player
