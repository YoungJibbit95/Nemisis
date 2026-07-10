#include "nemisis/player/PlayerAnimation.hpp"

#include <algorithm>
#include <cmath>

namespace nemisis::player {

namespace {

constexpr float kPi = 3.1415926535F;
constexpr float kTwoPi = kPi * 2.0F;
constexpr float kRadiansToDegrees = 180.0F / kPi;
constexpr float kDegreesToRadians = kPi / 180.0F;
constexpr float kFireKickDurationSeconds = 0.105F;
constexpr float kLandDurationSeconds = 0.24F;

[[nodiscard]] constexpr std::size_t boneIndex(CharacterBone bone) {
    return static_cast<std::size_t>(bone);
}

[[nodiscard]] constexpr std::size_t socketIndex(CharacterSocket socket) {
    return static_cast<std::size_t>(socket);
}

[[nodiscard]] float finiteOr(float value, float fallback = 0.0F) {
    return std::isfinite(value) ? value : fallback;
}

[[nodiscard]] float clamp01(float value) {
    return std::clamp(finiteOr(value), 0.0F, 1.0F);
}

[[nodiscard]] float approach(float current, float target, float rate, float deltaSeconds) {
    const float maxStep = std::max(0.0F, finiteOr(rate)) * std::max(0.0F, finiteOr(deltaSeconds));
    if (current < target) {
        return std::min(target, current + maxStep);
    }
    return std::max(target, current - maxStep);
}

[[nodiscard]] float consumeTimer(float value, float deltaSeconds) {
    return std::max(0.0F, finiteOr(value) - std::max(0.0F, finiteOr(deltaSeconds)));
}

[[nodiscard]] float wrapPhase(float phase) {
    phase = std::fmod(finiteOr(phase), kTwoPi);
    return phase < 0.0F ? phase + kTwoPi : phase;
}

[[nodiscard]] float wrap01(float value) {
    value = std::fmod(finiteOr(value), 1.0F);
    return value < 0.0F ? value + 1.0F : value;
}

[[nodiscard]] float smoothstep01(float value) {
    value = clamp01(value);
    return value * value * (3.0F - (2.0F * value));
}

[[nodiscard]] float horizontalSpeed(novacore::math::Vec3 velocity) {
    const float x = finiteOr(velocity.x);
    const float z = finiteOr(velocity.z);
    return std::sqrt((x * x) + (z * z));
}

[[nodiscard]] novacore::math::Vec3 normalizedHorizontal(
    novacore::math::Vec3 value,
    novacore::math::Vec3 fallback) {
    const float length = horizontalSpeed(value);
    if (length <= 0.0001F) {
        const float fallbackLength = horizontalSpeed(fallback);
        if (fallbackLength <= 0.0001F) {
            return {0.0F, 0.0F, 1.0F};
        }
        return {fallback.x / fallbackLength, 0.0F, fallback.z / fallbackLength};
    }
    return {value.x / length, 0.0F, value.z / length};
}

[[nodiscard]] float dotHorizontal(novacore::math::Vec3 lhs, novacore::math::Vec3 rhs) {
    return (finiteOr(lhs.x) * finiteOr(rhs.x)) + (finiteOr(lhs.z) * finiteOr(rhs.z));
}

[[nodiscard]] float cueAlpha(float remaining, float duration) {
    return duration > 0.0F ? clamp01(remaining / duration) : 0.0F;
}

[[nodiscard]] bool isGroundedLike(movement::MovementMode mode) {
    return mode == movement::MovementMode::Grounded ||
        mode == movement::MovementMode::Sliding;
}

[[nodiscard]] bool isAirborneLike(movement::MovementMode mode) {
    return mode == movement::MovementMode::Airborne ||
        mode == movement::MovementMode::Dashing ||
        mode == movement::MovementMode::WallRunning;
}

[[nodiscard]] bool isLoopingClip(CharacterAnimationClip clip) {
    return clip == CharacterAnimationClip::Idle ||
        clip == CharacterAnimationClip::Walk ||
        clip == CharacterAnimationClip::Sprint ||
        clip == CharacterAnimationClip::Crouch ||
        clip == CharacterAnimationClip::Airborne ||
        clip == CharacterAnimationClip::Fall ||
        clip == CharacterAnimationClip::WallRun ||
        clip == CharacterAnimationClip::Ads;
}

[[nodiscard]] float clipDurationSeconds(CharacterAnimationClip clip) {
    switch (clip) {
    case CharacterAnimationClip::Idle:
        return 2.40F;
    case CharacterAnimationClip::Walk:
        return 0.82F;
    case CharacterAnimationClip::Sprint:
        return 0.58F;
    case CharacterAnimationClip::Crouch:
        return 1.05F;
    case CharacterAnimationClip::Slide:
        return 0.65F;
    case CharacterAnimationClip::Jump:
        return 0.42F;
    case CharacterAnimationClip::Airborne:
        return 0.90F;
    case CharacterAnimationClip::Fall:
        return 0.72F;
    case CharacterAnimationClip::Land:
        return kLandDurationSeconds;
    case CharacterAnimationClip::WallRun:
        return 0.68F;
    case CharacterAnimationClip::Mantle:
        return 0.80F;
    case CharacterAnimationClip::Reload:
        return 1.0F;
    case CharacterAnimationClip::Ads:
        return 0.18F;
    case CharacterAnimationClip::Fire:
        return kFireKickDurationSeconds;
    }
    return 1.0F;
}

[[nodiscard]] float normalizedClipTime(CharacterAnimationClip clip, float elapsedSeconds) {
    const float duration = std::max(0.001F, clipDurationSeconds(clip));
    const float normalized = std::max(0.0F, finiteOr(elapsedSeconds)) / duration;
    return isLoopingClip(clip) ? wrap01(normalized) : clamp01(normalized);
}

[[nodiscard]] CharacterClipPhase playbackPhase(CharacterAnimationClip clip, float normalizedTime) {
    if (isLoopingClip(clip)) {
        return CharacterClipPhase::Loop;
    }
    if (normalizedTime < 0.20F) {
        return CharacterClipPhase::Enter;
    }
    if (normalizedTime >= 0.82F) {
        return CharacterClipPhase::Exit;
    }
    return CharacterClipPhase::Loop;
}

[[nodiscard]] CharacterAnimationClip chooseLocomotionClip(
    const CharacterAnimationInput& input,
    const CharacterAnimationState& state) {
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
        if (input.velocity.y > 0.35F) {
            return CharacterAnimationClip::Jump;
        }
        if (input.velocity.y < -0.35F) {
            return CharacterAnimationClip::Fall;
        }
        return CharacterAnimationClip::Airborne;
    }
    if (state.landSeconds > 0.0F) {
        return CharacterAnimationClip::Land;
    }
    if (input.crouchHeld) {
        return CharacterAnimationClip::Crouch;
    }
    if (input.speed01 >= 0.68F || input.tacticalSprintHeld) {
        return CharacterAnimationClip::Sprint;
    }
    if (input.speed01 >= 0.10F || horizontalSpeed(input.velocity) > 0.32F) {
        return CharacterAnimationClip::Walk;
    }
    return CharacterAnimationClip::Idle;
}

[[nodiscard]] CharacterAnimationClip chooseUpperBodyClip(
    const CharacterAnimationInput& input,
    float fireAlpha) {
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
    case CharacterAnimationClip::Crouch:
    case CharacterAnimationClip::Jump:
    case CharacterAnimationClip::Fall:
    case CharacterAnimationClip::Land:
    case CharacterAnimationClip::Slide:
    case CharacterAnimationClip::Airborne:
    case CharacterAnimationClip::WallRun:
    case CharacterAnimationClip::Mantle:
        return 1.0F;
    case CharacterAnimationClip::Reload:
    case CharacterAnimationClip::Ads:
    case CharacterAnimationClip::Fire:
        return 0.0F;
    }
    return 0.0F;
}

