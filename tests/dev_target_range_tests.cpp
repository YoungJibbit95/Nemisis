#include "nemisis/dev/DevTargetRange.hpp"

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

nemisis::weapons::ShotTraceResult aimedShot(novacore::math::Vec3 target, float damage = 50.0F) {
    const novacore::math::Vec3 origin{0.0F, 1.65F, -12.0F};
    const auto delta = target - origin;
    const float length = std::sqrt((delta.x * delta.x) + (delta.y * delta.y) + (delta.z * delta.z));

    nemisis::weapons::ShotTraceResult shot{};
    shot.origin = origin;
    shot.direction = {delta.x / length, delta.y / length, delta.z / length};
    shot.rangeMeters = 100.0F;
    shot.damage = damage;
    return shot;
}

void testDefaultRangeHasMultipleLanes() {
    const auto range = nemisis::dev::makeDefaultDevTargetRange();

    expect(nemisis::dev::totalTargetCount(range) == 4U, "default range creates four target lanes");
    expect(nemisis::dev::aliveTargetCount(range) == 4U, "default range starts with all lanes alive");
    expect(nemisis::dev::eliminatedTargetCount(range) == 0U, "default range starts with no eliminated lanes");
    expect(nemisis::dev::activeTargetLane(range) != nullptr, "default range exposes an active lane");
    expect(nemisis::dev::activeTarget(range) != nullptr, "default range exposes an active target");
}

void testShotSelectsNearestHitLane() {
    auto range = nemisis::dev::makeDefaultDevTargetRange();
    const auto center = range.lanes[1].target.position;

    const auto result = nemisis::dev::applyShotToDevTargetRange(range, aimedShot(center));

    expect(result.hit, "aimed shot hits one target lane");
    expect(result.laneId == "center_20m", "center aimed shot selects center lane");
    expect(range.activeLaneIndex == 1U, "hit lane becomes active lane");
    expect(range.lanes[1].target.health == 100.0F, "hit lane receives damage");
    expect(range.lanes[0].target.health == range.lanes[0].target.maxHealth, "unhit lane remains undamaged");
}

void testLaneEliminatesAndRespawnsIndependently() {
    auto range = nemisis::dev::makeDefaultDevTargetRange();
    const auto left = range.lanes[0].target.position;

    const auto result = nemisis::dev::applyShotToDevTargetRange(range, aimedShot(left, 250.0F));
    expect(result.hit, "lethal shot hits left lane");
    expect(result.targetHit.eliminated, "lethal shot eliminates lane target");
    expect(range.lanes[0].target.eliminated, "range stores eliminated target");
    expect(nemisis::dev::aliveTargetCount(range) == 3U, "only eliminated lane is down");

    nemisis::dev::beginTargetLaneRespawn(range, result.laneIndex, 0.5F);
    expect(nemisis::dev::nextTargetRespawnSeconds(range) > 0.49F, "respawn timer starts on eliminated lane");
    expect(nemisis::dev::tickDevTargetRangeRespawns(range, 0.25F) == 0U, "respawn waits before timer elapses");
    expect(range.lanes[0].target.eliminated, "target remains down before timer");
    expect(nemisis::dev::tickDevTargetRangeRespawns(range, 0.25F) == 1U, "respawn returns completed lane count");
    expect(!range.lanes[0].target.eliminated, "target is alive after respawn");
    expect(range.lanes[0].target.health == range.lanes[0].target.maxHealth, "respawn restores health");
    expect(nemisis::dev::aliveTargetCount(range) == 4U, "all lanes are alive after respawn");
}

void testResetRestoresAllLanes() {
    auto range = nemisis::dev::makeDefaultDevTargetRange();
    range.lanes[0].target.health = 1.0F;
    range.lanes[0].target.eliminated = true;
    range.lanes[0].respawnSeconds = 4.0F;
    range.lanes[2].target.health = 60.0F;
    range.activeLaneIndex = 99U;

    nemisis::dev::resetDevTargetRange(range);

    expect(range.activeLaneIndex < range.lanes.size(), "reset clamps active lane");
    expect(nemisis::dev::aliveTargetCount(range) == range.lanes.size(), "reset revives all lanes");
    expect(nemisis::dev::nextTargetRespawnSeconds(range) == 0.0F, "reset clears respawn timers");
    expect(range.lanes[2].target.health == range.lanes[2].target.maxHealth, "reset restores damaged lane health");
}

} // namespace

int main() {
    testDefaultRangeHasMultipleLanes();
    testShotSelectsNearestHitLane();
    testLaneEliminatesAndRespawnsIndependently();
    testResetRestoresAllLanes();

    if (failures > 0) {
        std::cerr << failures << " dev target range test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Dev target range tests passed\n";
    return EXIT_SUCCESS;
}
