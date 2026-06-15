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
    sample.prediction.storedSamples = 7;
    sample.prediction.lastAcknowledgedTick = 40;
    sample.prediction.unacknowledgedTickSpan = 2;
    sample.prediction.hasLatestError = true;
    sample.prediction.latestError.tick = 39;
    sample.prediction.latestError.positionErrorMeters = 0.08F;
    sample.prediction.latestError.yawErrorDegrees = 0.4F;
    sample.prediction.latestError.exceedsCorrectionThreshold = false;
    sample.snapshots.storedSnapshots = 3;
    sample.snapshots.newestServerTick = 44;
    sample.snapshots.hasNewestServerTick = true;
    sample.targetRange = nemisis::dev::makeDefaultDevTargetRange();
    sample.target.health = 122.0F;
    sample.target.hitsTaken = 2;
    sample.targetHit.hit = true;
    sample.rangeSession.drill.score = 900;
    sample.rangeSession.drill.timeRemainingSeconds = 47.5F;
    sample.rangeSession.drill.shotsFired = 5;
    sample.rangeSession.drill.shotsHit = 4;
    sample.rangeSession.drill.recoilControlScore = 82.0F;
    sample.rangeSession.drill.latestTtkSeconds = 0.42F;
    sample.movementTech.doubleJumpPlatformThrown = true;
    sample.movementTech.energyPlatformSeconds = 0.2F;
    sample.collision.nearWallRunSurface = true;
    sample.collision.wallPrimitiveId = "wallrun_left_panel_a";
    sample.collision.mantleCandidate = true;
    sample.collision.mantlePrimitiveId = "ledge_training_mid";
    sample.collision.mantleHeight = 1.3F;
    sample.collision.swept = true;
    sample.collision.sweepHit = true;
    sample.collision.sweepPrimitiveId = "cover_left_mid";
    sample.collision.sweepFraction = 0.42F;
    sample.collision.sweepIterations = 2;
    sample.collision.contacts.push_back(nemisis::dev::GreyboxContact{
        "floor_main",
        nemisis::dev::GreyboxPrimitiveKind::Floor,
        nemisis::dev::GreyboxContactRole::Ground,
        {},
        {0.0F, 1.0F, 0.0F},
        0.0F,
        1.0F,
        0.0F,
        false,
        true,
    });
    sample.collision.contacts.push_back(nemisis::dev::GreyboxContact{
        "wallrun_left_panel_a",
        nemisis::dev::GreyboxPrimitiveKind::WallRunPanel,
        nemisis::dev::GreyboxContactRole::Wall,
        {},
        {1.0F, 0.0F, 0.0F},
        0.12F,
        1.0F,
        0.0F,
        true,
        false,
    });

    sandbox.recordTick(sample);
    const auto summary = sandbox.latestSummary();

    expect(summary.find("tick=42") != std::string::npos, "summary includes tick");
    expect(summary.find("weapon=ar_01") != std::string::npos, "summary includes weapon");
    expect(summary.find("ammo=29") != std::string::npos, "summary includes ammo");
    expect(summary.find("tech=energy-step") != std::string::npos, "summary includes movement tech cue");
    expect(summary.find("fire=yes") != std::string::npos, "summary includes fire result");
    expect(summary.find("pending=3") != std::string::npos, "summary includes pending commands");
    expect(summary.find("predStored=7") != std::string::npos, "summary includes prediction history sample count");
    expect(summary.find("predAck=40") != std::string::npos, "summary includes prediction acknowledgement tick");
    expect(summary.find("predSpan=2") != std::string::npos, "summary includes prediction unacknowledged span");
    expect(summary.find("predErr=0.08") != std::string::npos, "summary includes latest prediction error");
    expect(summary.find("predFix=no") != std::string::npos, "summary includes prediction correction state");
    expect(summary.find("snapStored=3") != std::string::npos, "summary includes snapshot interpolation count");
    expect(summary.find("snapNewest=44") != std::string::npos, "summary includes newest snapshot tick");
    expect(summary.find("drillVariant=PRECISION") != std::string::npos, "summary includes drill variant");
    expect(summary.find("drillObjective=ACCURACY") != std::string::npos, "summary includes drill objective");
    expect(summary.find("drillScore=900") != std::string::npos, "summary includes drill score");
    expect(summary.find("recoilControl=82") != std::string::npos, "summary includes recoil control score");
    expect(summary.find("latestTtk=0.42") != std::string::npos, "summary includes latest TTK");
    expect(summary.find("targetsAlive=4/4") != std::string::npos, "summary includes target lane count");
    expect(summary.find("targetHp=122.0") != std::string::npos, "summary includes target health");
    expect(summary.find("hit=yes") != std::string::npos, "summary includes target hit");
    expect(summary.find("wallrunSurface=yes") != std::string::npos, "summary includes wallrun surface state");
    expect(summary.find("mantle=ledge_training_mid") != std::string::npos, "summary includes mantle candidate id");
    expect(summary.find("wall=wallrun_left_panel_a") != std::string::npos, "summary includes wall contact id");
    expect(summary.find("contacts=2") != std::string::npos, "summary includes contact count");
    expect(summary.find("contactRoles=G1/S0/W1/B0/X0") != std::string::npos, "summary includes contact role breakdown");
    expect(summary.find("swept=yes") != std::string::npos, "summary includes sweep enabled state");
    expect(summary.find("sweepHit=cover_left_mid") != std::string::npos, "summary includes sweep hit id");
    expect(summary.find("sweepIterations=2") != std::string::npos, "summary includes sweep iteration count");
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