[[nodiscard]] float upperBodyAlphaFor(
    CharacterAnimationClip clip,
    const CharacterAnimationInput& input,
    float fireAlpha) {
    switch (clip) {
    case CharacterAnimationClip::Reload:
        return input.weapon.reloading ? 1.0F : 0.0F;
    case CharacterAnimationClip::Fire:
        return std::max(fireAlpha, input.weapon.timeSinceLastShotSeconds < 0.08F ? 1.0F : 0.0F);
    case CharacterAnimationClip::Ads:
        return std::max(input.weapon.adsAlpha, input.adsHeld ? 0.85F : 0.0F);
    default:
        return 0.0F;
    }
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

[[nodiscard]] novacore::math::Quat multiply(
    novacore::math::Quat lhs,
    novacore::math::Quat rhs) {
    return {
        (lhs.w * rhs.x) + (lhs.x * rhs.w) + (lhs.y * rhs.z) - (lhs.z * rhs.y),
        (lhs.w * rhs.y) - (lhs.x * rhs.z) + (lhs.y * rhs.w) + (lhs.z * rhs.x),
        (lhs.w * rhs.z) + (lhs.x * rhs.y) - (lhs.y * rhs.x) + (lhs.z * rhs.w),
        (lhs.w * rhs.w) - (lhs.x * rhs.x) - (lhs.y * rhs.y) - (lhs.z * rhs.z),
    };
}

[[nodiscard]] novacore::math::Quat rotationDegrees(float pitch, float yaw, float roll) {
    const float halfPitch = finiteOr(pitch) * kDegreesToRadians * 0.5F;
    const float halfYaw = finiteOr(yaw) * kDegreesToRadians * 0.5F;
    const float halfRoll = finiteOr(roll) * kDegreesToRadians * 0.5F;
    const novacore::math::Quat x{std::sin(halfPitch), 0.0F, 0.0F, std::cos(halfPitch)};
    const novacore::math::Quat y{0.0F, std::sin(halfYaw), 0.0F, std::cos(halfYaw)};
    const novacore::math::Quat z{0.0F, 0.0F, std::sin(halfRoll), std::cos(halfRoll)};
    return multiply(multiply(y, x), z);
}

void pushEvent(
    CharacterAnimationState& state,
    CharacterAnimationEvent event,
    float normalizedTime,
    float weight = 1.0F) {
    if (state.eventCount >= state.events.size()) {
        return;
    }
    state.events[state.eventCount++] = CharacterAnimationEventMarker{
        event,
        clamp01(normalizedTime),
        clamp01(weight),
        state.simulationTick,
    };
}

[[nodiscard]] bool crossedLoopMarker(float previous, float advance, float marker) {
    const float before = std::floor(previous - marker);
    const float after = std::floor(previous + std::max(0.0F, advance) - marker);
    return after > before;
}

void updateClipState(
    CharacterAnimationClip desired,
    CharacterAnimationClip& active,
    CharacterAnimationClip& previous,
    float& elapsedSeconds,
    float& transitionAlpha,
    float transitionDurationSeconds,
    float deltaSeconds) {
    if (desired != active) {
        previous = active;
        active = desired;
        elapsedSeconds = 0.0F;
        transitionAlpha = 0.0F;
    } else {
        elapsedSeconds += deltaSeconds;
    }
    transitionAlpha = approach(
        transitionAlpha,
        1.0F,
        1.0F / std::max(0.001F, transitionDurationSeconds),
        deltaSeconds);
}

void setBone(
    CharacterSkeletonPose& pose,
    CharacterBone bone,
    CharacterBone parent,
    novacore::math::Vec3 translation,
    novacore::math::Quat rotation = {}) {
    pose.bones[boneIndex(bone)] = CharacterBonePose{
        bone,
        parent,
        CharacterPoseTransform{translation, rotation, {1.0F, 1.0F, 1.0F}},
    };
}

void setSocket(
    CharacterSkeletonPose& pose,
    CharacterSocket socket,
    CharacterBone parent,
    novacore::math::Vec3 translation,
    novacore::math::Quat rotation = {}) {
    pose.sockets[socketIndex(socket)] = CharacterSocketPose{
        socket,
        parent,
        CharacterPoseTransform{translation, rotation, {1.0F, 1.0F, 1.0F}},
    };
}

void buildSkeletonPose(CharacterAnimationFrame& frame, const CharacterAnimationInput& input) {
    const float gaitSin = std::sin(frame.gaitPhase);
    const float gaitCos = std::cos(frame.gaitPhase);
    const float moving = frame.stride01 * (1.0F - frame.landAlpha);
    const float legSwing = gaitSin * 28.0F * moving;
    const float armSwing = -legSwing * (1.0F - (frame.adsAlpha * 0.85F));
    const float crouch = frame.crouchAlpha;
    const float slide = frame.slideAlpha;
    const float mantle = frame.mantleAlpha;
    const float reloadArc = normalizedReloadArc(input.weapon, frame.reloadAlpha);
    const float aimPitch = std::clamp(finiteOr(input.aimPitchDegrees), -85.0F, 85.0F) * frame.adsAlpha;
    const float aimYaw = std::clamp(finiteOr(input.aimYawDegrees), -90.0F, 90.0F) * frame.adsAlpha;
    const float lowerBodyPitch = (-15.0F * crouch) + (-26.0F * slide) + (9.0F * mantle);
    const float pelvisDrop = (-0.18F * crouch) + (-0.30F * slide) + (0.08F * frame.landAlpha);
    const float kneeBend = (32.0F * crouch) + (48.0F * slide) + (18.0F * frame.landAlpha);
    const float upperPitch = (aimPitch * 0.48F) + (frame.fireAlpha * -2.5F);
    const float upperYaw = (aimYaw * 0.55F) + (frame.turnLean * 3.0F);

    setBone(frame.pose, CharacterBone::Root, CharacterBone::Invalid, frame.thirdPersonBodyOffset,
        rotationDegrees(frame.thirdPersonBodyPitchDegrees, 0.0F, frame.thirdPersonBodyRollDegrees));
    setBone(frame.pose, CharacterBone::Pelvis, CharacterBone::Root, {0.0F, 0.92F + pelvisDrop, 0.04F * crouch},
        rotationDegrees(lowerBodyPitch, frame.turnLean * 4.0F, -frame.turnLean * 2.0F));
    setBone(frame.pose, CharacterBone::SpineLower, CharacterBone::Pelvis, {0.0F, 0.18F, 0.0F},
        rotationDegrees(upperPitch * 0.25F, upperYaw * 0.20F, frame.wallRunLean * 3.0F));
    setBone(frame.pose, CharacterBone::SpineUpper, CharacterBone::SpineLower, {0.0F, 0.18F, 0.0F},
        rotationDegrees(upperPitch * 0.35F, upperYaw * 0.34F, frame.wallRunLean * 4.0F));
    setBone(frame.pose, CharacterBone::Chest, CharacterBone::SpineUpper, {0.0F, 0.17F, 0.0F},
        rotationDegrees(upperPitch * 0.40F, upperYaw * 0.46F, frame.wallRunLean * 3.0F));
    setBone(frame.pose, CharacterBone::Neck, CharacterBone::Chest, {0.0F, 0.16F, 0.0F},
        rotationDegrees(aimPitch * 0.12F, aimYaw * 0.12F, -frame.wallRunLean * 2.0F));
    setBone(frame.pose, CharacterBone::Head, CharacterBone::Neck, {0.0F, 0.14F, 0.0F},
        rotationDegrees(aimPitch * 0.08F, aimYaw * 0.08F, 0.0F));

    setBone(frame.pose, CharacterBone::ClavicleLeft, CharacterBone::Chest, {-0.12F, 0.10F, 0.0F},
        rotationDegrees(0.0F, -8.0F * frame.adsAlpha, -3.0F));
    setBone(frame.pose, CharacterBone::UpperArmLeft, CharacterBone::ClavicleLeft, {-0.18F, 0.0F, 0.0F},
        rotationDegrees(armSwing - (36.0F * frame.adsAlpha) - (35.0F * reloadArc) - (78.0F * mantle), 0.0F, -8.0F));
    setBone(frame.pose, CharacterBone::ForearmLeft, CharacterBone::UpperArmLeft, {-0.25F, 0.0F, 0.0F},
        rotationDegrees(-42.0F * frame.adsAlpha - (28.0F * reloadArc) - (42.0F * mantle), 0.0F, 4.0F));
    setBone(frame.pose, CharacterBone::HandLeft, CharacterBone::ForearmLeft, {-0.22F, 0.0F, 0.0F},
        rotationDegrees(-8.0F * reloadArc, 0.0F, 12.0F * frame.energyPlatformAlpha));

    setBone(frame.pose, CharacterBone::ClavicleRight, CharacterBone::Chest, {0.12F, 0.10F, 0.0F},
        rotationDegrees(0.0F, 8.0F * frame.adsAlpha, 3.0F));
    setBone(frame.pose, CharacterBone::UpperArmRight, CharacterBone::ClavicleRight, {0.18F, 0.0F, 0.0F},
        rotationDegrees(-armSwing - (32.0F * frame.adsAlpha) - (70.0F * mantle) - (3.0F * frame.fireAlpha), 0.0F, 8.0F));
    setBone(frame.pose, CharacterBone::ForearmRight, CharacterBone::UpperArmRight, {0.25F, 0.0F, 0.0F},
        rotationDegrees(-38.0F * frame.adsAlpha - (38.0F * mantle) - (4.0F * frame.fireAlpha), 0.0F, -4.0F));
    setBone(frame.pose, CharacterBone::HandRight, CharacterBone::ForearmRight, {0.22F, 0.0F, 0.0F},
        rotationDegrees(-2.0F * frame.fireAlpha, input.weapon.recoilYawOffsetDegrees * 0.25F, 0.0F));

    setBone(frame.pose, CharacterBone::ThighLeft, CharacterBone::Pelvis, {-0.10F, -0.05F, 0.0F},
        rotationDegrees(legSwing + kneeBend, 0.0F, 0.0F));
    setBone(frame.pose, CharacterBone::CalfLeft, CharacterBone::ThighLeft, {0.0F, -0.42F, 0.0F},
        rotationDegrees((-kneeBend * 1.35F) + std::max(0.0F, -gaitCos) * 12.0F * moving, 0.0F, 0.0F));
    setBone(frame.pose, CharacterBone::FootLeft, CharacterBone::CalfLeft, {0.0F, -0.42F, 0.05F},
        rotationDegrees(kneeBend * 0.35F, 0.0F, 0.0F));
    setBone(frame.pose, CharacterBone::ThighRight, CharacterBone::Pelvis, {0.10F, -0.05F, 0.0F},
        rotationDegrees(-legSwing + kneeBend, 0.0F, 0.0F));
    setBone(frame.pose, CharacterBone::CalfRight, CharacterBone::ThighRight, {0.0F, -0.42F, 0.0F},
        rotationDegrees((-kneeBend * 1.35F) + std::max(0.0F, gaitCos) * 12.0F * moving, 0.0F, 0.0F));
    setBone(frame.pose, CharacterBone::FootRight, CharacterBone::CalfRight, {0.0F, -0.42F, 0.05F},
        rotationDegrees(kneeBend * 0.35F, 0.0F, 0.0F));

    setSocket(frame.pose, CharacterSocket::Camera, CharacterBone::Head, {0.0F, 0.12F, 0.02F});
    setSocket(frame.pose, CharacterSocket::WeaponGrip, CharacterBone::HandRight, {0.02F, -0.02F, -0.06F},
        rotationDegrees(0.0F, 90.0F, 0.0F));
    setSocket(frame.pose, CharacterSocket::SupportHand, CharacterBone::HandLeft, {0.0F, -0.01F, -0.03F});
    setSocket(frame.pose, CharacterSocket::Muzzle, CharacterBone::HandRight, {0.03F, -0.02F, -0.72F});
    setSocket(frame.pose, CharacterSocket::FootLeft, CharacterBone::FootLeft, {0.0F, -0.02F, 0.10F});
    setSocket(frame.pose, CharacterSocket::FootRight, CharacterBone::FootRight, {0.0F, -0.02F, 0.10F});
}

void appendAdditiveLayer(
    CharacterAnimationFrame& frame,
    CharacterAnimationClip clip,
    CharacterPoseLayer layer,
    float weight,
    float normalizedTime) {
    if (weight <= 0.001F || frame.additiveLayerCount >= frame.additiveLayers.size()) {
        return;
    }
    frame.additiveLayers[frame.additiveLayerCount++] = {
        clip,
        layer,
        clamp01(weight),
        clamp01(normalizedTime),
    };
}

} // namespace

