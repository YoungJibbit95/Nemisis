#pragma once

#include "nemisis/movement/MovementSystem.hpp"
#include "nemisis/weapons/WeaponTypes.hpp"

#include "novacore/math/Types.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace nemisis::player {

enum class CharacterAnimationClip {
    Idle,
    Walk,
    Sprint,
    Slide,
    Airborne,
    WallRun,
    Mantle,
    Reload,
    Ads,
    Fire,
    Crouch,
    Jump,
    Fall,
    Land
};

enum class CharacterClipPhase {
    Enter,
    Loop,
    Exit
};

enum class CharacterAnimationEvent {
    FootstepLeft,
    FootstepRight,
    JumpStart,
    Apex,
    Land,
    SlideStart,
    WallRunStart,
    WallRunEnd,
    MantleReach,
    MantlePull,
    MantleComplete,
    AdsEnter,
    AdsExit,
    Fire,
    ReloadStart,
    ReloadInsert,
    ReloadComplete
};

enum class CharacterBone : std::uint8_t {
    Root,
    Pelvis,
    SpineLower,
    SpineUpper,
    Chest,
    Neck,
    Head,
    ClavicleLeft,
    UpperArmLeft,
    ForearmLeft,
    HandLeft,
    ClavicleRight,
    UpperArmRight,
    ForearmRight,
    HandRight,
    ThighLeft,
    CalfLeft,
    FootLeft,
    ThighRight,
    CalfRight,
    FootRight,
    Count,
    Invalid = 0xFFU
};

enum class CharacterSocket : std::uint8_t {
    Camera,
    WeaponGrip,
    SupportHand,
    Muzzle,
    FootLeft,
    FootRight,
    Count
};

enum class CharacterPoseLayer : std::uint8_t {
    Base,
    UpperBody,
    Arms,
    FullBody
};

inline constexpr std::size_t kCharacterBoneCount = static_cast<std::size_t>(CharacterBone::Count);
inline constexpr std::size_t kCharacterSocketCount = static_cast<std::size_t>(CharacterSocket::Count);
inline constexpr std::size_t kMaxCharacterAnimationEvents = 16U;
inline constexpr std::size_t kMaxCharacterAdditiveLayers = 3U;

struct CharacterPoseTransform final {
    novacore::math::Vec3 translation{};
    novacore::math::Quat rotation{};
    novacore::math::Vec3 scale{1.0F, 1.0F, 1.0F};
};

struct CharacterBonePose final {
    CharacterBone bone = CharacterBone::Root;
    CharacterBone parent = CharacterBone::Invalid;
    CharacterPoseTransform local{};
};

struct CharacterSocketPose final {
    CharacterSocket socket = CharacterSocket::Camera;
    CharacterBone parentBone = CharacterBone::Head;
    CharacterPoseTransform local{};
};

struct CharacterSkeletonPose final {
    std::array<CharacterBonePose, kCharacterBoneCount> bones{};
    std::array<CharacterSocketPose, kCharacterSocketCount> sockets{};
};

struct CharacterAnimationEventMarker final {
    CharacterAnimationEvent event = CharacterAnimationEvent::FootstepLeft;
    float normalizedTime = 0.0F;
    float weight = 1.0F;
    std::uint64_t simulationTick = 0U;
};

struct CharacterAdditiveLayer final {
    CharacterAnimationClip clip = CharacterAnimationClip::Idle;
    CharacterPoseLayer layer = CharacterPoseLayer::UpperBody;
    float weight = 0.0F;
    float normalizedTime = 0.0F;
};

struct CharacterAnimationInput final {
    novacore::math::Vec3 velocity{};
    movement::MovementMode movementMode = movement::MovementMode::Grounded;
    movement::MovementTechState movementTech{};
    weapons::WeaponRuntimeState weapon{};
    float speed01 = 0.0F;
    float cameraRollDegrees = 0.0F;
    float mantleProgress01 = 0.0F;
    float fixedDeltaSeconds = 1.0F / 60.0F;
    bool hasWallRunContact = false;
    bool adsHeld = false;
    bool sprintHeld = false;
    bool tacticalSprintHeld = false;
    bool crouchHeld = false;

    // World-space orientation inputs used to resolve local locomotion direction.
    novacore::math::Vec3 facingForward{0.0F, 0.0F, 1.0F};
    novacore::math::Vec3 facingRight{1.0F, 0.0F, 0.0F};
    float turnDeltaDegrees = 0.0F;
    float aimPitchDegrees = 0.0F;
    float aimYawDegrees = 0.0F;
};

struct CharacterAnimationState final {
    float locomotionPhase = 0.0F;
    float idleBreathPhase = 0.0F;
    float upperBodyBlend = 0.0F;
    float fireKickSeconds = 0.0F;
    float reloadBlend = 0.0F;
    float adsBlend = 0.0F;
    float wallRunBlend = 0.0F;
    float mantleBlend = 0.0F;
    float slideBlend = 0.0F;
    float airborneBlend = 0.0F;
    std::uint32_t observedShotIndex = 0;
    movement::MovementMode previousMode = movement::MovementMode::Grounded;

