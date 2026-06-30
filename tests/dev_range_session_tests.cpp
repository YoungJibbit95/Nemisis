#include "nemisis/dev/DevRangeSession.hpp"

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

nemisis::weapons::FireResult firedShot() {
    nemisis::weapons::FireResult fire{};
    fire.fired = true;
    fire.ammoInMagazine = 29;
    fire.shotIndex = 1;
    return fire;
}

nemisis::dev::DevRangeShotScoreContext centerLaneContext() {
    nemisis::dev::DevRangeShotScoreContext context{};
    context.laneKnown = true;
    context.laneIndex = 1U;
    context.laneId = "center_20m";
    context.laneName = "CENTER 20M";
    context.targetMaxHealth = 150.0F;
    context.distanceMeters = 19.25F;
    return context;
}

void testScoreAndAccuracy() {
    nemisis::dev::DevRangeSessionState session{};

    nemisis::dev::DebugTargetHitResult miss{};
    nemisis::dev::recordShotResult(session, firedShot(), miss);
    expect(session.score.shotsFired == 1U, "miss records fired shot");
    expect(session.score.shotsHit == 0U, "miss does not record hit");
    expect(session.score.currentStreak == 0U, "miss clears streak");

    nemisis::dev::DebugTargetHitResult hit{};
    hit.hit = true;
    hit.damageApplied = 28.0F;
    hit.healthRemaining = 122.0F;
    nemisis::dev::recordShotResult(session, firedShot(), hit);
    expect(session.score.shotsFired == 2U, "hit records fired shot");
    expect(session.score.shotsHit == 1U, "hit records hit");
    expect(session.score.damageDealt == 28.0F, "hit records damage dealt");
    expect(session.score.currentStreak == 1U, "hit increments streak");
    expect(session.score.bestStreak == 1U, "hit updates best streak");
    expect(nemisis::dev::devRangeAccuracy(session.score) > 0.49F && nemisis::dev::devRangeAccuracy(session.score) < 0.51F, "accuracy is hits over shots");

    hit.eliminated = true;
    hit.damageApplied = 122.0F;
    hit.healthRemaining = 0.0F;
    nemisis::dev::recordShotResult(session, firedShot(), hit);
    expect(session.score.targetsEliminated == 1U, "elimination increments target count");
    expect(session.score.currentStreak == 2U, "elimination keeps hit streak");
    expect(!session.eventText.empty(), "session stores feedback text");
}

void testRespawnAndResetTimers() {
    nemisis::dev::DevRangeSessionState session{};
    nemisis::dev::DevRangeSessionTuning tuning{};
    tuning.targetRespawnDelaySeconds = 0.5F;

    nemisis::dev::beginTargetRespawn(session, tuning);
    expect(session.targetRespawnSeconds > 0.49F, "target respawn starts");
    expect(!nemisis::dev::tickTargetRespawn(session, 0.25F), "target respawn waits before delay");
    expect(nemisis::dev::tickTargetRespawn(session, 0.25F), "target respawn completes after delay");

    nemisis::dev::recordRangeReset(session, tuning);
    expect(session.score.rangeResets == 1U, "range reset increments count");
    expect(session.score.currentStreak == 0U, "range reset clears streak");
    expect(session.targetRespawnSeconds == 0.0F, "range reset clears target respawn timer");
}