void resetCharacterAnimation(CharacterAnimationState& state) {
    state = {};
}

CharacterAnimationFrame updateCharacterAnimation(
    CharacterAnimationState& state,
    const CharacterAnimationInput& input) {
    const float dt = std::clamp(finiteOr(input.fixedDeltaSeconds), 0.0F, 0.10F);
    const float horizontal = horizontalSpeed(input.velocity);
    const float strideRate =
        2.2F +
        (clamp01(horizontal / 9.0F) * 8.8F) +
        (input.movementMode == movement::MovementMode::Sliding ? 2.0F : 0.0F) +
        (input.movementMode == movement::MovementMode::WallRunning ? 2.8F : 0.0F);
    const float previousGait01 = wrap01(state.locomotionPhase / kTwoPi);
    const float gaitAdvance01 = (strideRate * dt) / kTwoPi;

    ++state.simulationTick;
    state.eventCount = 0U;
    state.locomotionPhase = wrapPhase(state.locomotionPhase + (strideRate * dt));
    state.idleBreathPhase = wrapPhase(state.idleBreathPhase + (1.45F * dt));
    state.fireKickSeconds = consumeTimer(state.fireKickSeconds, dt);
    state.landSeconds = consumeTimer(state.landSeconds, dt);

    if (isAirborneLike(state.previousMode) && isGroundedLike(input.movementMode)) {
        state.landSeconds = kLandDurationSeconds;
        pushEvent(state, CharacterAnimationEvent::Land, 0.0F, clamp01(std::abs(state.previousVerticalVelocity) / 12.0F));
    }
    if (isGroundedLike(state.previousMode) && input.movementMode == movement::MovementMode::Airborne &&
        input.velocity.y > 0.0F) {
        pushEvent(state, CharacterAnimationEvent::JumpStart, 0.0F);
    }
    if (isAirborneLike(state.previousMode) && isAirborneLike(input.movementMode) &&
        state.previousVerticalVelocity > 0.0F && input.velocity.y <= 0.0F) {
        pushEvent(state, CharacterAnimationEvent::Apex, 0.5F);
    }
    if (state.previousMode != movement::MovementMode::Sliding &&
        input.movementMode == movement::MovementMode::Sliding) {
        pushEvent(state, CharacterAnimationEvent::SlideStart, 0.0F);
    }
    if (state.previousMode != movement::MovementMode::WallRunning &&
        input.movementMode == movement::MovementMode::WallRunning) {
        pushEvent(state, CharacterAnimationEvent::WallRunStart, 0.0F);
    }
    if (state.previousMode == movement::MovementMode::WallRunning &&
        input.movementMode != movement::MovementMode::WallRunning) {
        pushEvent(state, CharacterAnimationEvent::WallRunEnd, 1.0F);
    }
    if (state.previousMode != movement::MovementMode::Mantling &&
        input.movementMode == movement::MovementMode::Mantling) {
        pushEvent(state, CharacterAnimationEvent::MantleReach, 0.0F);
    }
    if (input.movementMode == movement::MovementMode::Mantling &&
        state.previousMantleProgress01 < 0.45F && input.mantleProgress01 >= 0.45F) {
        pushEvent(state, CharacterAnimationEvent::MantlePull, input.mantleProgress01);
    }
    if (state.previousMode == movement::MovementMode::Mantling &&
        input.movementMode != movement::MovementMode::Mantling) {
        pushEvent(state, CharacterAnimationEvent::MantleComplete, 1.0F);
    }

    if (input.weapon.shotIndex != state.observedShotIndex) {
        state.observedShotIndex = input.weapon.shotIndex;
        if (input.weapon.shotIndex > 0U) {
            state.fireKickSeconds = kFireKickDurationSeconds;
            pushEvent(state, CharacterAnimationEvent::Fire, 0.0F);
        }
    }
    if (!state.previousReloading && input.weapon.reloading) {
        pushEvent(state, CharacterAnimationEvent::ReloadStart, 0.0F);
    }
    if (input.weapon.reloading && state.previousReloadProgress01 < 0.50F &&
        input.weapon.reloadProgress >= 0.50F) {
        pushEvent(state, CharacterAnimationEvent::ReloadInsert, 0.50F);
    }
    if (state.previousReloading && !input.weapon.reloading) {
        pushEvent(state, CharacterAnimationEvent::ReloadComplete, 1.0F);
    }
    const bool adsActive = input.adsHeld || input.weapon.adsAlpha > 0.50F;
    if (!state.previousAdsActive && adsActive) {
        pushEvent(state, CharacterAnimationEvent::AdsEnter, 0.0F);
    } else if (state.previousAdsActive && !adsActive) {
        pushEvent(state, CharacterAnimationEvent::AdsExit, 1.0F);
    }

    const float fireAlpha = clamp01(state.fireKickSeconds / kFireKickDurationSeconds);
    const CharacterAnimationClip desiredLocomotion = chooseLocomotionClip(input, state);
    const CharacterAnimationClip desiredUpperBody = chooseUpperBodyClip(input, fireAlpha);
    updateClipState(
        desiredLocomotion,
        state.locomotionClip,
        state.previousLocomotionClip,
        state.locomotionClipSeconds,
        state.locomotionTransitionAlpha,
        desiredLocomotion == CharacterAnimationClip::Land ? 0.055F : 0.11F,
        dt);
    updateClipState(
        desiredUpperBody,
        state.upperBodyClip,
        state.previousUpperBodyClip,
        state.upperBodyClipSeconds,
        state.upperBodyTransitionAlpha,
        desiredUpperBody == CharacterAnimationClip::Fire ? 0.025F : 0.08F,
        dt);

    if ((desiredLocomotion == CharacterAnimationClip::Walk ||
            desiredLocomotion == CharacterAnimationClip::Sprint ||
            desiredLocomotion == CharacterAnimationClip::Crouch) &&
        state.locomotionTransitionAlpha > 0.35F) {
        const float weight = clamp01(input.speed01 * (desiredLocomotion == CharacterAnimationClip::Crouch ? 0.75F : 1.0F));
        if (crossedLoopMarker(previousGait01, gaitAdvance01, 0.0F)) {
            pushEvent(state, CharacterAnimationEvent::FootstepLeft, 0.0F, weight);
        }
        if (crossedLoopMarker(previousGait01, gaitAdvance01, 0.5F)) {
            pushEvent(state, CharacterAnimationEvent::FootstepRight, 0.5F, weight);
        }
    }

    const float reloadTarget = input.weapon.reloading ? 1.0F : 0.0F;
    const float adsTarget = (input.adsHeld || input.weapon.adsAlpha > 0.01F) ?
        std::max(input.weapon.adsAlpha, input.adsHeld ? 1.0F : 0.0F) : 0.0F;
    const float wallRunTarget = input.movementMode == movement::MovementMode::WallRunning ? 1.0F : 0.0F;
    const float mantleTarget = input.movementMode == movement::MovementMode::Mantling ? 1.0F : 0.0F;
    const float slideTarget = input.movementMode == movement::MovementMode::Sliding ? 1.0F : 0.0F;
    const float airborneTarget = isAirborneLike(input.movementMode) ? 1.0F : 0.0F;
    const float crouchTarget = input.crouchHeld && isGroundedLike(input.movementMode) ? 1.0F : 0.0F;
    const float landTarget = state.landSeconds > 0.0F ? 1.0F : 0.0F;
    const float turnTarget = std::clamp(finiteOr(input.turnDeltaDegrees) / 35.0F, -1.0F, 1.0F);

    state.reloadBlend = approach(state.reloadBlend, reloadTarget, 7.0F, dt);
    state.adsBlend = approach(state.adsBlend, adsTarget, 9.5F, dt);
    state.wallRunBlend = approach(state.wallRunBlend, wallRunTarget, 8.0F, dt);
    state.mantleBlend = approach(state.mantleBlend, mantleTarget, 10.0F, dt);
    state.slideBlend = approach(state.slideBlend, slideTarget, 10.0F, dt);
    state.airborneBlend = approach(state.airborneBlend, airborneTarget, 8.0F, dt);
    state.crouchBlend = approach(state.crouchBlend, crouchTarget, 8.0F, dt);
    state.landBlend = approach(state.landBlend, landTarget, landTarget > 0.0F ? 16.0F : 7.0F, dt);
    state.turnLean = approach(state.turnLean, turnTarget, 8.0F, dt);

    state.previousMode = input.movementMode;
    state.previousVerticalVelocity = finiteOr(input.velocity.y);
    state.previousMantleProgress01 = clamp01(input.mantleProgress01);
    state.previousReloadProgress01 = clamp01(input.weapon.reloadProgress);
    state.previousReloading = input.weapon.reloading;
    state.previousAdsActive = adsActive;

    return evaluateCharacterAnimation(state, input);
}

