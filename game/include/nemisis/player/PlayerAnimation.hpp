#pragma once

#include "nemisis/movement/MovementSystem.hpp"
#include "nemisis/weapons/WeaponTypes.hpp"

#include "novacore/math/Types.hpp"

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
    Fire
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
};

struct CharacterAnimationFrame final {
    CharacterAnimationClip locomotionClip = CharacterAnimationClip::Idle;
    CharacterAnimationClip upperBodyClip = CharacterAnimationClip::Idle;

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

} // namespace nemisis::player
