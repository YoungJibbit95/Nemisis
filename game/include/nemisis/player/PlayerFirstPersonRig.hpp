#pragma once

#include "nemisis/movement/MovementSystem.hpp"
#include "nemisis/player/PlayerAnimation.hpp"
#include "nemisis/player/PlayerComponents.hpp"
#include "nemisis/weapons/WeaponTypes.hpp"

#include "novacore/math/Types.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace nemisis::player {

enum class FirstPersonRigJoint : std::uint8_t {
    Root,
    Pelvis,
    Spine,
    Chest,
    Neck,
    Head,
    RightClavicle,
    RightUpperArm,
    RightForearm,
    RightHand,
    LeftClavicle,
    LeftUpperArm,
    LeftForearm,
    LeftHand,
    WeaponRoot,
    Muzzle,
    Count
};

inline constexpr std::size_t kFirstPersonRigJointCount =
    static_cast<std::size_t>(FirstPersonRigJoint::Count);

struct FirstPersonRigMountDesc final {
    novacore::math::Vec3 hipOffset{};
    novacore::math::Vec3 adsOffset{};
    novacore::math::Vec3 scale{1.0F, 1.0F, 1.0F};
    float yawCorrectionDegrees = 0.0F;
    float pitchCorrectionDegrees = 0.0F;
    float rollCorrectionDegrees = 0.0F;
    float hipPitchFollow = 0.42F;
    float adsPitchFollow = 0.82F;
    float recoilYawScale = 0.50F;
    float recoilPitchScale = 0.38F;
    float reloadPitchDegrees = 8.0F;
    float reloadRollDegrees = 8.0F;
};

struct FirstPersonRigInput final {
    novacore::math::Vec3 cameraPosition{};
    PlayerViewComponent view{};
    CharacterAnimationFrame animation{};
    weapons::WeaponRuntimeState weapon{};
    weapons::WeaponClass weaponClass = weapons::WeaponClass::AssaultRifle;
    movement::MovementMode movementMode = movement::MovementMode::Grounded;
    novacore::math::Vec3 headBobOffset{};
    novacore::math::Vec3 weaponSwayOffset{};
    FirstPersonRigMountDesc weaponMount{};
    FirstPersonRigMountDesc armsMount{};
    FirstPersonRigMountDesc bodyMount{};
    float cameraRollDegrees = 0.0F;
    float adsAlpha = 0.0F;
    float speed01 = 0.0F;
    float mantleProgress01 = 0.0F;
    bool hasWallRunContact = false;
    bool hasAnimationFrame = false;
};

struct FirstPersonJointPose final {
    FirstPersonRigJoint joint = FirstPersonRigJoint::Root;
    FirstPersonRigJoint parent = FirstPersonRigJoint::Count;
    novacore::math::Vec3 localPosition{};
    novacore::math::Vec3 worldPosition{};
    float yawDegrees = 0.0F;
    float pitchDegrees = 0.0F;
    float rollDegrees = 0.0F;
};

struct FirstPersonRigAttachment final {
    novacore::math::Vec3 position{};
    novacore::math::Vec3 scale{1.0F, 1.0F, 1.0F};
    float yawDegrees = 0.0F;
    float pitchDegrees = 0.0F;
    float rollDegrees = 0.0F;
    float alpha = 1.0F;
    bool visible = true;
};

struct FirstPersonRigFrame final {
    std::array<FirstPersonJointPose, kFirstPersonRigJointCount> joints{};
    FirstPersonRigAttachment weapon{};
    FirstPersonRigAttachment arms{};
    FirstPersonRigAttachment body{};
    FirstPersonRigAttachment rightHand{};
    FirstPersonRigAttachment leftHand{};
    FirstPersonRigAttachment supportElbow{};
    FirstPersonRigAttachment muzzle{};
    float lookDownBodyAlpha = 0.0F;
    float reloadArc = 0.0F;
    float wallRunRollDegrees = 0.0F;
};

[[nodiscard]] std::string_view firstPersonRigJointName(FirstPersonRigJoint joint);

[[nodiscard]] const FirstPersonJointPose& firstPersonRigJoint(
    const FirstPersonRigFrame& frame,
    FirstPersonRigJoint joint);

[[nodiscard]] FirstPersonRigFrame evaluateFirstPersonRig(const FirstPersonRigInput& input);

} // namespace nemisis::player