CharacterAnimationFrame evaluateCharacterAnimation(
    const CharacterAnimationState& state,
    const CharacterAnimationInput& input) {
    CharacterAnimationFrame frame{};
    const float fireAlpha = clamp01(state.fireKickSeconds / kFireKickDurationSeconds);
    const bool hasUpdatedState = state.simulationTick > 0U;
    frame.locomotionClip = hasUpdatedState ? state.locomotionClip : chooseLocomotionClip(input, state);
    frame.upperBodyClip = hasUpdatedState ? state.upperBodyClip : chooseUpperBodyClip(input, fireAlpha);
    frame.previousLocomotionClip = hasUpdatedState ? state.previousLocomotionClip : frame.locomotionClip;
    frame.previousUpperBodyClip = hasUpdatedState ? state.previousUpperBodyClip : frame.upperBodyClip;
    frame.locomotionAlpha = locomotionAlphaFor(frame.locomotionClip, input);
    frame.upperBodyAlpha = upperBodyAlphaFor(frame.upperBodyClip, input, fireAlpha);
    frame.gaitPhase = state.locomotionPhase;
    frame.stride01 = clamp01(input.speed01);
    frame.idleBreath01 = (std::sin(state.idleBreathPhase) * 0.5F) + 0.5F;
    frame.adsAlpha = std::max(state.adsBlend, clamp01(input.weapon.adsAlpha));
    frame.reloadAlpha = state.reloadBlend;
    frame.fireAlpha = fireAlpha;
    frame.slideAlpha = state.slideBlend;
    frame.airborneAlpha = state.airborneBlend;
    frame.wallRunAlpha = state.wallRunBlend;
    frame.mantleAlpha = std::max(state.mantleBlend, clamp01(input.mantleProgress01));
    frame.crouchAlpha = state.crouchBlend;
    frame.landAlpha = state.landBlend;
    frame.jumpAlpha = input.movementMode == movement::MovementMode::Airborne ?
        clamp01((input.velocity.y + 0.35F) / 4.0F) : 0.0F;
    frame.fallAlpha = input.movementMode == movement::MovementMode::Airborne ?
        clamp01((-input.velocity.y + 0.35F) / 5.5F) : 0.0F;
    frame.energyPlatformAlpha = cueAlpha(
        input.movementTech.energyPlatformSeconds,
        movement::kEnergyPlatformCueSeconds);
    frame.gravityBootAlpha = std::max(
        cueAlpha(input.movementTech.gravityInverterGlowSeconds, movement::kGravityInverterGlowCueSeconds),
        input.movementTech.gravityInvertersActive ? 0.78F : 0.0F);
    frame.mantleReachAlpha = std::max(
        cueAlpha(input.movementTech.mantleReachSeconds, movement::kMantleReachCueSeconds),
        cueAlpha(input.movementTech.mantleClimbSeconds, movement::kMantleClimbCueSeconds));
    frame.locomotionTransitionAlpha = state.locomotionTransitionAlpha;
    frame.upperBodyTransitionAlpha = state.upperBodyTransitionAlpha;
    frame.turnLean = state.turnLean;
    frame.simulationTick = state.simulationTick;
    frame.eventCount = std::min(state.eventCount, frame.events.size());
    std::copy_n(state.events.begin(), frame.eventCount, frame.events.begin());

    if (frame.locomotionClip == CharacterAnimationClip::Mantle) {
        frame.locomotionNormalizedTime = clamp01(input.mantleProgress01);
    } else if (frame.locomotionClip == CharacterAnimationClip::Walk ||
        frame.locomotionClip == CharacterAnimationClip::Sprint ||
        frame.locomotionClip == CharacterAnimationClip::Crouch) {
        frame.locomotionNormalizedTime = wrap01(state.locomotionPhase / kTwoPi);
    } else {
        frame.locomotionNormalizedTime = normalizedClipTime(frame.locomotionClip, state.locomotionClipSeconds);
    }
    if (frame.upperBodyClip == CharacterAnimationClip::Reload) {
        frame.upperBodyNormalizedTime = clamp01(input.weapon.reloadProgress);
    } else if (frame.upperBodyClip == CharacterAnimationClip::Fire) {
        frame.upperBodyNormalizedTime = 1.0F - fireAlpha;
    } else {
        frame.upperBodyNormalizedTime = normalizedClipTime(frame.upperBodyClip, state.upperBodyClipSeconds);
    }
    frame.locomotionPhase = playbackPhase(frame.locomotionClip, frame.locomotionNormalizedTime);
    frame.upperBodyPhase = playbackPhase(frame.upperBodyClip, frame.upperBodyNormalizedTime);

    const novacore::math::Vec3 movementDirection = normalizedHorizontal(input.velocity, input.facingForward);
    const novacore::math::Vec3 facingForward = normalizedHorizontal(input.facingForward, {0.0F, 0.0F, 1.0F});
    const novacore::math::Vec3 facingRight = normalizedHorizontal(input.facingRight, {1.0F, 0.0F, 0.0F});
    if (horizontalSpeed(input.velocity) > 0.001F) {
        frame.locomotionForward = std::clamp(dotHorizontal(movementDirection, facingForward), -1.0F, 1.0F);
        frame.locomotionRight = std::clamp(dotHorizontal(movementDirection, facingRight), -1.0F, 1.0F);
        frame.movementAngleDegrees = std::atan2(frame.locomotionRight, frame.locomotionForward) * kRadiansToDegrees;
    }

    appendAdditiveLayer(
        frame,
        CharacterAnimationClip::Ads,
        CharacterPoseLayer::UpperBody,
        frame.adsAlpha,
        frame.adsAlpha);
    appendAdditiveLayer(
        frame,
        CharacterAnimationClip::Fire,
        CharacterPoseLayer::Arms,
        frame.fireAlpha,
        1.0F - frame.fireAlpha);
    appendAdditiveLayer(
        frame,
        CharacterAnimationClip::Reload,
        CharacterPoseLayer::UpperBody,
        frame.reloadAlpha,
        clamp01(input.weapon.reloadProgress));

    const float gaitSin = std::sin(frame.gaitPhase);
    const float gaitCos = std::cos(frame.gaitPhase);
    const float groundedStride = isGroundedLike(input.movementMode) ? frame.stride01 : frame.stride01 * 0.35F;
    const float freeWeapon = 1.0F - (frame.adsAlpha * 0.78F);
    const float sprintAlpha = frame.locomotionClip == CharacterAnimationClip::Sprint ? frame.locomotionAlpha : 0.0F;
    const float reloadArc = normalizedReloadArc(input.weapon, frame.reloadAlpha);
    const float mantleEase = smoothstep01(input.mantleProgress01);
    const float wallRunLean = std::clamp(input.cameraRollDegrees / 14.0F, -1.0F, 1.0F) * frame.wallRunAlpha;
    const float crouch = frame.crouchAlpha;
    const float land = frame.landAlpha;
    frame.wallRunLean = wallRunLean;

    frame.firstPersonWeaponOffset =
        makeVec(gaitSin * 0.012F * groundedStride, gaitCos * 0.010F * groundedStride, 0.0F) +
        makeVec(0.045F * sprintAlpha, -0.066F * sprintAlpha, -0.155F * sprintAlpha) * freeWeapon +
        makeVec(0.020F, -0.115F, -0.070F) * frame.slideAlpha * freeWeapon +
        makeVec(0.012F, -0.055F, -0.025F) * crouch * freeWeapon +
        makeVec(wallRunLean * 0.030F, -0.026F * frame.wallRunAlpha, -0.024F * frame.wallRunAlpha) * freeWeapon +
        makeVec(0.044F, -0.085F + (mantleEase * 0.145F), -0.110F) * frame.mantleAlpha * freeWeapon +
        makeVec(0.0F, -0.025F * land, 0.018F * land) +
        makeVec(0.0F, -0.120F * reloadArc, -0.018F * reloadArc) +
        makeVec(0.0F, 0.026F * frame.fireAlpha, -0.018F * frame.fireAlpha);

    frame.firstPersonArmsOffset =
        makeVec(gaitSin * 0.010F * groundedStride, gaitCos * 0.008F * groundedStride, 0.0F) +
        makeVec(0.038F * sprintAlpha, -0.056F * sprintAlpha, -0.100F * sprintAlpha) * freeWeapon +
        makeVec(0.020F, -0.090F, -0.052F) * frame.slideAlpha * freeWeapon +
        makeVec(0.010F, -0.045F, -0.020F) * crouch * freeWeapon +
        makeVec(wallRunLean * 0.024F, -0.020F * frame.wallRunAlpha, -0.020F * frame.wallRunAlpha) * freeWeapon +
        makeVec(0.024F, -0.035F + (mantleEase * 0.175F), -0.045F) * frame.mantleAlpha +
        makeVec(-0.048F * reloadArc, -0.110F * reloadArc, 0.0F) +
        makeVec(-0.040F * frame.energyPlatformAlpha, -0.035F * frame.energyPlatformAlpha, 0.030F * frame.energyPlatformAlpha);

    frame.firstPersonBodyOffset =
        makeVec(gaitSin * 0.012F * groundedStride, -0.020F * groundedStride, gaitCos * 0.008F * groundedStride) +
        makeVec(0.0F, -0.160F * frame.slideAlpha, -0.030F * frame.slideAlpha) +
        makeVec(0.0F, -0.090F * crouch, 0.025F * crouch) +
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
        makeVec(0.0F, -0.070F * crouch, 0.020F * crouch) +
        makeVec(0.0F, -0.035F * land, 0.0F) +
        makeVec(0.0F, 0.090F * frame.mantleAlpha, 0.0F);

    frame.firstPersonWeaponYawAddDegrees =
        (gaitSin * 0.32F * groundedStride * freeWeapon) +
        (wallRunLean * 1.5F) -
        (reloadArc * 1.8F) +
        (frame.turnLean * 0.8F);
    frame.firstPersonWeaponPitchAddDegrees =
        (-4.8F * sprintAlpha * freeWeapon) -
        (6.8F * frame.slideAlpha * freeWeapon) -
        (2.5F * crouch * freeWeapon) -
        (7.0F * frame.mantleAlpha * freeWeapon) +
        (2.0F * land) +
        (9.0F * reloadArc) +
        (input.weapon.recoilPitchOffsetDegrees * 0.34F) -
        (frame.fireAlpha * 1.8F);
    frame.firstPersonWeaponRollAddDegrees =
        (gaitSin * 1.1F * groundedStride * freeWeapon) +
        (4.4F * sprintAlpha * freeWeapon) -
        (7.2F * frame.slideAlpha * freeWeapon) +
        (wallRunLean * 5.4F) -
        (8.5F * reloadArc) +
        (frame.energyPlatformAlpha * 2.0F) +
        (frame.turnLean * 1.5F);

    frame.firstPersonArmsYawAddDegrees =
        (gaitSin * 0.22F * groundedStride) +
        (wallRunLean * 1.2F) -
        (reloadArc * 1.3F) +
        (frame.turnLean * 0.6F);
    frame.firstPersonArmsPitchAddDegrees =
        (-3.5F * sprintAlpha * freeWeapon) -
        (5.2F * frame.slideAlpha * freeWeapon) -
        (2.0F * crouch * freeWeapon) -
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
        (frame.energyPlatformAlpha * 5.0F) +
        (frame.turnLean * 1.2F);

    frame.firstPersonBodyPitchDegrees =
        (-6.0F * frame.slideAlpha) +
        (-3.0F * crouch) +
        (4.0F * frame.mantleAlpha) -
        (2.0F * frame.airborneAlpha) +
        (3.0F * land);
    frame.firstPersonBodyRollDegrees =
        (wallRunLean * 5.0F) +
        (gaitSin * 0.6F * groundedStride) +
        (frame.turnLean * 2.0F);
    frame.thirdPersonBodyPitchDegrees =
        (-10.0F * frame.slideAlpha) +
        (-5.0F * crouch) +
        (7.0F * frame.mantleAlpha) -
        (2.5F * frame.airborneAlpha) +
        (5.0F * land);
    frame.thirdPersonBodyRollDegrees =
        (wallRunLean * 8.0F) +
        (gaitSin * 1.2F * groundedStride) +
        (frame.turnLean * 4.0F);

    buildSkeletonPose(frame, input);
    return frame;
}

