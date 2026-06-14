#include "nemisis/player/PlayerFirstPersonRig.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

int failures = 0;

void expect(bool condition, std::string_view message) {
    if (condition) {
        return;
    }

    ++failures;
    std::cerr << "[fail] " << message << '\n';
}

void expectNear(float actual, float expected, float tolerance, std::string_view message) {
    expect(std::abs(actual - expected) <= tolerance, message);
}

nemisis::player::FirstPersonRigMountDesc longGunMount() {
    nemisis::player::FirstPersonRigMountDesc mount{};
    mount.hipOffset = {0.26F, -0.30F, 0.90F};
    mount.adsOffset = {0.035F, -0.165F, 0.72F};
    mount.scale = {1.0F, 1.0F, 1.0F};
    mount.yawCorrectionDegrees = 0.0F;
    mount.hipPitchFollow = 0.38F;
    mount.adsPitchFollow = 0.78F;
    mount.recoilYawScale = 0.46F;
    mount.recoilPitchScale = 0.34F;
    mount.reloadPitchDegrees = 9.5F;
    mount.reloadRollDegrees = 9.0F;
    return mount;
}

nemisis::player::FirstPersonRigMountDesc sidearmMount() {
    auto mount = longGunMount();
    mount.hipOffset = {0.18F, -0.245F, 0.68F};
    mount.adsOffset = {0.020F, -0.125F, 0.54F};
    mount.scale = {1.35F, 1.35F, 1.35F};
    mount.rollCorrectionDegrees = 0.0F;
    mount.recoilYawScale = 0.58F;
    mount.recoilPitchScale = 0.42F;
    return mount;
}

nemisis::player::FirstPersonRigMountDesc armsMount() {
    nemisis::player::FirstPersonRigMountDesc mount{};
    mount.hipOffset = {0.08F, -0.50F, 0.78F};
    mount.adsOffset = {0.018F, -0.43F, 0.62F};
    mount.scale = {0.84F, 0.84F, 0.84F};
    mount.hipPitchFollow = 0.46F;
    mount.adsPitchFollow = 0.58F;
    return mount;
}

nemisis::player::FirstPersonRigMountDesc bodyMount() {
    nemisis::player::FirstPersonRigMountDesc mount{};
    mount.hipOffset = {0.0F, -2.34F, 0.30F};
    mount.adsOffset = {0.0F, -2.38F, 0.22F};
    mount.scale = {0.46F, 0.46F, 0.46F};
    mount.hipPitchFollow = 0.12F;
    mount.adsPitchFollow = 0.10F;
    return mount;
}

nemisis::player::FirstPersonRigInput baseInput() {
    nemisis::player::FirstPersonRigInput input{};
    input.cameraPosition = {0.0F, 1.65F, 0.0F};
    input.view.yawDegrees = 0.0F;
    input.view.pitchDegrees = 0.0F;
    input.weapon.weaponId = "ar_01";
    input.weapon.ammoInMagazine = 30;
    input.weapon.timeSinceLastShotSeconds = 1.0F;
    input.weaponMount = longGunMount();
    input.armsMount = armsMount();
    input.bodyMount = bodyMount();
    input.animation = nemisis::player::evaluateCharacterAnimation(
        nemisis::player::CharacterAnimationState{},
        nemisis::player::CharacterAnimationInput{});
    input.hasAnimationFrame = true;
    return input;
}

void testRigBuildsStableSkeletonHierarchy() {
    const auto frame = nemisis::player::evaluateFirstPersonRig(baseInput());

    const auto& root = nemisis::player::firstPersonRigJoint(frame, nemisis::player::FirstPersonRigJoint::Root);
    const auto& pelvis = nemisis::player::firstPersonRigJoint(frame, nemisis::player::FirstPersonRigJoint::Pelvis);
    const auto& chest = nemisis::player::firstPersonRigJoint(frame, nemisis::player::FirstPersonRigJoint::Chest);
    const auto& head = nemisis::player::firstPersonRigJoint(frame, nemisis::player::FirstPersonRigJoint::Head);
    const auto& rightHand = nemisis::player::firstPersonRigJoint(frame, nemisis::player::FirstPersonRigJoint::RightHand);
    const auto& leftHand = nemisis::player::firstPersonRigJoint(frame, nemisis::player::FirstPersonRigJoint::LeftHand);

    expect(root.parent == nemisis::player::FirstPersonRigJoint::Count, "root joint has no parent");
    expect(pelvis.parent == nemisis::player::FirstPersonRigJoint::Root, "pelvis is parented to first-person root");
    expect(chest.parent == nemisis::player::FirstPersonRigJoint::Spine, "chest is parented through the torso chain");
    expect(head.worldPosition.y < root.worldPosition.y, "first-person head proxy stays below the camera");
    expect(chest.worldPosition.y > pelvis.worldPosition.y, "torso chain raises from pelvis to chest");
    expect(rightHand.worldPosition.z > root.worldPosition.z + 0.60F, "right hand is in front of the camera");
    expect(leftHand.worldPosition.z > root.worldPosition.z + 0.65F, "support hand is in front of the camera");
    expect(nemisis::player::firstPersonRigJointName(nemisis::player::FirstPersonRigJoint::WeaponRoot) == "weapon_root", "joint names expose socket ids");
}