void testPlayerDamageRegenAndRespawn() {
    nemisis::dev::DevRangeSessionState session{};
    nemisis::dev::DevRangeSessionTuning tuning{};
    tuning.playerRespawnDelaySeconds = 0.25F;
    tuning.playerRegenDelaySeconds = 0.1F;
    tuning.playerRegenPerSecond = 10.0F;

    nemisis::player::PlayerHealthComponent health{};
    health.health = 100.0F;
    nemisis::player::DamageResult damage{};
    damage.damageApplied = 20.0F;
    damage.healthRemaining = 100.0F;
    nemisis::dev::recordPlayerDamage(session, damage, tuning);
    expect(session.playerRegenDelaySeconds > 0.09F, "player damage starts regen delay");

    nemisis::dev::tickPlayerRegen(session, health, 0.05F, tuning);
    expect(health.health == 100.0F, "regen waits for delay");
    nemisis::dev::tickPlayerRegen(session, health, 0.15F, tuning);
    expect(health.health > 100.0F, "regen heals after delay");

    nemisis::dev::beginPlayerRespawn(session, health, tuning);
    expect(health.eliminated, "begin player respawn marks eliminated");
    expect(!nemisis::dev::tickPlayerRespawn(session, health, 0.1F), "player respawn waits before delay");
    expect(nemisis::dev::tickPlayerRespawn(session, health, 0.2F), "player respawn completes after delay");
    expect(health.health == health.maxHealth, "player respawn restores health");
    expect(session.score.playerRespawns == 1U, "player respawn increments count");
}

void testTimedDrillLaneTtkAndRecoilScoring() {
    nemisis::dev::DevRangeSessionState session{};
    nemisis::dev::DevRangeSessionTuning tuning{};
    tuning.drillTimeLimitSeconds = 12.0F;
    nemisis::dev::recordRangeReset(session, tuning);

    nemisis::dev::tickDevRangeDrill(session, 0.15F, tuning);

    auto firstFire = firedShot();
    firstFire.recoilPitchOffsetDegrees = 0.35F;
    firstFire.recoilYawOffsetDegrees = 0.12F;
    firstFire.movementSpreadDegrees = 0.20F;

    nemisis::dev::DebugTargetHitResult firstHit{};
    firstHit.hit = true;
    firstHit.damageApplied = 50.0F;
    firstHit.healthRemaining = 100.0F;
    firstHit.distanceMeters = 19.25F;
    nemisis::dev::recordShotResult(session, firstFire, firstHit, centerLaneContext(), tuning);

    nemisis::dev::tickDevRangeDrill(session, 0.20F, tuning);

    auto finalFire = firedShot();
    finalFire.shotIndex = 2U;
    finalFire.recoilPitchOffsetDegrees = 0.55F;
    finalFire.recoilYawOffsetDegrees = -0.18F;
    finalFire.movementSpreadDegrees = 0.16F;

    nemisis::dev::DebugTargetHitResult elimination{};
    elimination.hit = true;
    elimination.eliminated = true;
    elimination.damageApplied = 100.0F;
    elimination.healthRemaining = 0.0F;
    elimination.distanceMeters = 19.25F;
    nemisis::dev::recordShotResult(session, finalFire, elimination, centerLaneContext(), tuning);

    const auto* lane = nemisis::dev::activeDevRangeLaneScore(session, 1U);
    expect(lane != nullptr, "drill creates lane score entry");
    expect(lane != nullptr && lane->shotsFired == 2U, "lane score records target shots");
    expect(lane != nullptr && lane->shotsHit == 2U, "lane score records target hits");
    expect(lane != nullptr && lane->targetsEliminated == 1U, "lane score records lane eliminations");
    expect(lane != nullptr && lane->latestTtkSeconds > 0.19F && lane->latestTtkSeconds < 0.21F, "lane score records measured TTK from first hit to elimination");
    expect(lane != nullptr && lane->bestTtkSeconds == lane->latestTtkSeconds, "lane score records best TTK");
    expect(session.drill.targetsEliminated == 1U, "drill records eliminations");
    expect(session.drill.perfectLaneClears == 1U, "drill records perfect lane clear");
    expect(session.drill.score > 0U, "drill accumulates score");
    expect(session.drill.bestScore == session.drill.score, "drill best score tracks current score");
    expect(session.drill.recoilControlScore < 100.0F, "drill recoil control reacts to recoil and spread");
    expect(session.drill.recoilHud.valid, "drill exposes latest recoil HUD sample");
    expect(session.drill.recoilHud.shotIndex == 2U, "recoil HUD tracks latest shot index");
    expect(session.drill.recoilHud.hit, "recoil HUD tracks hit state");
    expect(session.drill.recoilHud.pitchDegrees > 0.54F && session.drill.recoilHud.pitchDegrees < 0.56F, "recoil HUD tracks pitch offset");
    expect(session.drill.scoring.scoreMultiplier > 1.0F, "drill exposes active score multiplier");
    expect(session.drill.scoring.latestScoreDelta > 0, "drill exposes latest score delta");
    expect(session.drill.scoring.latestScaledPoints >= session.drill.scoring.latestRawPoints, "drill exposes scaled score points");
    expect(nemisis::dev::devRangeDrillAccuracy(session.drill) == 1.0F, "drill accuracy uses drill shots");
}

