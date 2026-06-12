#include "nemisis/dev/DevSandbox.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
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

void testSummaryIncludesPlayableTelemetry() {
    nemisis::dev::DevSandbox sandbox;

    nemisis::dev::DevSandboxSample sample{};
    sample.tick = 42;
    sample.command.fireHeld = true;
    sample.command.move.x = 1.0F;
    sample.position.x = 3.5F;
    sample.velocity.z = 7.25F;
    sample.weapon.weaponId = "ar_01";
    sample.weapon.ammoInMagazine = 29;
    sample.weapon.shotIndex = 1;
    sample.fire.fired = true;
    sample.network.pendingCommandCount = 3;
    sample.targetRange = nemisis::dev::makeDefaultDevTargetRange();
    sample.target.health = 122.0F;
    sample.target.hitsTaken = 2;
    sample.targetHit.hit = true;
    sample.movementTech.doubleJumpPlatformThrown = true;
    sample.movementTech.energyPlatformSeconds = 0.2F;
    sample.collision.nearWallRunSurface = true;
    sample.collision.wallPrimitiveId = "wallrun_left_panel_a";

    sandbox.recordTick(sample);
    const auto summary = sandbox.latestSummary();

    expect(summary.find("tick=42") != std::string::npos, "summary includes tick");
    expect(summary.find("weapon=ar_01") != std::string::npos, "summary includes weapon");
    expect(summary.find("ammo=29") != std::string::npos, "summary includes ammo");
    expect(summary.find("tech=energy-step") != std::string::npos, "summary includes movement tech cue");
    expect(summary.find("fire=yes") != std::string::npos, "summary includes fire result");
    expect(summary.find("pending=3") != std::string::npos, "summary includes pending commands");
    expect(summary.find("targetsAlive=4/4") != std::string::npos, "summary includes target lane count");
    expect(summary.find("targetHp=122.0") != std::string::npos, "summary includes target health");
    expect(summary.find("hit=yes") != std::string::npos, "summary includes target hit");
    expect(summary.find("wallrunSurface=yes") != std::string::npos, "summary includes wallrun surface state");
    expect(summary.find("wall=wallrun_left_panel_a") != std::string::npos, "summary includes wall contact id");
}

void testClearColorReflectsState() {
    nemisis::dev::DevSandbox sandbox;

    nemisis::dev::DevSandboxSample sample{};
    sample.movementMode = nemisis::movement::MovementMode::Grounded;
    sandbox.recordTick(sample);
    const auto grounded = sandbox.clearColor();

    sample.fire.fired = true;
    sandbox.recordTick(sample);
    const auto fired = sandbox.clearColor();

    expect(grounded != fired, "firing changes dev sandbox clear color");
}

} // namespace

int main() {
    testSummaryIncludesPlayableTelemetry();
    testClearColorReflectsState();

    if (failures > 0) {
        std::cerr << failures << " dev sandbox test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis dev sandbox tests passed\n";
    return EXIT_SUCCESS;
}
