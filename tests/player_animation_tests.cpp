#include "nemisis/player/PlayerAnimation.hpp"

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

nemisis::player::CharacterAnimationInput baseInput() {
    nemisis::player::CharacterAnimationInput input{};
    input.fixedDeltaSeconds = 1.0F / 60.0F;
    input.weapon.weaponId = "ar_01";
    input.weapon.ammoInMagazine = 30;
    input.weapon.timeSinceLastShotSeconds = 1.0F;
    return input;
}

nemisis::player::CharacterAnimationFrame evaluate(
    nemisis::player::CharacterAnimationState& state,
    nemisis::player::CharacterAnimationInput input,
    int ticks = 1) {
    nemisis::player::CharacterAnimationFrame frame{};
    for (int i = 0; i < ticks; ++i) {
        frame = nemisis::player::updateCharacterAnimation(state, input);
    }
    return frame;
}

void testIdleProducesBreathingViewmodelPose() {
    nemisis::player::CharacterAnimationState state{};
    auto input = baseInput();

    const auto frame = evaluate(state, input, 12);

    expect(frame.locomotionClip == nemisis::player::CharacterAnimationClip::Idle, "idle clip is selected without movement");
    expect(frame.locomotionAlpha > 0.95F, "idle alpha is high while standing still");
    expect(frame.idleBreath01 >= 0.0F && frame.idleBreath01 <= 1.0F, "idle breathing stays normalized");
    expect(frame.firstPersonWeaponOffset.z > -0.02F, "idle weapon pose is not sprint-lowered");
    expect(nemisis::player::characterAnimationClipName(frame.locomotionClip) == "idle", "idle clip has debug name");
}

void testWalkAndSprintAdvanceGaitAndLowerWeapon() {
    nemisis::player::CharacterAnimationState walkState{};
    auto walkInput = baseInput();
    walkInput.velocity = {3.0F, 0.0F, 1.0F};
    walkInput.speed01 = 0.42F;
    const auto walkFrame = evaluate(walkState, walkInput, 8);

    nemisis::player::CharacterAnimationState sprintState{};
    auto sprintInput = baseInput();
    sprintInput.velocity = {7.2F, 0.0F, 0.0F};
    sprintInput.speed01 = 0.92F;
    sprintInput.sprintHeld = true;
    const auto sprintFrame = evaluate(sprintState, sprintInput, 8);

    expect(walkFrame.locomotionClip == nemisis::player::CharacterAnimationClip::Walk, "walk clip is selected at medium speed");
    expect(sprintFrame.locomotionClip == nemisis::player::CharacterAnimationClip::Sprint, "sprint clip is selected at high speed");
    expect(sprintFrame.gaitPhase > walkFrame.gaitPhase, "sprint gait advances faster than walk");
    expect(sprintFrame.firstPersonWeaponOffset.y < walkFrame.firstPersonWeaponOffset.y - 0.02F, "sprint lowers weapon into a run pose");
    expect(sprintFrame.firstPersonWeaponOffset.z < walkFrame.firstPersonWeaponOffset.z - 0.08F, "sprint pulls weapon back");
}

void testAdsDampsFreeWeaponPose() {
    nemisis::player::CharacterAnimationState hipState{};
    auto hipInput = baseInput();
    hipInput.speed01 = 0.94F;
    hipInput.velocity = {7.0F, 0.0F, 0.0F};
    hipInput.sprintHeld = true;
    const auto hipFrame = evaluate(hipState, hipInput, 10);

    nemisis::player::CharacterAnimationState adsState{};
    auto adsInput = hipInput;
    adsInput.adsHeld = true;
    adsInput.weapon.adsAlpha = 1.0F;
    const auto adsFrame = evaluate(adsState, adsInput, 10);

    expect(adsFrame.upperBodyClip == nemisis::player::CharacterAnimationClip::Ads, "ADS selects upper-body ADS clip");
    expect(adsFrame.adsAlpha > 0.95F, "ADS alpha follows weapon state");
    expect(adsFrame.firstPersonWeaponOffset.z > hipFrame.firstPersonWeaponOffset.z, "ADS reduces sprint weapon pullback");
    expect(adsFrame.firstPersonWeaponRollAddDegrees < hipFrame.firstPersonWeaponRollAddDegrees, "ADS damps sprint roll");
}

