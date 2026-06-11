#pragma once

#include "nemisis/movement/MovementSystem.hpp"
#include "nemisis/player/PlayerComponents.hpp"
#include "nemisis/weapons/WeaponTypes.hpp"

#include "novacore/math/Types.hpp"

namespace nemisis::player {

struct CameraRigTuning final {
    float eyeHeight = 1.65F;
    float crouchEyeHeight = 1.18F;
    float positionSharpness = 18.0F;
    float viewSharpness = 24.0F;
    float fovSharpness = 10.0F;
    float baseVerticalFovDegrees = 74.0F;
    float sprintFovKickDegrees = 5.0F;
    float tacticalSprintFovKickDegrees = 7.0F;
    float slideFovKickDegrees = 3.5F;
    float dashFovKickDegrees = 4.0F;
    float airborneFovKickDegrees = 1.25F;
    float recoilPitchVisualScale = 0.72F;
    float recoilYawVisualScale = 0.55F;
    float velocitySwayDegrees = 0.45F;
    float weaponSwayScale = 0.035F;
    float bobWalkAmplitude = 0.035F;
    float bobSprintAmplitude = 0.060F;
    float bobSlideAmplitude = 0.025F;
    float bobWalkFrequency = 7.5F;
    float bobSprintFrequency = 10.5F;
    float slideRollDegrees = -4.5F;
    float dashRollDegrees = 2.5F;
    float maxVisualPitchDegrees = 89.0F;
};

struct CameraRigInput final {
    novacore::math::Vec3 playerPosition{};
    novacore::math::Vec3 playerVelocity{};
    movement::MovementMode movementMode = movement::MovementMode::Grounded;
    PlayerViewComponent authoredView{};
    weapons::WeaponRuntimeState weapon{};
    bool adsHeld = false;
    bool sprintHeld = false;
    bool tacticalSprintHeld = false;
    bool crouchHeld = false;
    float fixedDeltaSeconds = 1.0F / 60.0F;
};

struct CameraRigFrame final {
    novacore::math::Vec3 position{};
    PlayerViewComponent view{};
    novacore::math::Vec3 headBobOffset{};
    novacore::math::Vec3 weaponSwayOffset{};
    float rollDegrees = 0.0F;
    float verticalFovDegrees = 74.0F;
    float speed01 = 0.0F;
    float adsAlpha = 0.0F;
};

struct CameraRigState final {
    bool initialized = false;
    novacore::math::Vec3 position{};
    PlayerViewComponent view{};
    novacore::math::Vec3 headBobOffset{};
    novacore::math::Vec3 weaponSwayOffset{};
    float bobPhase = 0.0F;
    float rollDegrees = 0.0F;
    float verticalFovDegrees = 74.0F;
    float speed01 = 0.0F;
    float adsAlpha = 0.0F;
};

void resetCameraRig(CameraRigState& state);

[[nodiscard]] float cameraRigHorizontalSpeed(novacore::math::Vec3 velocity);

[[nodiscard]] CameraRigFrame updateCameraRig(
    CameraRigState& state,
    const CameraRigInput& input,
    const CameraRigTuning& tuning = {});

} // namespace nemisis::player