std::string_view characterAnimationClipName(CharacterAnimationClip clip) {
    switch (clip) {
    case CharacterAnimationClip::Idle: return "idle";
    case CharacterAnimationClip::Walk: return "walk";
    case CharacterAnimationClip::Sprint: return "sprint";
    case CharacterAnimationClip::Slide: return "slide";
    case CharacterAnimationClip::Airborne: return "airborne";
    case CharacterAnimationClip::WallRun: return "wallrun";
    case CharacterAnimationClip::Mantle: return "mantle";
    case CharacterAnimationClip::Reload: return "reload";
    case CharacterAnimationClip::Ads: return "ads";
    case CharacterAnimationClip::Fire: return "fire";
    case CharacterAnimationClip::Crouch: return "crouch";
    case CharacterAnimationClip::Jump: return "jump";
    case CharacterAnimationClip::Fall: return "fall";
    case CharacterAnimationClip::Land: return "land";
    }
    return "unknown";
}

std::string_view characterAnimationEventName(CharacterAnimationEvent event) {
    switch (event) {
    case CharacterAnimationEvent::FootstepLeft: return "footstep_left";
    case CharacterAnimationEvent::FootstepRight: return "footstep_right";
    case CharacterAnimationEvent::JumpStart: return "jump_start";
    case CharacterAnimationEvent::Apex: return "apex";
    case CharacterAnimationEvent::Land: return "land";
    case CharacterAnimationEvent::SlideStart: return "slide_start";
    case CharacterAnimationEvent::WallRunStart: return "wallrun_start";
    case CharacterAnimationEvent::WallRunEnd: return "wallrun_end";
    case CharacterAnimationEvent::MantleReach: return "mantle_reach";
    case CharacterAnimationEvent::MantlePull: return "mantle_pull";
    case CharacterAnimationEvent::MantleComplete: return "mantle_complete";
    case CharacterAnimationEvent::AdsEnter: return "ads_enter";
    case CharacterAnimationEvent::AdsExit: return "ads_exit";
    case CharacterAnimationEvent::Fire: return "fire";
    case CharacterAnimationEvent::ReloadStart: return "reload_start";
    case CharacterAnimationEvent::ReloadInsert: return "reload_insert";
    case CharacterAnimationEvent::ReloadComplete: return "reload_complete";
    }
    return "unknown";
}