void testSlideWallRunAndMantleProduceDistinctPoses() {
    nemisis::player::CharacterAnimationState slideState{};
    auto slideInput = baseInput();
    slideInput.movementMode = nemisis::movement::MovementMode::Sliding;
    slideInput.velocity = {6.0F, 0.0F, 0.0F};
    slideInput.speed01 = 0.80F;
    const auto slideFrame = evaluate(slideState, slideInput, 10);

    nemisis::player::CharacterAnimationState wallState{};
    auto wallInput = baseInput();
    wallInput.movementMode = nemisis::movement::MovementMode::WallRunning;
    wallInput.velocity = {7.8F, 0.0F, 0.0F};
    wallInput.speed01 = 0.85F;
    wallInput.cameraRollDegrees = 12.0F;
    wallInput.hasWallRunContact = true;
    const auto wallFrame = evaluate(wallState, wallInput, 10);

    nemisis::player::CharacterAnimationState mantleState{};
    auto mantleInput = baseInput();
    mantleInput.movementMode = nemisis::movement::MovementMode::Mantling;
    mantleInput.mantleProgress01 = 0.55F;
    mantleInput.movementTech.mantleReachSeconds = nemisis::movement::kMantleReachCueSeconds;
    const auto mantleFrame = evaluate(mantleState, mantleInput, 10);

    expect(slideFrame.locomotionClip == nemisis::player::CharacterAnimationClip::Slide, "slide clip is selected");
    expect(slideFrame.slideAlpha > 0.95F, "slide alpha blends in quickly");
    expect(slideFrame.firstPersonWeaponPitchAddDegrees < -4.0F, "slide pitches weapon down");
    expect(wallFrame.locomotionClip == nemisis::player::CharacterAnimationClip::WallRun, "wallrun clip is selected");
    expect(wallFrame.wallRunAlpha > 0.95F, "wallrun alpha blends in quickly");
    expect(wallFrame.wallRunLean > 0.70F, "wallrun lean follows camera roll");
    expect(mantleFrame.locomotionClip == nemisis::player::CharacterAnimationClip::Mantle, "mantle clip is selected");
    expect(mantleFrame.mantleAlpha > 0.50F, "mantle alpha respects progress");
    expect(mantleFrame.mantleReachAlpha > 0.95F, "mantle reach cue drives left-hand animation");
}

void testReloadFireAndEnergyPlatformDriveUpperBody() {
    nemisis::player::CharacterAnimationState reloadState{};
    auto reloadInput = baseInput();
    reloadInput.weapon.reloading = true;
    reloadInput.weapon.reloadProgress = 0.50F;
    const auto reloadFrame = evaluate(reloadState, reloadInput, 10);

    nemisis::player::CharacterAnimationState fireState{};
    auto fireInput = baseInput();
    (void)evaluate(fireState, fireInput, 1);
    fireInput.weapon.shotIndex = 1;
    fireInput.weapon.timeSinceLastShotSeconds = 0.0F;
    const auto fireFrame = evaluate(fireState, fireInput, 1);

    nemisis::player::CharacterAnimationState jumpState{};
    auto jumpInput = baseInput();
    jumpInput.movementMode = nemisis::movement::MovementMode::Airborne;
    jumpInput.movementTech.energyPlatformSeconds = nemisis::movement::kEnergyPlatformCueSeconds;
    const auto jumpFrame = evaluate(jumpState, jumpInput, 2);

    expect(reloadFrame.upperBodyClip == nemisis::player::CharacterAnimationClip::Reload, "reload selects reload upper-body clip");
    expect(reloadFrame.reloadAlpha > 0.95F, "reload blends in quickly");
    expect(reloadFrame.firstPersonArmsOffset.y < -0.08F, "reload lowers support arms");
    expect(fireFrame.upperBodyClip == nemisis::player::CharacterAnimationClip::Fire, "shot index selects fire upper-body clip");
    expect(fireFrame.fireAlpha > 0.90F, "fire kick starts immediately on a new shot");
    expect(jumpFrame.locomotionClip == nemisis::player::CharacterAnimationClip::Airborne, "airborne clip selected during double jump cue");
    expect(jumpFrame.energyPlatformAlpha > 0.95F, "energy platform cue exposes double jump animation alpha");
    expect(jumpFrame.leftHandLocalOffset.x < -0.22F, "energy platform cue pushes left hand out for lore throw");
}

void testResetClearsTransientAnimationState() {
    nemisis::player::CharacterAnimationState state{};
    auto input = baseInput();
    input.weapon.shotIndex = 7;
    input.weapon.timeSinceLastShotSeconds = 0.0F;
    const auto activeFrame = evaluate(state, input, 1);
    expect(activeFrame.fireAlpha > 0.0F, "fire frame is active before reset");

    nemisis::player::resetCharacterAnimation(state);
    const auto resetFrame = nemisis::player::evaluateCharacterAnimation(state, baseInput());
    expect(resetFrame.fireAlpha == 0.0F, "reset clears fire alpha");
    expect(resetFrame.reloadAlpha == 0.0F, "reset clears reload alpha");
    expect(resetFrame.wallRunAlpha == 0.0F, "reset clears wallrun alpha");
}

} // namespace

int main() {
    testIdleProducesBreathingViewmodelPose();
    testWalkAndSprintAdvanceGaitAndLowerWeapon();
    testAdsDampsFreeWeaponPose();
    testSlideWallRunAndMantleProduceDistinctPoses();
    testReloadFireAndEnergyPlatformDriveUpperBody();
    testResetClearsTransientAnimationState();

    if (failures > 0) {
        std::cerr << failures << " player animation test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "player animation tests passed\n";
    return EXIT_SUCCESS;
}
