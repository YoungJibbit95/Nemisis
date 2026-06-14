#include "nemisis/player/PlayerAnimation.hpp"

#include <algorithm>
#include <cmath>

namespace nemisis::player {

namespace {

constexpr float kPi = 3.1415926535F;
constexpr float kTwoPi = kPi * 2.0F;

[[nodiscard]] float clamp01(float value) {
    return std::clamp(value, 0.0F, 1.0F);
}

[[nodiscard]] float approach(float current, float target, float rate, float deltaSeconds) {
    const float maxStep = std::max(0.0F, rate) * std::max(0.0F, deltaSeconds);
    if (current < target) {
        return std::min(target, current + maxStep);
    }
    return std::max(target, current - maxStep);
}

[[nodiscard]] float consumeTimer(float value, float deltaSeconds) {
    return std::max(0.0F, value - std::max(0.0F, deltaSeconds));
}

[[nodiscard]] float wrapPhase(float phase) {
    while (phase >= kTwoPi) {
        phase -= kTwoPi;
    }
    while (phase < 0.0F) {
        phase += kTwoPi;
    }
    return phase;
}

[[nodiscard]] float smoothstep01(float value) {
    value = clamp01(value);
    return value * value * (3.0F - (2.0F * value));
}

[[nodiscard]] float horizontalSpeed(novacore::math::Vec3 velocity) {
    return std::sqrt((velocity.x * velocity.x) + (velocity.z * velocity.z));
}

[[nodiscard]] float cueAlpha(float remaining, float duration) {
    if (duration <= 0.0F) {
        return 0.0F;
    }
    return clamp01(remaining / duration);
}

[[nodiscard]] bool isGroundedLike(movement::MovementMode mode) {
    return mode == movement::MovementMode::Grounded ||
        mode == movement::MovementMode::Sliding ||
        mode == movement::MovementMode::Dashing;
}

[[nodiscard]] CharacterAnimationClip chooseLocomotionClip(const CharacterAnimationInput& input) {
    if (input.movementMode == movement::MovementMode::Mantling) {
        return CharacterAnimationClip::Mantle;
    }
    if (input.movementMode == movement::MovementMode::WallRunning) {
        return CharacterAnimationClip::WallRun;
    }
    if (input.movementMode == movement::MovementMode::Sliding) {
        return CharacterAnimationClip::Slide;
    }
    if (input.movementMode == movement::MovementMode::Airborne ||
        input.movementMode == movement::MovementMode::Dashing) {
        return CharacterAnimationClip::Airborne;
    }
    if (input.speed01 >= 0.68F || input.tacticalSprintHeld) {
        return CharacterAnimationClip::Sprint;
    }
    if (input.speed01 >= 0.10F || horizontalSpeed(input.velocity) > 0.32F) {
        return CharacterAnimationClip::Walk;
    }
    return CharacterAnimationClip::Idle;
}

[[nodiscard]] CharacterAnimationClip chooseUpperBodyClip(const CharacterAnimationInput& input, float fireAlpha) {
    if (input.weapon.reloading) {
        return CharacterAnimationClip::Reload;
    }
    if (fireAlpha > 0.04F || input.weapon.timeSinceLastShotSeconds < 0.08F) {
        return CharacterAnimationClip::Fire;
    }
    if (input.adsHeld || input.weapon.adsAlpha > 0.62F) {
        return CharacterAnimationClip::Ads;
    }
    return CharacterAnimationClip::Idle;
}

[[nodiscard]] float locomotionAlphaFor(CharacterAnimationClip clip, const CharacterAnimationInput& input) {
    switch (clip) {
    case CharacterAnimationClip::Idle:
        return 1.0F - clamp01(input.speed01 * 3.0F);
    case CharacterAnimationClip::Walk:
        return clamp01(input.speed01 * 1.4F);
    case CharacterAnimationClip::Sprint:
        return clamp01((input.speed01 - 0.55F) / 0.45F);
    case CharacterAnimationClip::Slide:
    case CharacterAnimationClip::Airborne:
    case CharacterAnimationClip::WallRun:
    case CharacterAnimationClip::Mantle:
        return 1.0F;
    case CharacterAnimationClip::Reload:
    case CharacterAnimationClip::Ads:
    case CharacterAnimationClip::Fire:
        break;
    }
    return 0.0F;
}

[[nodiscard]] float upperBodyAlphaFor(CharacterAnimationClip clip, const CharacterAnimationInput& input, float fireAlpha) {
    switch (clip) {
    case CharacterAnimationClip::Reload:
        return input.weapon.reloading ? 1.0F : 0.0F;
    case CharacterAnimationClip::Fire:
        return std::max(fireAlpha, input.weapon.timeSinceLastShotSeconds < 0.08F ? 1.0F : 0.0F);
    case CharacterAnimationClip::Ads:
        return std::max(input.weapon.adsAlpha, input.adsHeld ? 0.85F : 0.0F);
    case CharacterAnimationClip::Idle:
        return 0.0F;
    case CharacterAnimationClip::Walk:
    case CharacterAnimationClip::Sprint:
    case CharacterAnimationClip::Slide:
    case CharacterAnimationClip::Airborne:
    case CharacterAnimationClip::WallRun:
    case CharacterAnimationClip::Mantle:
        break;
    }
    return 0.0F;
}

[[nodiscard]] float normalizedReloadArc(const weapons::WeaponRuntimeState& weapon, float reloadBlend) {
    if (!weapon.reloading && reloadBlend <= 0.0F) {
        return 0.0F;
    }
    const float progress = weapon.reloading ? clamp01(weapon.reloadProgress) : (1.0F - reloadBlend);
    return std::sin(progress * kPi) * std::max(reloadBlend, weapon.reloading ? 1.0F : 0.0F);
}

[[nodiscard]] novacore::math::Vec3 makeVec(float x, float y, float z) {
    return novacore::math::Vec3{x, y, z};
}

} // namespace

void resetCharacterAnimation(CharacterAnimationState& state) {
    state = {};
}

CharacterAnimationFrame updateCharacterAnimation(
    CharacterAnimationState& state,
    const CharacterAnimationInput& input) {
    const float dt = std::clamp(input.fixedDeltaSeconds, 0.0F, 0.10F);
    const float horizontal = horizontalSpeed(input.velocity);
    const float strideRate =
        2.2F +
        (clamp01(horizontal / 9.0F) * 8.8F) +
        (input.movementMode == movement::MovementMode::Sliding ? 2.0F : 0.0F) +
        (input.movementMode == movement::MovementMode::WallRunning ? 2.8F : 0.0F);

    state.locomotionPhase = wrapPhase(state.locomotionPhase + (strideRate * dt));
    state.idleBreathPhase = wrapPhase(state.idleBreathPhase + (1.45F * dt));
    state.fireKickSeconds = consumeTimer(state.fireKickSeconds, dt);
    if (input.weapon.shotIndex != state.observedShotIndex) {
        state.observedShotIndex = input.weapon.shotIndex;
        if (input.weapon.shotIndex > 0U) {
            state.fireKickSeconds = 0.105F;
        }
    }

    const float reloadTarget = input.weapon.reloading ? 1.0F : 0.0F;
    const float adsTarget = (input.adsHeld || input.weapon.adsAlpha > 0.01F) ? input.weapon.adsAlpha : 0.0F;
    const float wallRunTarget = input.movementMode == movement::MovementMode::WallRunning ? 1.0F : 0.0F;
    const float mantleTarget = input.movementMode == movement::MovementMode::Mantling ? 1.0F : 0.0F;
    const float slideTarget = input.movementMode == movement::MovementMode::Sliding ? 1.0F : 0.0F;
    const float airborneTarget = input.movementMode == movement::MovementMode::Airborne ||
            input.movementMode == movement::MovementMode::Dashing
        ? 1.0F
        : 0.0F;

    state.reloadBlend = approach(state.reloadBlend, reloadTarget, 7.0F, dt);
    state.adsBlend = approach(state.adsBlend, adsTarget, 9.5F, dt);
    state.wallRunBlend = approach(state.wallRunBlend, wallRunTarget, 8.0F, dt);
    state.mantleBlend = approach(state.mantleBlend, mantleTarget, 10.0F, dt);
    state.slideBlend = approach(state.slideBlend, slideTarget, 10.0F, dt);
    state.airborneBlend = approach(state.airborneBlend, airborneTarget, 8.0F, dt);
    state.previousMode = input.movementMode;

    return evaluateCharacterAnimation(state, input);
}

CharacterAnimationFrame evaluateCharacterAnimation(
    const CharacterAnimationState& state,
    const CharacterAnimationInput& input) {
    CharacterAnimationFrame frame{};
    frame.locomotionClip = chooseLocomotionClip(input);
    const float fireAlpha = clamp01(state.fireKickSeconds / 0.105F);
    frame.upperBodyClip = chooseUpperBodyClip(input, fireAlpha);
    frame.locomotionAlpha = locomotionAlphaFor(frame.locomotionClip, input);
    frame.upperBodyAlpha = upperBodyAlphaFor(frame.upperBodyClip, input, fireAlpha);
    frame.gaitPhase = state.locomotionPhase;
    frame.stride01 = clamp01(input.speed01);
    frame.idleBreath01 = (std::sin(state.idleBreathPhase) * 0.5F) + 0.5F;
    frame.adsAlpha = std::max(state.adsBlend, input.weapon.adsAlpha);
    frame.reloadAlpha = state.reloadBlend;
    frame.fireAlpha = fireAlpha;
    frame.slideAlpha = state.slideBlend;
    frame.airborneAlpha = state.airborneBlend;
    frame.wallRunAlpha = state.wallRunBlend;
    frame.mantleAlpha = std::max(state.mantleBlend, clamp01(input.mantleProgress01));
    frame.energyPlatformAlpha = cueAlpha(
        input.movementTech.energyPlatformSeconds,
        movement::kEnergyPlatformCueSeconds);
    frame.gravityBootAlpha = std::max(
        cueAlpha(input.movementTech.gravityInverterGlowSeconds, movement::kGravityInverterGlowCueSeconds),
        input.movementTech.gravityInvertersActive ? 0.78F : 0.0F);
    frame.mantleReachAlpha = std::max(
        cueAlpha(input.movementTech.mantleReachSeconds, movement::kMantleReachCueSeconds),
        cueAlpha(input.movementTech.mantleClimbSeconds, movement::kMantleClimbCueSeconds));

    const float gaitSin = std::sin(frame.gaitPhase);
    const float gaitCos = std::cos(frame.gaitPhase);
    const float groundedStride = isGroundedLike(input.movementMode) ? frame.stride01 : frame.stride01 * 0.35F;
    const float freeWeapon = 1.0F - (frame.adsAlpha * 0.78F);
    const float sprintAlpha = frame.locomotionClip == CharacterAnimationClip::Sprint ? frame.locomotionAlpha : 0.0F;
    const float reloadArc = normalizedReloadArc(input.weapon, frame.reloadAlpha);
    const float mantleEase = smoothstep01(input.mantleProgress01);
    const float wallRunLean = std::clamp(input.cameraRollDegrees / 14.0F, -1.0F, 1.0F) * frame.wallRunAlpha;
    frame.wallRunLean = wallRunLean;

    frame.firstPersonWeaponOffset =
        makeVec(gaitSin * 0.012F * groundedStride, gaitCos * 0.010F * groundedStride, 0.0F) +
        makeVec(0.045F * sprintAlpha, -0.066F * sprintAlpha, -0.155F * sprintAlpha) * freeWeapon +
        makeVec(0.020F, -0.115F, -0.070F) * frame.slideAlpha * freeWeapon +
        makeVec(wallRunLean * 0.030F, -0.026F * frame.wallRunAlpha, -0.024F * frame.wallRunAlpha) * freeWeapon +
        makeVec(0.044F, -0.085F + (mantleEase * 0.145F), -0.110F) * frame.mantleAlpha * freeWeapon +
        makeVec(0.0F, -0.120F * reloadArc, -0.018F * reloadArc) +
        makeVec(0.0F, 0.026F * frame.fireAlpha, -0.018F * frame.fireAlpha);

    frame.firstPersonArmsOffset =
        makeVec(gaitSin * 0.010F * groundedStride, gaitCos * 0.008F * groundedStride, 0.0F) +
        makeVec(0.038F * sprintAlpha, -0.056F * sprintAlpha, -0.100F * sprintAlpha) * freeWeapon +
        makeVec(0.020F, -0.090F, -0.052F) * frame.slideAlpha * freeWeapon +
        makeVec(wallRunLean * 0.024F, -0.020F * frame.wallRunAlpha, -0.020F * frame.wallRunAlpha) * freeWeapon +
        makeVec(0.024F, -0.035F + (mantleEase * 0.175F), -0.045F) * frame.mantleAlpha +
        makeVec(-0.048F * reloadArc, -0.110F * reloadArc, 0.0F) +
        makeVec(-0.040F * frame.energyPlatformAlpha, -0.035F * frame.energyPlatformAlpha, 0.030F * frame.energyPlatformAlpha);

    frame.firstPersonBodyOffset =
        makeVec(gaitSin * 0.012F * groundedStride, -0.020F * groundedStride, gaitCos * 0.008F * groundedStride) +
        makeVec(0.0F, -0.160F * frame.slideAlpha, -0.030F * frame.slideAlpha) +
        makeVec(wallRunLean * 0.024F, -0.020F * frame.wallRunAlpha, 0.0F) +
        makeVec(0.0F, 0.050F * frame.mantleAlpha, -0.020F * frame.mantleAlpha);

    frame.rightHandLocalOffset =
        makeVec(0.110F, -0.112F, -0.170F) +
        makeVec(0.012F * gaitSin * groundedStride, -0.020F * reloadArc, 0.010F * frame.fireAlpha) +
        makeVec(0.010F * wallRunLean, 0.0F, -0.012F * frame.wallRunAlpha);
    frame.leftHandLocalOffset =
        makeVec(-0.185F, -0.120F, 0.145F) +
        makeVec(-0.020F * reloadArc, -0.025F * reloadArc, 0.035F * reloadArc) +
        makeVec(-0.058F * frame.energyPlatformAlpha, -0.050F * frame.energyPlatformAlpha, 0.065F * frame.energyPlatformAlpha) +
        makeVec(0.018F * wallRunLean, 0.0F, -0.010F * frame.wallRunAlpha);
    frame.supportElbowLocalOffset =
        makeVec(-0.260F, -0.185F, -0.030F) +
        makeVec(-0.040F * reloadArc, -0.060F * reloadArc, 0.020F * reloadArc) +
        makeVec(-0.030F * frame.mantleReachAlpha, 0.035F * frame.mantleReachAlpha, 0.060F * frame.mantleReachAlpha);

    frame.thirdPersonBodyOffset =
        makeVec(0.0F, 0.030F * frame.airborneAlpha, 0.0F) +
        makeVec(0.0F, -0.100F * frame.slideAlpha, 0.0F) +
        makeVec(0.0F, 0.090F * frame.mantleAlpha, 0.0F);

    frame.firstPersonWeaponYawAddDegrees =
        (gaitSin * 0.32F * groundedStride * freeWeapon) +
        (wallRunLean * 1.5F) -
        (reloadArc * 1.8F);
    frame.firstPersonWeaponPitchAddDegrees =
        (-4.8F * sprintAlpha * freeWeapon) -
        (6.8F * frame.slideAlpha * freeWeapon) -
        (7.0F * frame.mantleAlpha * freeWeapon) +
        (9.0F * reloadArc) +
        (input.weapon.recoilPitchOffsetDegrees * 0.34F) -
        (frame.fireAlpha * 1.8F);
    frame.firstPersonWeaponRollAddDegrees =
        (gaitSin * 1.1F * groundedStride * freeWeapon) +
        (4.4F * sprintAlpha * freeWeapon) -
        (7.2F * frame.slideAlpha * freeWeapon) +
        (wallRunLean * 5.4F) -
        (8.5F * reloadArc) +
        (frame.energyPlatformAlpha * 2.0F);

    frame.firstPersonArmsYawAddDegrees =
        (gaitSin * 0.22F * groundedStride) +
        (wallRunLean * 1.2F) -
        (reloadArc * 1.3F);
    frame.firstPersonArmsPitchAddDegrees =
        (-3.5F * sprintAlpha * freeWeapon) -
        (5.2F * frame.slideAlpha * freeWeapon) -
        (8.0F * frame.mantleAlpha * (1.0F - (mantleEase * 0.35F))) +
        (8.0F * reloadArc) -
        (frame.energyPlatformAlpha * 7.5F) -
        (frame.fireAlpha * 1.2F);
    frame.firstPersonArmsRollAddDegrees =
        (gaitSin * 0.95F * groundedStride) +
        (3.6F * sprintAlpha * freeWeapon) -
        (5.0F * frame.slideAlpha * freeWeapon) +
        (wallRunLean * 4.2F) -
        (11.5F * reloadArc) +
        (frame.energyPlatformAlpha * 5.0F);

    frame.firstPersonBodyPitchDegrees =
        (-6.0F * frame.slideAlpha) +
        (4.0F * frame.mantleAlpha) -
        (2.0F * frame.airborneAlpha);
    frame.firstPersonBodyRollDegrees =
        (wallRunLean * 5.0F) +
        (gaitSin * 0.6F * groundedStride);
    frame.thirdPersonBodyPitchDegrees =
        (-10.0F * frame.slideAlpha) +
        (7.0F * frame.mantleAlpha) -
        (2.5F * frame.airborneAlpha);
    frame.thirdPersonBodyRollDegrees =
        (wallRunLean * 8.0F) +
        (gaitSin * 1.2F * groundedStride);

    return frame;
}

std::string_view characterAnimationClipName(CharacterAnimationClip clip) {
    switch (clip) {
    case CharacterAnimationClip::Idle:
        return "idle";
    case CharacterAnimationClip::Walk:
        return "walk";
    case CharacterAnimationClip::Sprint:
        return "sprint";
    case CharacterAnimationClip::Slide:
        return "slide";
    case CharacterAnimationClip::Airborne:
        return "airborne";
    case CharacterAnimationClip::WallRun:
        return "wallrun";
    case CharacterAnimationClip::Mantle:
        return "mantle";
    case CharacterAnimationClip::Reload:
        return "reload";
    case CharacterAnimationClip::Ads:
        return "ads";
    case CharacterAnimationClip::Fire:
        return "fire";
    }
    return "unknown";
}

} // namespace nemisis::player
