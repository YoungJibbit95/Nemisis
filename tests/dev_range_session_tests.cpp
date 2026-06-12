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

} // namespace

int main() {
    testScoreAndAccuracy();
    testRespawnAndResetTimers();
    testPlayerDamageRegenAndRespawn();

    if (failures > 0) {
        std::cerr << failures << " dev range session test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis dev range session tests passed\n";
    return EXIT_SUCCESS;
}
