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

} // namespace

int main() {
    testCameraInitializesAtEyeHeight();
    testSprintExpandsFovAndBob();
    testWeaponRecoilAffectsVisualView();

    if (failures > 0) {
        std::cerr << failures << " player camera rig test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis player camera rig tests passed\n";
    return EXIT_SUCCESS;
}