void testDrillTimerCompletesAndResetRestarts() {
    nemisis::dev::DevRangeSessionState session{};
    nemisis::dev::DevRangeSessionTuning tuning{};
    tuning.drillTimeLimitSeconds = 1.0F;
    tuning.eventTextSeconds = 0.5F;

    nemisis::dev::recordRangeReset(session, tuning);
    expect(session.drill.timeRemainingSeconds > 0.99F, "range reset starts timed drill");

    nemisis::dev::tickDevRangeDrill(session, 0.45F, tuning);
    expect(session.drill.status == nemisis::dev::DevRangeDrillStatus::Active, "drill remains active before time limit");
    nemisis::dev::tickDevRangeDrill(session, 0.60F, tuning);
    expect(session.drill.status == nemisis::dev::DevRangeDrillStatus::Complete, "drill completes at time limit");
    expect(session.drill.timeRemainingSeconds == 0.0F, "completed drill has no remaining time");
    expect(session.eventText.find("Drill complete") != std::string::npos, "drill completion writes event text");

    session.drill.score = 500U;
    session.drill.bestScore = 500U;
    nemisis::dev::recordRangeReset(session, tuning);
    expect(session.drill.status == nemisis::dev::DevRangeDrillStatus::Active, "range reset restarts drill");
    expect(session.drill.score == 0U, "range reset clears current drill score");
    expect(session.drill.bestScore == 500U, "range reset preserves best drill score");
}

void testDrillVariantsChangeRulesAndPreserveBestScores() {
    nemisis::dev::DevRangeSessionState session{};
    nemisis::dev::DevRangeSessionTuning tuning{};

    nemisis::dev::recordRangeReset(session, tuning);
    expect(session.drill.variant == nemisis::dev::DevRangeDrillVariant::Precision, "default drill starts in precision variant");
    expect(session.drill.timeLimitSeconds > 69.9F && session.drill.timeLimitSeconds < 70.1F, "precision drill uses precision time limit");
    expect(nemisis::dev::devRangeDrillVariantName(session.drill.variant) == "PRECISION", "precision variant exposes HUD name");
    expect(nemisis::dev::devRangeDrillObjectiveLabel(session.drill.variant) == "ACCURACY", "precision variant exposes objective label");

    session.drill.score = 420U;
    session.drill.bestScore = 420U;
    nemisis::dev::cycleDevRangeDrillVariant(session, tuning);
    expect(session.drill.variant == nemisis::dev::DevRangeDrillVariant::RecoilControl, "cycling moves precision drill to recoil control");
    expect(session.drill.timeLimitSeconds > 59.9F && session.drill.timeLimitSeconds < 60.1F, "recoil-control drill uses control time limit");
    expect(session.drill.score == 0U, "variant switch restarts current drill score");
    expect(session.drill.bestScoreByVariant[0] == 420U, "variant switch preserves precision best score");
    expect(session.drill.bestScoreByVariant[1] == 0U, "variant switch does not leak precision score into recoil-control best");
    expect(session.eventText.find("RECOIL CONTROL") != std::string::npos, "variant switch writes HUD feedback");

    auto fire = firedShot();
    fire.recoilPitchOffsetDegrees = 2.0F;
    fire.recoilYawOffsetDegrees = 1.0F;
    nemisis::dev::DebugTargetHitResult miss{};
    nemisis::dev::recordShotResult(session, fire, miss, tuning);
    expect(session.drill.score == 0U, "recoil-control miss penalty cannot underflow score");
    expect(session.drill.scoring.latestPenaltyPoints == 0.0F, "miss penalty reports applied points and avoids underflow");
    expect(session.drill.scoring.latestScoreDelta == 0, "miss score delta reflects clamped score");

    nemisis::dev::cycleDevRangeDrillVariant(session, tuning);
    expect(session.drill.variant == nemisis::dev::DevRangeDrillVariant::SpeedClear, "cycling moves recoil control drill to speed clear");
    expect(session.drill.timeLimitSeconds > 41.9F && session.drill.timeLimitSeconds < 42.1F, "speed-clear drill uses shorter time limit");
    expect(nemisis::dev::devRangeDrillObjectiveLabel(session.drill.variant) == "TTK", "speed-clear drill exposes TTK objective");
    expect(nemisis::dev::devRangeDrillRules(nemisis::dev::DevRangeDrillVariant::RecoilControl).recoilMultiplierScale > nemisis::dev::devRangeDrillRules(nemisis::dev::DevRangeDrillVariant::SpeedClear).recoilMultiplierScale, "recoil-control drill weights recoil multiplier more than speed clear");
    expect(nemisis::dev::devRangeDrillRules(nemisis::dev::DevRangeDrillVariant::SpeedClear).speedMultiplierScale > nemisis::dev::devRangeDrillRules(nemisis::dev::DevRangeDrillVariant::Precision).speedMultiplierScale, "speed-clear drill weights timer multiplier more than precision");
}

