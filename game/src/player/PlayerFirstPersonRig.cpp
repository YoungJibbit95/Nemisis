#include "nemisis/player/PlayerFirstPersonRig.hpp"

#include <algorithm>
#include <cmath>

namespace nemisis::player {

namespace {

constexpr float kPi = 3.14159265358979323846F;

struct ViewBasis final {
    novacore::math::Vec3 right{1.0F, 0.0F, 0.0F};
    novacore::math::Vec3 up{0.0F, 1.0F, 0.0F};
    novacore::math::Vec3 forward{0.0F, 0.0F, 1.0F};
};

[[nodiscard]] float clamp01(float value) {
    return std::clamp(value, 0.0F, 1.0F);
}

[[nodiscard]] float degreesToRadians(float degrees) {
    return degrees * (kPi / 180.0F);
}

[[nodiscard]] novacore::math::Vec3 makeVec(float x, float y, float z) {
    return novacore::math::Vec3{x, y, z};
}

[[nodiscard]] novacore::math::Vec3 lerpVec3(
    novacore::math::Vec3 a,
    novacore::math::Vec3 b,
    float t) {
    return a + ((b - a) * t);
}

[[nodiscard]] novacore::math::Vec3 mulVec3(
    novacore::math::Vec3 a,
    novacore::math::Vec3 b) {
    return {a.x * b.x, a.y * b.y, a.z * b.z};
}

[[nodiscard]] novacore::math::Vec3 normalized(novacore::math::Vec3 value) {
    const float lengthSquared = value.lengthSquared();
    if (lengthSquared <= 0.000001F) {
        return {0.0F, 0.0F, 1.0F};
    }

    const float invLength = 1.0F / std::sqrt(lengthSquared);
    return value * invLength;
}

[[nodiscard]] novacore::math::Vec3 rotateZ(novacore::math::Vec3 value, float rollDegrees) {
    const float roll = degreesToRadians(rollDegrees);
    const float sine = std::sin(roll);
    const float cosine = std::cos(roll);
    return {
        (value.x * cosine) - (value.y * sine),
        (value.x * sine) + (value.y * cosine),
        value.z,
    };
}

[[nodiscard]] novacore::math::Vec3 rotateX(novacore::math::Vec3 value, float pitchDegrees) {
    const float pitch = degreesToRadians(pitchDegrees);
    const float sine = std::sin(pitch);
    const float cosine = std::cos(pitch);
    return {
        value.x,
        (value.y * cosine) - (value.z * sine),
        (value.y * sine) + (value.z * cosine),
    };
}

[[nodiscard]] novacore::math::Vec3 rotateY(novacore::math::Vec3 value, float yawDegrees) {
    const float yaw = degreesToRadians(yawDegrees);
    const float sine = std::sin(yaw);
    const float cosine = std::cos(yaw);
    return {
        (value.x * cosine) + (value.z * sine),
        value.y,
        (-value.x * sine) + (value.z * cosine),
    };
}

[[nodiscard]] novacore::math::Vec3 rotateMeshLocal(
    novacore::math::Vec3 value,
    float yawDegrees,
    float pitchDegrees,
    float rollDegrees) {
    return rotateY(rotateX(rotateZ(value, rollDegrees), pitchDegrees), yawDegrees);
}

[[nodiscard]] ViewBasis basisFromMeshRotation(
    float yawDegrees,
    float pitchDegrees,
    float rollDegrees) {
    return ViewBasis{
        normalized(rotateMeshLocal({1.0F, 0.0F, 0.0F}, yawDegrees, pitchDegrees, rollDegrees)),
        normalized(rotateMeshLocal({0.0F, 1.0F, 0.0F}, yawDegrees, pitchDegrees, rollDegrees)),
        normalized(rotateMeshLocal({0.0F, 0.0F, 1.0F}, yawDegrees, pitchDegrees, rollDegrees)),
    };
}

[[nodiscard]] ViewBasis basisFromView(PlayerViewComponent view, float rollDegrees) {
    // RenderMesh3D pitch rotates in the opposite direction to camera pitch.
    return basisFromMeshRotation(view.yawDegrees, -view.pitchDegrees, rollDegrees);
}

[[nodiscard]] novacore::math::Vec3 toWorld(
    const ViewBasis& basis,
    novacore::math::Vec3 local) {
    return (basis.right * local.x) + (basis.up * local.y) + (basis.forward * local.z);
}

[[nodiscard]] float normalizedReloadArc(const weapons::WeaponRuntimeState& weapon, float animationReloadAlpha) {
    const float progress = weapon.reloading ? clamp01(weapon.reloadProgress) : 0.0F;
    const float proceduralArc = weapon.reloading ? std::sin(progress * kPi) : 0.0F;
    return std::max(proceduralArc, std::sin(clamp01(animationReloadAlpha) * kPi) * clamp01(animationReloadAlpha));
}

[[nodiscard]] bool isSidearm(weapons::WeaponClass weaponClass) {
    return weaponClass == weapons::WeaponClass::Sidearm;
}

[[nodiscard]] float bodyLookDownAlpha(float pitchDegrees) {
    return clamp01((-pitchDegrees - 12.0F) / 42.0F);
}

[[nodiscard]] float bodyPitchForView(float pitchDegrees, const CharacterAnimationFrame& animation) {
    return animation.firstPersonBodyPitchDegrees + std::clamp(pitchDegrees * 0.12F, -10.0F, 6.0F);
}

[[nodiscard]] float bodyRollForView(float cameraRollDegrees, const CharacterAnimationFrame& animation) {
    return animation.firstPersonBodyRollDegrees + (cameraRollDegrees * 0.10F);
}

[[nodiscard]] FirstPersonRigAttachment makeAttachment(
    novacore::math::Vec3 position,
    novacore::math::Vec3 scale,
    float yawDegrees,
    float pitchDegrees,
    float rollDegrees,
    float alpha = 1.0F) {
    return FirstPersonRigAttachment{
        position,
        scale,
        yawDegrees,
        pitchDegrees,
        rollDegrees,
        alpha,
        alpha > 0.001F,
    };
}

void setJoint(
    FirstPersonRigFrame& frame,
    FirstPersonRigJoint joint,
    FirstPersonRigJoint parent,
    novacore::math::Vec3 localPosition,
    novacore::math::Vec3 worldPosition,
    float yawDegrees,
    float pitchDegrees,
    float rollDegrees) {
    frame.joints[static_cast<std::size_t>(joint)] = FirstPersonJointPose{
        joint,
        parent,
        localPosition,
        worldPosition,
        yawDegrees,
        pitchDegrees,
        rollDegrees,
    };
}

void setSocket(
    FirstPersonRigFrame& frame,
    FirstPersonRigSocket socket,
    FirstPersonRigJoint joint,
    novacore::math::Vec3 localPosition,
    novacore::math::Vec3 worldPosition,
    float yawDegrees,
    float pitchDegrees,
    float rollDegrees) {
    frame.sockets[static_cast<std::size_t>(socket)] = FirstPersonRigSocketPose{
        socket,
        joint,
        localPosition,
        worldPosition,
        yawDegrees,
        pitchDegrees,
        rollDegrees,
        true,
    };
}

void buildBodyJoints(
    FirstPersonRigFrame& frame,
    const FirstPersonRigInput& input,
    const ViewBasis& bodyBasis,
    novacore::math::Vec3 bodyRoot,
    float bodyPitch,
    float bodyRoll) {
    const float slide = clamp01(input.animation.slideAlpha);
    const float mantle = clamp01(input.animation.mantleAlpha);
    const float airborne = clamp01(input.animation.airborneAlpha);
    const float crouchDrop = input.movementMode == movement::MovementMode::Sliding ? 0.12F : 0.0F;

    const auto pelvisLocal = makeVec(0.0F, 0.18F - crouchDrop, -0.04F);
    const auto spineLocal = makeVec(0.0F, 0.42F - (slide * 0.06F), 0.025F);
    const auto chestLocal = makeVec(0.0F, 0.34F + (mantle * 0.04F), 0.035F - (airborne * 0.025F));
    const auto neckLocal = makeVec(0.0F, 0.24F, 0.018F);
    const auto headLocal = makeVec(0.0F, 0.15F, 0.018F);

    const auto pelvisWorld = bodyRoot + toWorld(bodyBasis, pelvisLocal);
    const auto spineWorld = pelvisWorld + toWorld(bodyBasis, spineLocal);
    const auto chestWorld = spineWorld + toWorld(bodyBasis, chestLocal);
    const auto neckWorld = chestWorld + toWorld(bodyBasis, neckLocal);
    const auto headWorld = neckWorld + toWorld(bodyBasis, headLocal);

    setJoint(
        frame,
        FirstPersonRigJoint::Root,
        FirstPersonRigJoint::Count,
        {},
        input.cameraPosition,
        input.view.yawDegrees,
        input.view.pitchDegrees,
        input.cameraRollDegrees);
    setJoint(
        frame,
        FirstPersonRigJoint::Pelvis,
        FirstPersonRigJoint::Root,
        pelvisLocal,
        pelvisWorld,
        input.view.yawDegrees,
        bodyPitch,
        bodyRoll);
    setJoint(
        frame,
        FirstPersonRigJoint::Spine,
        FirstPersonRigJoint::Pelvis,
        spineLocal,
        spineWorld,
        input.view.yawDegrees,
        bodyPitch * 0.72F,
        bodyRoll * 0.72F);
    setJoint(
        frame,
        FirstPersonRigJoint::Chest,
        FirstPersonRigJoint::Spine,
        chestLocal,
        chestWorld,
        input.view.yawDegrees,
        bodyPitch * 0.58F,
        bodyRoll * 0.58F);
    setJoint(
        frame,
        FirstPersonRigJoint::Neck,
        FirstPersonRigJoint::Chest,
        neckLocal,
        neckWorld,
        input.view.yawDegrees,
        input.view.pitchDegrees * 0.22F,
        bodyRoll * 0.45F);
    setJoint(
        frame,
        FirstPersonRigJoint::Head,
        FirstPersonRigJoint::Neck,
        headLocal,
        headWorld,
        input.view.yawDegrees,
        input.view.pitchDegrees * 0.28F,
        bodyRoll * 0.40F);
}

void buildArmJoints(
    FirstPersonRigFrame& frame,
    const FirstPersonRigInput& input,
    const ViewBasis& armsBasis,
    novacore::math::Vec3 armsRoot,
    novacore::math::Vec3 rightGripWorld,
    novacore::math::Vec3 leftGripWorld,
    float armsYaw,
    float armsPitch,
    float armsRoll) {
    const auto& animation = input.animation;
    const float reload = frame.reloadArc;
    const bool sidearm = isSidearm(input.weaponClass);
    const auto rightHandLocal = input.weaponSockets.rightGrip;
    const auto leftHandLocal = input.weaponSockets.leftGrip;

    const auto rightClavicleLocal = makeVec(0.16F, -0.04F, 0.10F);
    const auto leftClavicleLocal = makeVec(-0.16F, -0.04F, 0.10F);
    const auto rightUpperArmLocal = makeVec(0.10F, -0.12F, 0.17F);
    const auto rightForearmLocal = makeVec(0.04F, -0.08F, 0.22F);
    const auto leftUpperArmLocal = makeVec(-0.13F, -0.12F, 0.17F);
    const auto leftForearmLocal = makeVec(-0.05F, -0.08F, 0.24F);

    const auto rightClavicleWorld = armsRoot + toWorld(armsBasis, rightClavicleLocal);
    const auto leftClavicleWorld = armsRoot + toWorld(armsBasis, leftClavicleLocal);
    const auto rightUpperArmWorld = rightClavicleWorld + toWorld(armsBasis, rightUpperArmLocal);
    const auto leftUpperArmWorld = leftClavicleWorld + toWorld(armsBasis, leftUpperArmLocal);
    const auto rightForearmWorld =
        rightUpperArmWorld +
        toWorld(armsBasis, rightForearmLocal + (animation.rightHandLocalOffset * 0.35F));
    const auto leftForearmWorld =
        leftUpperArmWorld +
        toWorld(armsBasis, leftForearmLocal + (animation.leftHandLocalOffset * 0.35F));
    const auto rightHandWorld = rightGripWorld;
    const auto leftHandWorld = leftGripWorld;
    const auto supportElbowBias =
        animation.supportElbowLocalOffset +
        makeVec(sidearm ? -0.01F : -0.03F, -0.025F - (reload * 0.02F), 0.02F * reload);
    const auto supportElbowWorld =
        ((leftForearmWorld + leftHandWorld) * 0.5F) +
        toWorld(armsBasis, supportElbowBias);

    setJoint(
        frame,
        FirstPersonRigJoint::RightClavicle,
        FirstPersonRigJoint::Chest,
        rightClavicleLocal,
        rightClavicleWorld,
        armsYaw,
        armsPitch * 0.42F,
        armsRoll * 0.30F);
    setJoint(
        frame,
        FirstPersonRigJoint::RightUpperArm,
        FirstPersonRigJoint::RightClavicle,
        rightUpperArmLocal,
        rightUpperArmWorld,
        armsYaw,
        armsPitch * 0.62F,
        armsRoll * 0.48F);
    setJoint(
        frame,
        FirstPersonRigJoint::RightForearm,
        FirstPersonRigJoint::RightUpperArm,
        rightForearmLocal,
        rightForearmWorld,
        armsYaw,
        armsPitch * 0.76F,
        armsRoll * 0.62F);
    setJoint(
        frame,
        FirstPersonRigJoint::RightHand,
        FirstPersonRigJoint::RightForearm,
        rightHandLocal,
        rightHandWorld,
        armsYaw,
        armsPitch,
        armsRoll);
    setJoint(
        frame,
        FirstPersonRigJoint::LeftClavicle,
        FirstPersonRigJoint::Chest,
        leftClavicleLocal,
        leftClavicleWorld,
        armsYaw,
        armsPitch * 0.42F,
        armsRoll * 0.30F);
    setJoint(
        frame,
        FirstPersonRigJoint::LeftUpperArm,
        FirstPersonRigJoint::LeftClavicle,
        leftUpperArmLocal,
        leftUpperArmWorld,
        armsYaw,
        armsPitch * 0.62F,
        armsRoll * 0.48F);
    setJoint(
        frame,
        FirstPersonRigJoint::LeftForearm,
        FirstPersonRigJoint::LeftUpperArm,
        leftForearmLocal,
        leftForearmWorld,
        armsYaw,
        armsPitch * 0.76F,
        armsRoll * 0.62F);
    setJoint(
        frame,
        FirstPersonRigJoint::LeftHand,
        FirstPersonRigJoint::LeftForearm,
        leftHandLocal,
        leftHandWorld,
        armsYaw,
        armsPitch,
        armsRoll);

    frame.rightHand = makeAttachment(rightHandWorld, {0.10F, 0.10F, 0.10F}, armsYaw, armsPitch, armsRoll);
    frame.leftHand = makeAttachment(leftHandWorld, {0.10F, 0.10F, 0.10F}, armsYaw, armsPitch, armsRoll);
    frame.supportElbow = makeAttachment(supportElbowWorld, {0.12F, 0.12F, 0.12F}, armsYaw, armsPitch, armsRoll);
}

} // namespace

std::string_view firstPersonRigJointName(FirstPersonRigJoint joint) {
    switch (joint) {
    case FirstPersonRigJoint::Root:
        return "root";
    case FirstPersonRigJoint::Pelvis:
        return "pelvis";
    case FirstPersonRigJoint::Spine:
        return "spine";
    case FirstPersonRigJoint::Chest:
        return "chest";
    case FirstPersonRigJoint::Neck:
        return "neck";
    case FirstPersonRigJoint::Head:
        return "head";
    case FirstPersonRigJoint::RightClavicle:
        return "right_clavicle";
    case FirstPersonRigJoint::RightUpperArm:
        return "right_upper_arm";
    case FirstPersonRigJoint::RightForearm:
        return "right_forearm";
    case FirstPersonRigJoint::RightHand:
        return "right_hand";
    case FirstPersonRigJoint::LeftClavicle:
        return "left_clavicle";
    case FirstPersonRigJoint::LeftUpperArm:
        return "left_upper_arm";
    case FirstPersonRigJoint::LeftForearm:
        return "left_forearm";
    case FirstPersonRigJoint::LeftHand:
        return "left_hand";
    case FirstPersonRigJoint::WeaponRoot:
        return "weapon_root";
    case FirstPersonRigJoint::Muzzle:
        return "muzzle";
    case FirstPersonRigJoint::Count:
        break;
    }
    return "unknown";
}

std::string_view firstPersonRigSocketName(FirstPersonRigSocket socket) {
    switch (socket) {
    case FirstPersonRigSocket::Camera:
        return "camera";
    case FirstPersonRigSocket::BodyRoot:
        return "body_root";
    case FirstPersonRigSocket::WeaponRoot:
        return "weapon_root";
    case FirstPersonRigSocket::Muzzle:
        return "muzzle";
    case FirstPersonRigSocket::EjectionPort:
        return "ejection_port";
    case FirstPersonRigSocket::RightGrip:
        return "right_grip";
    case FirstPersonRigSocket::LeftGrip:
        return "left_grip";
    case FirstPersonRigSocket::RightHand:
        return "right_hand";
    case FirstPersonRigSocket::LeftHand:
        return "left_hand";
    case FirstPersonRigSocket::SupportElbow:
        return "support_elbow";
    case FirstPersonRigSocket::Count:
        break;
    }
    return "unknown";
}

const FirstPersonJointPose& firstPersonRigJoint(
    const FirstPersonRigFrame& frame,
    FirstPersonRigJoint joint) {
    return frame.joints[std::min(
        static_cast<std::size_t>(joint),
        kFirstPersonRigJointCount - 1U)];
}

const FirstPersonRigSocketPose& firstPersonRigSocket(
    const FirstPersonRigFrame& frame,
    FirstPersonRigSocket socket) {
    return frame.sockets[std::min(
        static_cast<std::size_t>(socket),
        kFirstPersonRigSocketCount - 1U)];
}

FirstPersonRigFrame evaluateFirstPersonRig(const FirstPersonRigInput& input) {
    FirstPersonRigFrame frame{};
    const auto& animation = input.animation;
    const float ads = std::max(clamp01(input.adsAlpha), clamp01(input.weapon.adsAlpha));
    const float reloadProgress = input.weapon.reloading ? clamp01(input.weapon.reloadProgress) : 0.0F;
    frame.reloadArc = std::max(normalizedReloadArc(input.weapon, animation.reloadAlpha), reloadProgress * animation.reloadAlpha);
    frame.lookDownBodyAlpha = bodyLookDownAlpha(input.view.pitchDegrees);
    frame.wallRunRollDegrees = input.hasWallRunContact ? input.cameraRollDegrees : 0.0F;

    PlayerViewComponent bodyView = input.view;
    bodyView.pitchDegrees = std::clamp(input.view.pitchDegrees * 0.10F, -7.5F, 4.5F);
    const auto cameraBasis = basisFromView(input.view, input.cameraRollDegrees);
    const auto bodyBasis = basisFromView(bodyView, bodyRollForView(input.cameraRollDegrees, animation) * 0.35F);

    const auto weaponOffset =
        lerpVec3(input.weaponMount.hipOffset, input.weaponMount.adsOffset, ads) +
        animation.firstPersonWeaponOffset;
    const auto armsOffset =
        lerpVec3(input.armsMount.hipOffset, input.armsMount.adsOffset, ads) +
        animation.firstPersonArmsOffset;
    const auto bodyOffset =
        lerpVec3(input.bodyMount.hipOffset, input.bodyMount.adsOffset, ads) +
        animation.firstPersonBodyOffset;

    const bool sidearm = isSidearm(input.weaponClass);
    const float recoilLift = input.weapon.recoilPitchOffsetDegrees * 0.010F;
    const float recoilSide = input.weapon.recoilYawOffsetDegrees * 0.014F;
    const float mantleLift = clamp01(input.mantleProgress01) * 0.13F;
    const auto weaponSway = input.weaponSwayOffset * (sidearm ? 0.52F : 0.82F) * (1.0F - (ads * 0.70F));
    const auto armsSway = input.weaponSwayOffset * 0.36F * (1.0F - (ads * 0.60F));

    const auto weaponPosition =
        input.cameraPosition +
        toWorld(
            cameraBasis,
            weaponOffset +
                weaponSway +
                makeVec(recoilSide, recoilLift - (frame.reloadArc * 0.12F) + mantleLift, 0.0F));
    const auto armsPosition =
        input.cameraPosition +
        toWorld(
            cameraBasis,
            armsOffset +
                armsSway +
                makeVec(-frame.reloadArc * 0.05F, (-frame.reloadArc * 0.11F) + mantleLift, 0.0F));
    const auto bodyPosition =
        input.cameraPosition +
        toWorld(bodyBasis, bodyOffset) +
        (input.headBobOffset * 0.22F);

    const float weaponYaw =
        input.view.yawDegrees +
        input.weaponMount.yawCorrectionDegrees +
        animation.firstPersonWeaponYawAddDegrees +
        (input.weapon.recoilYawOffsetDegrees * input.weaponMount.recoilYawScale);
    const float weaponPitch =
        -input.view.pitchDegrees +
        input.weaponMount.pitchCorrectionDegrees +
        animation.firstPersonWeaponPitchAddDegrees +
        (input.weapon.recoilPitchOffsetDegrees * input.weaponMount.recoilPitchScale) +
        (frame.reloadArc * input.weaponMount.reloadPitchDegrees);
    const float weaponRoll =
        input.cameraRollDegrees +
        input.weaponMount.rollCorrectionDegrees +
        animation.firstPersonWeaponRollAddDegrees -
        (frame.reloadArc * input.weaponMount.reloadRollDegrees);
    const float armsYaw =
        input.view.yawDegrees +
        input.armsMount.yawCorrectionDegrees +
        animation.firstPersonArmsYawAddDegrees;
    const float armsPitch =
        -input.view.pitchDegrees +
        input.armsMount.pitchCorrectionDegrees +
        animation.firstPersonArmsPitchAddDegrees +
        (frame.reloadArc * 8.0F);
    const float armsRoll =
        input.cameraRollDegrees +
        input.armsMount.rollCorrectionDegrees +
        animation.firstPersonArmsRollAddDegrees -
        (frame.reloadArc * 12.0F);
    const float bodyPitch = bodyPitchForView(input.view.pitchDegrees, animation);
    const float bodyRoll = bodyRollForView(input.cameraRollDegrees, animation);
    const float bodyAlpha = std::clamp(0.18F + (frame.lookDownBodyAlpha * 0.48F), 0.18F, 0.66F);

    frame.weapon = makeAttachment(
        weaponPosition,
        input.weaponMount.scale * (1.0F - (ads * 0.055F)),
        weaponYaw,
        weaponPitch,
        weaponRoll);
    frame.arms = makeAttachment(
        armsPosition,
        input.armsMount.scale,
        armsYaw,
        armsPitch,
        armsRoll);
    frame.body = makeAttachment(
        bodyPosition,
        mulVec3(input.bodyMount.scale, makeVec(1.0F - (ads * 0.025F), 1.0F, 1.0F - (ads * 0.025F))),
        input.view.yawDegrees + input.bodyMount.yawCorrectionDegrees,
        input.bodyMount.pitchCorrectionDegrees + bodyPitch,
        input.bodyMount.rollCorrectionDegrees + bodyRoll,
        bodyAlpha);

    buildBodyJoints(frame, input, bodyBasis, bodyPosition, bodyPitch, bodyRoll);
    const auto weaponBasis = basisFromMeshRotation(weaponYaw, weaponPitch, weaponRoll);
    const auto armsBasis = basisFromMeshRotation(armsYaw, armsPitch, armsRoll);
    const auto socketWorldPosition = [&](novacore::math::Vec3 localPosition) {
        return weaponPosition + toWorld(weaponBasis, mulVec3(localPosition, frame.weapon.scale));
    };
    const auto muzzlePosition = socketWorldPosition(input.weaponSockets.muzzle);
    const auto ejectionPortPosition = socketWorldPosition(input.weaponSockets.ejectionPort);
    const auto rightGripPosition = socketWorldPosition(input.weaponSockets.rightGrip);
    const auto leftGripPosition = socketWorldPosition(input.weaponSockets.leftGrip);

    buildArmJoints(
        frame,
        input,
        armsBasis,
        armsPosition,
        rightGripPosition,
        leftGripPosition,
        armsYaw,
        armsPitch,
        armsRoll);
    setJoint(
        frame,
        FirstPersonRigJoint::WeaponRoot,
        FirstPersonRigJoint::RightHand,
        {},
        weaponPosition,
        weaponYaw,
        weaponPitch,
        weaponRoll);
    setJoint(
        frame,
        FirstPersonRigJoint::Muzzle,
        FirstPersonRigJoint::WeaponRoot,
        input.weaponSockets.muzzle,
        muzzlePosition,
        weaponYaw,
        weaponPitch,
        weaponRoll);
    frame.muzzle = makeAttachment(muzzlePosition, {0.06F, 0.06F, 0.06F}, weaponYaw, weaponPitch, weaponRoll);

    setSocket(
        frame,
        FirstPersonRigSocket::Camera,
        FirstPersonRigJoint::Root,
        {},
        input.cameraPosition,
        input.view.yawDegrees,
        input.view.pitchDegrees,
        input.cameraRollDegrees);
    setSocket(
        frame,
        FirstPersonRigSocket::BodyRoot,
        FirstPersonRigJoint::Pelvis,
        input.bodyMount.hipOffset,
        bodyPosition,
        frame.body.yawDegrees,
        frame.body.pitchDegrees,
        frame.body.rollDegrees);
    setSocket(
        frame,
        FirstPersonRigSocket::WeaponRoot,
        FirstPersonRigJoint::WeaponRoot,
        {},
        weaponPosition,
        weaponYaw,
        weaponPitch,
        weaponRoll);
    setSocket(
        frame,
        FirstPersonRigSocket::Muzzle,
        FirstPersonRigJoint::Muzzle,
        input.weaponSockets.muzzle,
        muzzlePosition,
        weaponYaw,
        weaponPitch,
        weaponRoll);
    setSocket(
        frame,
        FirstPersonRigSocket::EjectionPort,
        FirstPersonRigJoint::WeaponRoot,
        input.weaponSockets.ejectionPort,
        ejectionPortPosition,
        weaponYaw,
        weaponPitch,
        weaponRoll);
    setSocket(
        frame,
        FirstPersonRigSocket::RightGrip,
        FirstPersonRigJoint::RightHand,
        input.weaponSockets.rightGrip,
        rightGripPosition,
        weaponYaw,
        weaponPitch,
        weaponRoll);
    setSocket(
        frame,
        FirstPersonRigSocket::LeftGrip,
        FirstPersonRigJoint::LeftHand,
        input.weaponSockets.leftGrip,
        leftGripPosition,
        weaponYaw,
        weaponPitch,
        weaponRoll);
    setSocket(
        frame,
        FirstPersonRigSocket::RightHand,
        FirstPersonRigJoint::RightHand,
        firstPersonRigJoint(frame, FirstPersonRigJoint::RightHand).localPosition,
        frame.rightHand.position,
        armsYaw,
        armsPitch,
        armsRoll);
    setSocket(
        frame,
        FirstPersonRigSocket::LeftHand,
        FirstPersonRigJoint::LeftHand,
        firstPersonRigJoint(frame, FirstPersonRigJoint::LeftHand).localPosition,
        frame.leftHand.position,
        armsYaw,
        armsPitch,
        armsRoll);
    setSocket(
        frame,
        FirstPersonRigSocket::SupportElbow,
        FirstPersonRigJoint::LeftForearm,
        firstPersonRigJoint(frame, FirstPersonRigJoint::LeftForearm).localPosition,
        frame.supportElbow.position,
        armsYaw,
        armsPitch,
        armsRoll);

    return frame;
}

} // namespace nemisis::player
