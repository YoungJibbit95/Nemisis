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

void testShotSelectsAimedLane() {
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
    range.lanes[0].pressure01 = 1.0F;
    range.lanes[0].threatSeconds = 2.0F;
    range.lanes[0].pressureActive = true;
    range.lanes[2].target.health = 60.0F;
    range.activeLaneIndex = 99U;

    nemisis::dev::resetDevTargetRange(range);

    expect(range.activeLaneIndex < range.lanes.size(), "reset clamps active lane");
    expect(nemisis::dev::aliveTargetCount(range) == range.lanes.size(), "reset revives all lanes");
    expect(nemisis::dev::nextTargetRespawnSeconds(range) == 0.0F, "reset clears respawn timers");
    expect(range.lanes[0].pressure01 == 0.0F, "reset clears lane pressure");
    expect(!range.lanes[0].pressureActive, "reset clears lane pressure active state");
    expect(range.lanes[2].target.health == range.lanes[2].target.maxHealth, "reset restores damaged lane health");
}

void testLanePressureBuildsAndDecays() {
    auto range = nemisis::dev::makeDefaultDevTargetRange();
    const auto centerPosition = range.lanes[1].target.position;

    nemisis::dev::updateDevTargetRangePressure(range, centerPosition + novacore::math::Vec3{0.0F, 0.0F, -2.0F}, 0.25F);
    const auto pressure = nemisis::dev::strongestDevTargetRangePressure(range);

    expect(pressure.active, "near target player activates lane pressure");
    expect(pressure.laneId == "center_20m", "active center lane produces strongest pressure");
    expect(pressure.pressure01 > 0.60F, "lane pressure rises near an active target");
    expect(pressure.damagePerSecond > 0.0F, "high lane pressure exposes player damage budget");

    range.lanes[pressure.laneIndex].target.eliminated = true;
    const auto eliminatedLaneIndex = pressure.laneIndex;
    nemisis::dev::updateDevTargetRangePressure(range, centerPosition + novacore::math::Vec3{0.0F, 0.0F, -2.0F}, 1.0F);
    expect(range.lanes[eliminatedLaneIndex].pressure01 < pressure.pressure01, "eliminated lane pressure decays");
}

void testVisibleShootingLanesAndCover() {
    using namespace nemisis::dev;
    const auto world = createDevRangeGreyboxWorld();
    auto range = makeDefaultDevTargetRange();
    for (std::size_t lane = 0; lane < range.lanes.size(); ++lane) {
        auto shot = aimedShot(range.lanes[lane].target.position);
        shot.origin = world.playerSpawn + novacore::math::Vec3{0.0F, 1.65F, 0.0F};
        const auto delta = range.lanes[lane].target.position - shot.origin;
        shot.direction = delta * (1.0F / std::sqrt(delta.lengthSquared()));
        const auto contact = traceDevRangeShot(range, shot, world.primitives);
        expect(contact.targetLane == lane, "each default target is visible from the standing spawn eye");
    }

    range.lanes.resize(1);
    auto& target = range.lanes.front().target;
    target.position = {8.0F, 0.8F, 10.0F};
    target.radiusMeters = 0.5F;
    nemisis::weapons::ShotTraceResult shot{};
    shot.origin = {8.0F, 0.8F, 0.0F};
    shot.direction = {0.0F, 0.0F, 1.0F};
    shot.rangeMeters = 30.0F;
    shot.damage = 20.0F;
    const auto blocked = traceDevRangeShot(range, shot, world.primitives);
    expect(!blocked.targetLane && blocked.distanceMeters == 3.0F, "the visible right-hand cover stops the tracer at its near face");
    const auto health = target.health;
    expect(!applyShotToDevTargetRange(range, shot, blocked).hit && target.health == health,
           "cover prevents damage and target-hit feedback");
    target.position.z = 2.0F;
    const auto inFront = traceDevRangeShot(range, shot, world.primitives);
    expect(inFront.targetLane == 0U && inFront.distanceMeters == 1.5F, "the target wins when it stands in front of cover");
    shot.rangeMeters = 1.49F;
    expect(!traceDevRangeShot(range, shot, world.primitives).targetLane, "weapon range ends before the target surface");
    shot.rangeMeters = 1.5F;
    expect(traceDevRangeShot(range, shot, world.primitives).targetLane == 0U, "the exact range endpoint is hittable");
}

} // namespace

int main() {
    testDefaultRangeHasMultipleLanes();
    testShotSelectsAimedLane();
    testLaneEliminatesAndRespawnsIndependently();
    testResetRestoresAllLanes();
    testLanePressureBuildsAndDecays();
    testVisibleShootingLanesAndCover();

    if (failures > 0) {
        std::cerr << failures << " dev target range test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Dev target range tests passed\n";
    return EXIT_SUCCESS;
}