std::string_view characterBoneName(CharacterBone bone) {
    switch (bone) {
    case CharacterBone::Root: return "root";
    case CharacterBone::Pelvis: return "pelvis";
    case CharacterBone::SpineLower: return "spine_lower";
    case CharacterBone::SpineUpper: return "spine_upper";
    case CharacterBone::Chest: return "chest";
    case CharacterBone::Neck: return "neck";
    case CharacterBone::Head: return "head";
    case CharacterBone::ClavicleLeft: return "clavicle_l";
    case CharacterBone::UpperArmLeft: return "upper_arm_l";
    case CharacterBone::ForearmLeft: return "forearm_l";
    case CharacterBone::HandLeft: return "hand_l";
    case CharacterBone::ClavicleRight: return "clavicle_r";
    case CharacterBone::UpperArmRight: return "upper_arm_r";
    case CharacterBone::ForearmRight: return "forearm_r";
    case CharacterBone::HandRight: return "hand_r";
    case CharacterBone::ThighLeft: return "thigh_l";
    case CharacterBone::CalfLeft: return "calf_l";
    case CharacterBone::FootLeft: return "foot_l";
    case CharacterBone::ThighRight: return "thigh_r";
    case CharacterBone::CalfRight: return "calf_r";
    case CharacterBone::FootRight: return "foot_r";
    case CharacterBone::Count:
    case CharacterBone::Invalid:
        break;
    }
    return "unknown";
}