void testVariantMultiplierBreakdownIsDeterministic() {
    nemisis::dev::DevRangeSessionState session{};
    nemisis::dev::DevRangeSessionTuning tuning{};
    nemisis::dev::setDevRangeDrillVariant(session, nemisis::dev::DevRangeDrillVariant::SpeedClear, tuning);
    nemisis::dev::tickDevRangeDrill(session, 4.2F, tuning);

    auto fire = firedShot();
    fire.recoilPitchOffsetDegrees = 0.20F;
    fire.recoilYawOffsetDegrees = 0.05F;
    fire.movementSpreadDegrees = 0.10F;

    nemisis::dev::DebugTargetHitResult hit{};
    hit.hit = true;
    hit.damageApplied = 45.0F;
    hit.healthRemaining = 60.0F;
    hit.distanceMeters = 16.0F;
    nemisis::dev::recordShotResult(session, fire, hit, centerLaneContext(), tuning);

    expect(session.drill.scoring.accuracyFactor == 1.0F, "multiplier telemetry records drill accuracy factor");
    expect(session.drill.scoring.speedFactor > 0.89F && session.drill.scoring.speedFactor < 0.91F, "multiplier telemetry records timer factor");
    expect(session.drill.scoring.recoilFactor > 0.89F, "multiplier telemetry records recoil-control factor");
    expect(session.drill.scoring.streakFactor > 0.09F && session.drill.scoring.streakFactor < 0.11F, "multiplier telemetry records streak factor");
    expect(nemisis::dev::devRangeDrillScoreMultiplier(session.drill) > 1.58F, "speed-clear hit receives deterministic multiplier");
    expect(nemisis::dev::devRangeRecoilControl01(session.drill) == session.drill.recoilHud.control01, "recoil HUD exposes normalized control value");
}

} // namespace

int main() {
    testScoreAndAccuracy();
    testRespawnAndResetTimers();
    testPlayerDamageRegenAndRespawn();
    testTimedDrillLaneTtkAndRecoilScoring();
    testDrillTimerCompletesAndResetRestarts();
    testDrillVariantsChangeRulesAndPreserveBestScores();
    testVariantMultiplierBreakdownIsDeterministic();

    if (failures > 0) {
        std::cerr << failures << " dev range session test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis dev range session tests passed\n";
    return EXIT_SUCCESS;
}