void testAdsMovesWeaponAndHandsTowardSightline() {
    auto hip = baseInput();
    auto ads = hip;
    ads.adsAlpha = 1.0F;
    ads.weapon.adsAlpha = 1.0F;
    ads.animation.adsAlpha = 1.0F;

    const auto hipFrame = nemisis::player::evaluateFirstPersonRig(hip);
    const auto adsFrame = nemisis::player::evaluateFirstPersonRig(ads);

    expect(std::abs(adsFrame.weapon.position.x) < std::abs(hipFrame.weapon.position.x), "ADS moves weapon toward centerline");
    expect(adsFrame.weapon.position.y > hipFrame.weapon.position.y, "ADS raises weapon toward eye line");
    expect(adsFrame.weapon.position.z < hipFrame.weapon.position.z, "ADS pulls weapon closer to the player");
    expect(adsFrame.arms.position.y > hipFrame.arms.position.y, "ADS raises the arms socket with the weapon");
    expect(adsFrame.weapon.scale.x < hipFrame.weapon.scale.x, "ADS slightly shrinks weapon scale to reduce clipping");
}

void testLookDownRevealsBodyWithoutLiftingItIntoCamera() {
    auto level = baseInput();
    auto down = level;
    down.view.pitchDegrees = -54.0F;
    down.animation.firstPersonBodyOffset = {0.0F, -0.04F, 0.02F};

    const auto levelFrame = nemisis::player::evaluateFirstPersonRig(level);
    const auto downFrame = nemisis::player::evaluateFirstPersonRig(down);

    expect(downFrame.lookDownBodyAlpha > levelFrame.lookDownBodyAlpha + 0.80F, "looking down exposes the first-person body mask");
    expect(downFrame.body.alpha > levelFrame.body.alpha + 0.35F, "body render alpha rises only when the player looks down");
    expect(downFrame.body.position.y < down.cameraPosition.y - 2.0F, "body mesh root stays below the camera");
    expect(downFrame.body.pitchDegrees < 0.0F, "body inherits a small downward pitch instead of a camera-locked T pose");
}

void testReloadFireAndWallrunDriveSocketTransforms() {
    auto input = baseInput();
    input.weapon.reloading = true;
    input.weapon.reloadProgress = 0.50F;
    input.weapon.recoilPitchOffsetDegrees = 1.2F;
    input.weapon.recoilYawOffsetDegrees = -0.4F;
    input.animation.reloadAlpha = 1.0F;
    input.animation.fireAlpha = 1.0F;
    input.animation.firstPersonWeaponPitchAddDegrees = -1.8F;
    input.animation.firstPersonArmsRollAddDegrees = 4.0F;
    input.hasWallRunContact = true;
    input.cameraRollDegrees = 10.0F;

    const auto frame = nemisis::player::evaluateFirstPersonRig(input);

    expect(frame.reloadArc > 0.95F, "half reload produces full reload arc");
    expect(frame.weapon.pitchDegrees > 7.0F, "reload and recoil pitch weapon through the rig socket");
    expect(frame.weapon.rollDegrees < -5.0F, "reload rolls weapon away from the sightline");
    expect(std::abs(frame.arms.rollDegrees) > 4.0F, "arms retain animation/reload roll while wallrunning");
    expect(frame.wallRunRollDegrees > 3.0F, "wallrun camera roll propagates into first-person attachments");
    expect(frame.supportElbow.position.y < frame.leftHand.position.y, "support elbow stays below the support hand");
}

void testSidearmUsesShorterMuzzleAndPistolRoll() {
    auto rifle = baseInput();
    auto sidearm = rifle;
    sidearm.weaponClass = nemisis::weapons::WeaponClass::Sidearm;
    sidearm.weaponMount = sidearmMount();

    const auto rifleFrame = nemisis::player::evaluateFirstPersonRig(rifle);
    const auto sidearmFrame = nemisis::player::evaluateFirstPersonRig(sidearm);

    expect(rifleFrame.muzzle.position.z > rifleFrame.weapon.position.z + 0.70F, "rifle muzzle socket is near the long barrel");
    expect(sidearmFrame.muzzle.position.z < sidearmFrame.weapon.position.z + 0.50F, "sidearm muzzle socket is shorter than rifle socket");
    expectNear(sidearmFrame.weapon.rollDegrees, 0.0F, 0.01F, "sidearm uses normalized pistol roll");
}

} // namespace

int main() {
    testRigBuildsStableSkeletonHierarchy();
    testAdsMovesWeaponAndHandsTowardSightline();
    testLookDownRevealsBodyWithoutLiftingItIntoCamera();
    testReloadFireAndWallrunDriveSocketTransforms();
    testSidearmUsesShorterMuzzleAndPistolRoll();

    if (failures > 0) {
        std::cerr << failures << " first-person rig test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "first-person rig tests passed\n";
    return EXIT_SUCCESS;
}
