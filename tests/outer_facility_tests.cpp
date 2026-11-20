#include "nemisis/dev/OuterFacility.hpp"
#include "nemisis/dev/GreyboxCollision.hpp"
#include "nemisis/weapons/WeaponShot.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
int failures = 0;
void expect(bool condition, std::string_view message) {
    if (!condition) { ++failures; std::cerr << "[fail] " << message << '\n'; }
}
nemisis::dev::GreyboxCollisionQuery queryAt(novacore::math::Vec3 p) {
    nemisis::dev::GreyboxCollisionQuery q{};
    q.position = p; q.previousPosition = p;
    return q;
}
void testRoutesAndFire() {
    using namespace nemisis::dev;
    const auto world = createOuterFacilityWorld();
    const auto targets = makeOuterFacilityTargets();
    const auto physics = buildGreyboxPhysicsWorld(world);
    expect(world.id == kOuterFacilityWorldId && world.playerSpawn.z == -26.0F, "facility has its own identity and spawn");
    expect(targets.lanes.size() == 3 && targets.activeLaneIndex == 0, "facility owns three active target positions");
    for (const auto p : {world.playerSpawn,
            novacore::math::Vec3{-14,0,-9}, {-14,0,-7}, {-14,0,2}, {-14,0,11}, {-14,0,13},
            {17,0,5}, {17,0,7}, {17,0,14}, {17,0,21}, {17,0,23}}) {
        const auto collision = resolveGreyboxPlayerCollision(world, physics, queryAt(p));
        expect(collision.grounded && !collision.blocked, "spawn and both indoor routes are grounded and passable");
    }
    for (const auto center : kFacilityGroveCenters) {
        for (const auto offset : kFacilityTreeOffsets) {
            const auto p = center + offset;
            expect(std::abs(p.x) + 0.18F < world.boundsHalfExtents.x &&
                   std::abs(p.z) + 0.18F < world.boundsHalfExtents.z, "tree trunks stay inside map bounds");
            expect(resolveGreyboxPlayerCollision(world, physics, queryAt(p)).blocked,
                   "each visible tree trunk has movement collision");
        }
    }
    auto throughWest = queryAt({-14,0,-7});
    throughWest.previousPosition = {-14,0,-9};
    throughWest.useSweep = true;
    expect(!resolveGreyboxPlayerCollision(world, physics, throughWest).blocked,
           "west doorway admits a swept player capsule");
    auto throughEast = queryAt({17,0,7});
    throughEast.previousPosition = {17,0,5};
    throughEast.useSweep = true;
    expect(!resolveGreyboxPlayerCollision(world, physics, throughEast).blocked,
           "east doorway admits a swept player capsule");
    auto wallrun = queryAt({-21.7F,0.8F,0});
    wallrun.wallProbeDistance = 0.7F;
    wallrun.enableGroundSnap = false;
    const auto runnable = resolveGreyboxPlayerCollision(world, physics, wallrun);
    expect(runnable.nearWallRunSurface && runnable.wallPrimitiveId == "west_left",
           "long exterior face supplies wallrun contacts");
    auto position = novacore::math::Vec3{-4.8F,0,-8.2F};
    for (int i = 0; i < 53; ++i) {
        auto q = queryAt(position + novacore::math::Vec3{0,0,0.2F});
        q.previousPosition = position;
        q.useSweep = true;
        const auto result = resolveGreyboxPlayerCollision(world, physics, q);
        position = result.position;
    }
    expect(position.y > 3.1F && position.z > 1.7F,
           "continuous capsule movement can climb the eight steps");
    for (int i = 0; i < 20; ++i) {
        auto q = queryAt(position + novacore::math::Vec3{-0.2F,0,0});
        q.previousPosition = position;
        q.useSweep = true;
        position = resolveGreyboxPlayerCollision(world, physics, q).position;
    }
    expect(position.x < -7.3F && position.y > 3.1F,
           "roof landing is reachable from the upper step");
    const auto wall = resolveGreyboxPlayerCollision(world, physics, queryAt({-7,0,0}));
    expect(wall.blocked, "visible building wall blocks movement");
    const auto cover = resolveGreyboxPlayerCollision(world, physics, queryAt({5,0,5}));
    expect(cover.blocked, "visible courtyard cover blocks movement");
    nemisis::weapons::ShotTraceResult shot{};
    shot.origin = world.playerSpawn + novacore::math::Vec3{0,1.65F,0};
    shot.direction = {0,0,1}; shot.rangeMeters = 100; shot.damage = 20;
    const auto center = traceDevRangeShot(targets, shot, world.primitives);
    expect(center.targetLane == 0, "center target has an unobstructed spawn firing line");
    shot.origin = {5,0.8F,0}; shot.direction = {0,0,1};
    const auto blocked = traceDevRangeShot(targets, shot, world.primitives);
    expect(!blocked.targetLane && blocked.distanceMeters < 5.0F, "visible cover blocks shots");
    expect(world.primitives.size() == 59, "35 scene boxes and 24 tree collision boxes are present");
}
} // namespace
int main() {
    testRoutesAndFire();
    if (failures) return EXIT_FAILURE;
    std::cout << "Outer Facility tests passed\n";
    return EXIT_SUCCESS;
}
