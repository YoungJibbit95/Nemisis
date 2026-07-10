#include "nemisis/player/PlayerCameraRig.hpp"

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
    const float delta = actual > expected ? actual - expected : expected - actual;
    expect(delta <= tolerance, message);
}

void testCameraInitializesAtEyeHeight() {
    nemisis::player::CameraRigState state{};
    nemisis::player::CameraRigInput input{};
    input.playerPosition = {2.0F, 0.0F, 3.0F};
    input.authoredView.yawDegrees = 45.0F;
    input.authoredView.pitchDegrees = -5.0F;

    const auto frame = nemisis::player::updateCameraRig(state, input);

    expect(state.initialized, "camera rig initializes on first update");
    expectNear(frame.position.x, 2.0F, 0.001F, "camera x starts at player x");
    expect(frame.position.y > 1.6F && frame.position.y < 1.7F, "camera starts at eye height");
    expectNear(frame.view.yawDegrees, 45.0F, 0.001F, "camera starts at authored yaw");
}

void testSprintExpandsFovAndBob() {
    nemisis::player::CameraRigState state{};
    nemisis::player::CameraRigInput input{};
    input.fixedDeltaSeconds = 1.0F / 60.0F;

    auto frame = nemisis::player::updateCameraRig(state, input);
    const float baseFov = frame.verticalFovDegrees;

    input.playerVelocity = {0.0F, 0.0F, 8.6F};
    input.sprintHeld = true;
    for (int i = 0; i < 30; ++i) {
        frame = nemisis::player::updateCameraRig(state, input);
    }

    expect(frame.verticalFovDegrees > baseFov, "sprint expands visual FOV");
    expect(frame.speed01 > 0.9F, "camera rig reports high speed fraction");
    expect(frame.headBobOffset.y > 0.0F, "grounded sprint produces head bob");
}

void testWeaponRecoilAffectsVisualView() {
    nemisis::player::CameraRigState state{};
    nemisis::player::CameraRigInput input{};
    input.weapon.recoilPitchOffsetDegrees = 1.0F;
    input.weapon.recoilYawOffsetDegrees = 0.5F;

    const auto frame = nemisis::player::updateCameraRig(state, input);

    expect(frame.view.pitchDegrees < 0.0F, "recoil pulls visual pitch upward");
    expect(frame.view.yawDegrees > 0.0F, "recoil nudges visual yaw");
}

void testAdsNarrowsVisualFov() {
    nemisis::player::CameraRigState state{};
    nemisis::player::CameraRigInput input{};
    input.fixedDeltaSeconds = 1.0F / 60.0F;

    auto frame = nemisis::player::updateCameraRig(state, input);
    const float baseFov = frame.verticalFovDegrees;

    input.adsHeld = true;
    input.weapon.adsAlpha = 1.0F;
    for (int i = 0; i < 30; ++i) {
        frame = nemisis::player::updateCameraRig(state, input);
    }

    expect(frame.verticalFovDegrees < baseFov - 4.0F, "ADS narrows visual FOV clearly");
    expect(frame.adsAlpha > 0.90F, "camera rig exposes ADS alpha");
}

void testFastLookProducesBoundedWeaponInertia() {
    nemisis::player::CameraRigState state{};
    nemisis::player::CameraRigInput input{};
    input.fixedDeltaSeconds = 1.0F / 60.0F;
    (void)nemisis::player::updateCameraRig(state, input);

    input.authoredView.yawDegrees = 28.0F;
    const auto hip = nemisis::player::updateCameraRig(state, input);
    expect(hip.weaponSwayOffset.x < -0.01F, "fast right look lags the weapon left");
    expect(hip.weaponSwayOffset.x >= -0.061F, "look inertia remains inside the viewmodel clipping budget");

    nemisis::player::CameraRigState adsState{};
    input.authoredView = {};
    input.weapon.adsAlpha = 1.0F;
    (void)nemisis::player::updateCameraRig(adsState, input);
    input.authoredView.yawDegrees = 28.0F;
    const auto ads = nemisis::player::updateCameraRig(adsState, input);
    expect(-ads.weaponSwayOffset.x < -hip.weaponSwayOffset.x, "ADS damps look inertia for sight stability");
}

void testLandingKickRecoversWithoutMovingPhysicsState() {
    nemisis::player::CameraRigState state{};
    nemisis::player::CameraRigInput input{};
    input.fixedDeltaSeconds = 1.0F / 60.0F;
    input.movementMode = nemisis::movement::MovementMode::Airborne;
    input.playerVelocity.y = -9.0F;
    (void)nemisis::player::updateCameraRig(state, input);

    input.movementMode = nemisis::movement::MovementMode::Grounded;
    const auto landed = nemisis::player::updateCameraRig(state, input);
    expect(state.landingKick > 0.0F, "airborne-to-grounded transition starts a landing presentation impulse");
    expect(landed.weaponSwayOffset.y < 0.0F, "landing impulse settles the held weapon downward");

    const float initialKick = state.landingKick;
    for (int i = 0; i < 30; ++i) {
        (void)nemisis::player::updateCameraRig(state, input);
    }
    expect(state.landingKick < initialKick * 0.1F, "landing presentation impulse decays quickly");
}

} // namespace

int main() {
    testCameraInitializesAtEyeHeight();
    testSprintExpandsFovAndBob();
    testWeaponRecoilAffectsVisualView();
    testAdsNarrowsVisualFov();
    testFastLookProducesBoundedWeaponInertia();
    testLandingKickRecoversWithoutMovingPhysicsState();

    if (failures > 0) {
        std::cerr << failures << " player camera rig test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis player camera rig tests passed\n";
    return EXIT_SUCCESS;
}