    CharacterAnimationClip locomotionClip = CharacterAnimationClip::Idle;
    CharacterAnimationClip previousLocomotionClip = CharacterAnimationClip::Idle;
    CharacterAnimationClip upperBodyClip = CharacterAnimationClip::Idle;
    CharacterAnimationClip previousUpperBodyClip = CharacterAnimationClip::Idle;
    float locomotionClipSeconds = 0.0F;
    float upperBodyClipSeconds = 0.0F;
    float locomotionTransitionAlpha = 1.0F;
    float upperBodyTransitionAlpha = 1.0F;
    float landSeconds = 0.0F;
    float landBlend = 0.0F;
    float crouchBlend = 0.0F;
    float turnLean = 0.0F;
    float previousVerticalVelocity = 0.0F;
    float previousMantleProgress01 = 0.0F;
    float previousReloadProgress01 = 0.0F;
    bool previousReloading = false;
    bool previousAdsActive = false;
    std::uint64_t simulationTick = 0U;
    std::array<CharacterAnimationEventMarker, kMaxCharacterAnimationEvents> events{};
    std::size_t eventCount = 0U;
};

struct CharacterAnimationFrame final {
    CharacterAnimationClip locomotionClip = CharacterAnimationClip::Idle;
    CharacterAnimationClip upperBodyClip = CharacterAnimationClip::Idle;
    CharacterAnimationClip previousLocomotionClip = CharacterAnimationClip::Idle;
    CharacterAnimationClip previousUpperBodyClip = CharacterAnimationClip::Idle;

    float locomotionAlpha = 0.0F;
    float upperBodyAlpha = 0.0F;
    float gaitPhase = 0.0F;
    float stride01 = 0.0F;
    float idleBreath01 = 0.0F;
    float adsAlpha = 0.0F;
    float reloadAlpha = 0.0F;
    float fireAlpha = 0.0F;
    float slideAlpha = 0.0F;
    float airborneAlpha = 0.0F;
    float wallRunAlpha = 0.0F;
    float mantleAlpha = 0.0F;
    float energyPlatformAlpha = 0.0F;
    float gravityBootAlpha = 0.0F;
    float mantleReachAlpha = 0.0F;
    float wallRunLean = 0.0F;
    float locomotionNormalizedTime = 0.0F;
    float upperBodyNormalizedTime = 0.0F;
    float locomotionTransitionAlpha = 1.0F;
    float upperBodyTransitionAlpha = 1.0F;
    float locomotionForward = 0.0F;
    float locomotionRight = 0.0F;
    float movementAngleDegrees = 0.0F;
    float turnLean = 0.0F;
    float jumpAlpha = 0.0F;
    float fallAlpha = 0.0F;
    float landAlpha = 0.0F;
    float crouchAlpha = 0.0F;

    CharacterClipPhase locomotionPhase = CharacterClipPhase::Loop;
    CharacterClipPhase upperBodyPhase = CharacterClipPhase::Loop;
    std::array<CharacterAdditiveLayer, kMaxCharacterAdditiveLayers> additiveLayers{};
    std::size_t additiveLayerCount = 0U;
    std::array<CharacterAnimationEventMarker, kMaxCharacterAnimationEvents> events{};
    std::size_t eventCount = 0U;
    std::uint64_t simulationTick = 0U;
    CharacterSkeletonPose pose{};

    novacore::math::Vec3 firstPersonWeaponOffset{};
    novacore::math::Vec3 firstPersonArmsOffset{};
    novacore::math::Vec3 firstPersonBodyOffset{};
    novacore::math::Vec3 rightHandLocalOffset{};
    novacore::math::Vec3 leftHandLocalOffset{};
    novacore::math::Vec3 supportElbowLocalOffset{};
    novacore::math::Vec3 thirdPersonBodyOffset{};

    float firstPersonWeaponYawAddDegrees = 0.0F;
    float firstPersonWeaponPitchAddDegrees = 0.0F;
    float firstPersonWeaponRollAddDegrees = 0.0F;
    float firstPersonArmsYawAddDegrees = 0.0F;
    float firstPersonArmsPitchAddDegrees = 0.0F;
    float firstPersonArmsRollAddDegrees = 0.0F;
    float firstPersonBodyPitchDegrees = 0.0F;
    float firstPersonBodyRollDegrees = 0.0F;
    float thirdPersonBodyPitchDegrees = 0.0F;
    float thirdPersonBodyRollDegrees = 0.0F;
};

void resetCharacterAnimation(CharacterAnimationState& state);

[[nodiscard]] CharacterAnimationFrame updateCharacterAnimation(
    CharacterAnimationState& state,
    const CharacterAnimationInput& input);

[[nodiscard]] CharacterAnimationFrame evaluateCharacterAnimation(
    const CharacterAnimationState& state,
    const CharacterAnimationInput& input);

[[nodiscard]] std::string_view characterAnimationClipName(CharacterAnimationClip clip);

[[nodiscard]] std::string_view characterAnimationEventName(CharacterAnimationEvent event);

[[nodiscard]] std::string_view characterBoneName(CharacterBone bone);

[[nodiscard]] std::string_view characterSocketName(CharacterSocket socket);

[[nodiscard]] const CharacterBonePose* findCharacterBonePose(
    const CharacterSkeletonPose& pose,
    CharacterBone bone);

[[nodiscard]] const CharacterSocketPose* findCharacterSocketPose(
    const CharacterSkeletonPose& pose,
    CharacterSocket socket);

[[nodiscard]] bool hasCharacterAnimationEvent(
    const CharacterAnimationFrame& frame,
    CharacterAnimationEvent event);

} // namespace nemisis::player
