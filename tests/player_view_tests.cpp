#include "nemisis/player/PlayerView.hpp"

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

void testApplyLookWrapsYawAndClampsPitch() {
    nemisis::player::PlayerViewComponent view{};

    nemisis::player::applyLook(view, novacore::math::Vec2{190.0F, 120.0F});

    expectNear(view.yawDegrees, -170.0F, 0.001F, "yaw wraps into signed range");
    expectNear(view.pitchDegrees, 89.0F, 0.001F, "pitch clamps upward");

    nemisis::player::applyLook(view, novacore::math::Vec2{0.0F, -220.0F});
    expectNear(view.pitchDegrees, -89.0F, 0.001F, "pitch clamps downward");
}

void testViewVectorsUseYaw() {
    nemisis::player::PlayerViewComponent view{};
    view.yawDegrees = 90.0F;

    const auto vectors = nemisis::player::viewVectors(view);

    expect(vectors.horizontalForward.x > 0.99F, "90 degree yaw forward points along x");
    expect(std::abs(vectors.horizontalForward.z) < 0.01F, "90 degree yaw forward has near-zero z");
    expect(vectors.horizontalRight.z < -0.99F, "90 degree yaw right points negative z");
}

void testCommandMovesRelativeToView() {
    nemisis::player::PlayerViewComponent view{};
    view.yawDegrees = 90.0F;

    nemisis::player::PlayerInputCommand command{};
    command.move.y = 1.0F;

    const auto transformed = nemisis::player::commandRelativeToView(command, view);

    expect(transformed.move.x > 0.99F, "forward input becomes world x at 90 degree yaw");
    expect(std::abs(transformed.move.y) < 0.01F, "forward input loses world z at 90 degree yaw");
}

} // namespace

int main() {
    testApplyLookWrapsYawAndClampsPitch();
    testViewVectorsUseYaw();
    testCommandMovesRelativeToView();

    if (failures > 0) {
        std::cerr << failures << " player view test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis player view tests passed\n";
    return EXIT_SUCCESS;
}