std::string_view characterSocketName(CharacterSocket socket) {
    switch (socket) {
    case CharacterSocket::Camera: return "camera";
    case CharacterSocket::WeaponGrip: return "weapon_grip";
    case CharacterSocket::SupportHand: return "support_hand";
    case CharacterSocket::Muzzle: return "muzzle";
    case CharacterSocket::FootLeft: return "foot_l";
    case CharacterSocket::FootRight: return "foot_r";
    case CharacterSocket::Count: break;
    }
    return "unknown";
}

const CharacterBonePose* findCharacterBonePose(
    const CharacterSkeletonPose& pose,
    CharacterBone bone) {
    const std::size_t index = boneIndex(bone);
    return index < pose.bones.size() ? &pose.bones[index] : nullptr;
}

const CharacterSocketPose* findCharacterSocketPose(
    const CharacterSkeletonPose& pose,
    CharacterSocket socket) {
    const std::size_t index = socketIndex(socket);
    return index < pose.sockets.size() ? &pose.sockets[index] : nullptr;
}

bool hasCharacterAnimationEvent(
    const CharacterAnimationFrame& frame,
    CharacterAnimationEvent event) {
    const std::size_t count = std::min(frame.eventCount, frame.events.size());
    for (std::size_t index = 0U; index < count; ++index) {
        if (frame.events[index].event == event) {
            return true;
        }
    }
    return false;
}

} // namespace nemisis::player
