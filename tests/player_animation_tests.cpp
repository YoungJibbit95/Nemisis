#include "nemisis/player/PlayerAnimation.hpp"

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

bool nearlyEqual(float lhs, float rhs, float epsilon = 0.0001F) {
    return std::abs(lhs - rhs) <= epsilon;
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

void testDeterministicTransitionsAndNormalizedClipTime() {
    nemisis::player::CharacterAnimationState firstState{};
    nemisis::player::CharacterAnimationState secondState{};
    auto input = baseInput();
    input.velocity = {6.5F, 0.0F, 1.0F};
    input.speed01 = 0.84F;
    input.sprintHeld = true;

    float previousTransition = 0.0F;
    for (int tick = 0; tick < 16; ++tick) {
        const auto first = nemisis::player::updateCharacterAnimation(firstState, input);
        const auto second = nemisis::player::updateCharacterAnimation(secondState, input);
        expect(first.locomotionClip == second.locomotionClip, "identical fixed-tick input selects identical clips");
        expect(nearlyEqual(first.gaitPhase, second.gaitPhase), "identical fixed-tick input advances identical gait phase");
        expect(nearlyEqual(first.locomotionTransitionAlpha, second.locomotionTransitionAlpha),
            "identical fixed-tick input advances identical transition alpha");
        expect(first.locomotionNormalizedTime >= 0.0F && first.locomotionNormalizedTime < 1.0F,
            "looping locomotion time remains normalized");
        expect(first.locomotionTransitionAlpha >= previousTransition,
            "locomotion transition advances monotonically");
        previousTransition = first.locomotionTransitionAlpha;
    }

    expect(firstState.locomotionClip == nemisis::player::CharacterAnimationClip::Sprint,
        "deterministic state transition reaches sprint");
    expect(firstState.locomotionTransitionAlpha > 0.99F, "sprint transition reaches full weight");
}

void testJumpFallLandStateAndEvents() {
    nemisis::player::CharacterAnimationState state{};
    auto grounded = baseInput();
    (void)evaluate(state, grounded, 2);

    auto jumping = grounded;
    jumping.movementMode = nemisis::movement::MovementMode::Airborne;
    jumping.velocity = {1.0F, 5.5F, 0.0F};
    const auto jumpFrame = evaluate(state, jumping);
    expect(jumpFrame.locomotionClip == nemisis::player::CharacterAnimationClip::Jump,
        "positive airborne velocity selects jump clip");
    expect(jumpFrame.jumpAlpha > 0.95F, "jump alpha follows upward velocity");
    expect(nemisis::player::hasCharacterAnimationEvent(
        jumpFrame, nemisis::player::CharacterAnimationEvent::JumpStart),
        "ground-to-air transition emits jump marker");

    auto falling = jumping;
    falling.velocity.y = -4.0F;
    const auto fallFrame = evaluate(state, falling);
    expect(fallFrame.locomotionClip == nemisis::player::CharacterAnimationClip::Fall,
        "negative airborne velocity selects fall clip");
    expect(fallFrame.fallAlpha > 0.70F, "fall alpha follows downward velocity");
    expect(nemisis::player::hasCharacterAnimationEvent(
        fallFrame, nemisis::player::CharacterAnimationEvent::Apex),
        "vertical velocity sign change emits apex marker");

    auto landed = grounded;
    landed.velocity = {1.0F, 0.0F, 0.0F};
    const auto landFrame = evaluate(state, landed);
    expect(landFrame.locomotionClip == nemisis::player::CharacterAnimationClip::Land,
        "air-to-ground transition selects land clip");
    expect(landFrame.landAlpha > 0.20F, "landing pose starts blending on contact");
    expect(nemisis::player::hasCharacterAnimationEvent(
        landFrame, nemisis::player::CharacterAnimationEvent::Land),
        "air-to-ground transition emits land marker");
    expect(landFrame.locomotionPhase == nemisis::player::CharacterClipPhase::Enter,
        "new land clip reports enter phase");
}

void testCrouchDirectionAndTurnLean() {
    nemisis::player::CharacterAnimationState state{};
    auto input = baseInput();
    input.crouchHeld = true;
    input.velocity = {2.5F, 0.0F, 0.0F};
    input.speed01 = 0.35F;
    input.facingForward = {0.0F, 0.0F, 1.0F};
    input.facingRight = {1.0F, 0.0F, 0.0F};
    input.turnDeltaDegrees = 28.0F;
    const auto frame = evaluate(state, input, 12);

    expect(frame.locomotionClip == nemisis::player::CharacterAnimationClip::Crouch,
        "grounded crouch selects crouch clip");
    expect(frame.crouchAlpha > 0.95F, "crouch pose blends to full weight");
    expect(frame.locomotionRight > 0.99F && std::abs(frame.locomotionForward) < 0.01F,
        "world velocity resolves into local strafe direction");
    expect(frame.movementAngleDegrees > 89.0F && frame.movementAngleDegrees < 91.0F,
        "local strafe direction exposes a stable movement angle");
    expect(frame.turnLean > 0.70F, "turn delta drives deterministic turn lean");
    expect(frame.thirdPersonBodyOffset.y < -0.05F, "crouch lowers the third-person body pose");
}

void testConcurrentUpperBodyAdditivesAndMarkers() {
    nemisis::player::CharacterAnimationState state{};
    auto input = baseInput();
    (void)evaluate(state, input, 1);
    input.adsHeld = true;
    input.weapon.adsAlpha = 1.0F;
    input.weapon.shotIndex = 1U;
    input.weapon.timeSinceLastShotSeconds = 0.0F;
    input.weapon.reloading = true;
    input.weapon.reloadProgress = 0.40F;
    const auto layered = evaluate(state, input, 1);

    expect(layered.upperBodyClip == nemisis::player::CharacterAnimationClip::Reload,
        "reload remains the dominant upper-body state");
    expect(layered.additiveLayerCount == 3U, "ADS, fire, and reload coexist as additive layers");
    expect(layered.additiveLayers[0].clip == nemisis::player::CharacterAnimationClip::Ads,
        "ADS additive layer has stable ordering");
    expect(layered.additiveLayers[1].clip == nemisis::player::CharacterAnimationClip::Fire,
        "fire additive layer has stable ordering");
    expect(layered.additiveLayers[2].clip == nemisis::player::CharacterAnimationClip::Reload,
        "reload additive layer has stable ordering");
    expect(nemisis::player::hasCharacterAnimationEvent(
        layered, nemisis::player::CharacterAnimationEvent::Fire),
        "shot index change emits fire marker");
    expect(nemisis::player::hasCharacterAnimationEvent(
        layered, nemisis::player::CharacterAnimationEvent::ReloadStart),
        "reload edge emits reload-start marker");
    expect(nemisis::player::hasCharacterAnimationEvent(
        layered, nemisis::player::CharacterAnimationEvent::AdsEnter),
        "ADS edge emits enter marker");

    input.weapon.reloadProgress = 0.60F;
    const auto insert = evaluate(state, input, 1);
    expect(nemisis::player::hasCharacterAnimationEvent(
        insert, nemisis::player::CharacterAnimationEvent::ReloadInsert),
        "reload midpoint emits insert marker once crossed");

    input.weapon.reloading = false;
    input.weapon.timeSinceLastShotSeconds = 1.0F;
    const auto completed = evaluate(state, input, 1);
    expect(nemisis::player::hasCharacterAnimationEvent(
        completed, nemisis::player::CharacterAnimationEvent::ReloadComplete),
        "reload falling edge emits completion marker");
}

void testFootstepAndMovementTechMarkers() {
    nemisis::player::CharacterAnimationState walkState{};
    auto walk = baseInput();
    walk.velocity = {0.0F, 0.0F, 4.5F};
    walk.speed01 = 0.55F;
    int leftFootsteps = 0;
    int rightFootsteps = 0;
    for (int tick = 0; tick < 180; ++tick) {
        const auto frame = evaluate(walkState, walk);
        leftFootsteps += nemisis::player::hasCharacterAnimationEvent(
            frame, nemisis::player::CharacterAnimationEvent::FootstepLeft) ? 1 : 0;
        rightFootsteps += nemisis::player::hasCharacterAnimationEvent(
            frame, nemisis::player::CharacterAnimationEvent::FootstepRight) ? 1 : 0;
    }
    expect(leftFootsteps > 0 && rightFootsteps > 0, "loop phase crossings emit alternating footstep markers");
    expect(std::abs(leftFootsteps - rightFootsteps) <= 1, "footstep marker counts remain balanced");

    nemisis::player::CharacterAnimationState mantleState{};
    auto mantle = baseInput();
    mantle.movementMode = nemisis::movement::MovementMode::Mantling;
    mantle.mantleProgress01 = 0.20F;
    const auto reach = evaluate(mantleState, mantle);
    expect(nemisis::player::hasCharacterAnimationEvent(
        reach, nemisis::player::CharacterAnimationEvent::MantleReach),
        "mantle entry emits reach marker");
    mantle.mantleProgress01 = 0.55F;
    const auto pull = evaluate(mantleState, mantle);
    expect(nemisis::player::hasCharacterAnimationEvent(
        pull, nemisis::player::CharacterAnimationEvent::MantlePull),
        "mantle phase crossing emits pull marker");
}

void testSkeletonPoseAndSocketsAreQueryable() {
    nemisis::player::CharacterAnimationState state{};
    auto input = baseInput();
    input.velocity = {2.0F, 0.0F, 4.0F};
    input.speed01 = 0.60F;
    input.adsHeld = true;
    input.weapon.adsAlpha = 1.0F;
    input.aimPitchDegrees = 25.0F;
    input.aimYawDegrees = -18.0F;
    const auto frame = evaluate(state, input, 8);

    const auto* root = nemisis::player::findCharacterBonePose(frame.pose, nemisis::player::CharacterBone::Root);
    const auto* chest = nemisis::player::findCharacterBonePose(frame.pose, nemisis::player::CharacterBone::Chest);
    const auto* hand = nemisis::player::findCharacterBonePose(frame.pose, nemisis::player::CharacterBone::HandRight);
    const auto* muzzle = nemisis::player::findCharacterSocketPose(frame.pose, nemisis::player::CharacterSocket::Muzzle);
    expect(root != nullptr && root->parent == nemisis::player::CharacterBone::Invalid,
        "semantic skeleton pose exposes a root bone");
    expect(chest != nullptr && chest->parent == nemisis::player::CharacterBone::SpineUpper,
        "semantic skeleton pose exposes hierarchy links");
    expect(hand != nullptr && hand->parent == nemisis::player::CharacterBone::ForearmRight,
        "semantic skeleton pose exposes hand bone for weapon attachment");
    expect(muzzle != nullptr && muzzle->parentBone == nemisis::player::CharacterBone::HandRight,
        "muzzle socket is parented to the weapon hand");
    expect(chest != nullptr && std::abs(chest->local.rotation.x) > 0.001F,
        "ADS aim offset is present in upper-body bone rotation");
    expect(nemisis::player::characterBoneName(nemisis::player::CharacterBone::HandRight) == "hand_r",
        "bone semantics expose stable binding names");
    expect(nemisis::player::characterSocketName(nemisis::player::CharacterSocket::Muzzle) == "muzzle",
        "socket semantics expose stable binding names");
    expect(nemisis::player::characterAnimationEventName(nemisis::player::CharacterAnimationEvent::ReloadInsert) ==
            "reload_insert",
        "event markers expose stable debug names");
}

} // namespace

int main() {
    testIdleProducesBreathingViewmodelPose();
    testWalkAndSprintAdvanceGaitAndLowerWeapon();
    testAdsDampsFreeWeaponPose();
    testSlideWallRunAndMantleProduceDistinctPoses();
    testReloadFireAndEnergyPlatformDriveUpperBody();
    testResetClearsTransientAnimationState();
    testDeterministicTransitionsAndNormalizedClipTime();
    testJumpFallLandStateAndEvents();
    testCrouchDirectionAndTurnLean();
    testConcurrentUpperBodyAdditivesAndMarkers();
    testFootstepAndMovementTechMarkers();
    testSkeletonPoseAndSocketsAreQueryable();

    if (failures > 0) {
        std::cerr << failures << " player animation test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "player animation tests passed\n";
    return EXIT_SUCCESS;
}
