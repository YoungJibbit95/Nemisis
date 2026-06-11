#include "nemisis/weapons/WeaponShot.hpp"

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

nemisis::weapons::WeaponDefinition testWeapon() {
    nemisis::weapons::WeaponDefinition weapon{};
    weapon.id = "trace_ar";
    weapon.displayName = "Trace AR";
    weapon.maxRangeMeters = 90.0F;
    weapon.hipSpreadDegrees = 1.2F;
    weapon.adsSpreadDegrees = 0.2F;
    weapon.recoilPitchPerShotDegrees = 0.08F;
    weapon.recoilYawPerShotDegrees = 0.03F;
    weapon.damage = nemisis::weapons::DamageProfile{28.0F, 24.0F, 19.0F, 1.35F};
    return weapon;
}

void testShotTraceIsDeterministic() {
    const auto weapon = testWeapon();
    nemisis::weapons::ShotTraceRequest request{};
    request.seed = 1234;
    request.shotIndex = 5;
    request.movementSpeed = 6.0F;

    const auto first = nemisis::weapons::buildShotTrace(weapon, request);
    const auto second = nemisis::weapons::buildShotTrace(weapon, request);

    expectNear(first.direction.x, second.direction.x, 0.00001F, "same seed keeps direction x deterministic");
    expectNear(first.direction.y, second.direction.y, 0.00001F, "same seed keeps direction y deterministic");
    expectNear(first.direction.z, second.direction.z, 0.00001F, "same seed keeps direction z deterministic");
    expect(first.seed == request.seed, "shot trace keeps seed");
    expect(first.shotIndex == request.shotIndex, "shot trace keeps shot index");
}

void testShotTraceUsesWeaponRangeAndDamage() {
    const auto weapon = testWeapon();
    nemisis::weapons::ShotTraceRequest request{};

    const auto trace = nemisis::weapons::buildShotTrace(weapon, request);

    expectNear(trace.rangeMeters, weapon.maxRangeMeters, 0.001F, "shot trace uses max range");
    expectNear(trace.damage, weapon.damage.closeDamage, 0.001F, "shot trace uses close damage for first slice");
}

void testAdsHasLessSpreadThanHipFire() {
    const auto weapon = testWeapon();
    nemisis::weapons::ShotTraceRequest request{};
    request.seed = 77;
    request.movementSpeed = 0.0F;

    const auto hip = nemisis::weapons::buildShotTrace(weapon, request);
    request.ads = true;
    request.adsAlpha = 1.0F;
    const auto ads = nemisis::weapons::buildShotTrace(weapon, request);

    expect(ads.spreadDegrees < hip.spreadDegrees, "ads trace has less spread than hip fire");
}

void testMovementAndAirborneIncreaseSpread() {
    const auto weapon = testWeapon();
    nemisis::weapons::ShotTraceRequest request{};
    request.seed = 91;
    request.adsAlpha = 1.0F;

    const auto still = nemisis::weapons::buildShotTrace(weapon, request);
    request.movementSpeed = 8.0F;
    request.airborne = true;
    request.sprinting = true;
    const auto moving = nemisis::weapons::buildShotTrace(weapon, request);

    expect(moving.spreadDegrees > still.spreadDegrees, "movement, sprint, and airborne state increase spread");
}

void testRuntimeRecoilOverridesFallback() {
    const auto weapon = testWeapon();
    nemisis::weapons::ShotTraceRequest request{};
    request.shotIndex = 10;
    request.recoilPitchDegrees = 1.2F;
    request.recoilYawDegrees = -0.6F;

    const auto trace = nemisis::weapons::buildShotTrace(weapon, request);

    expectNear(trace.recoilPitchDegrees, 1.2F, 0.0001F, "runtime pitch recoil is preserved");
    expectNear(trace.recoilYawDegrees, -0.6F, 0.0001F, "runtime yaw recoil is preserved");
}

void testDirectionIsNormalized() {
    const auto weapon = testWeapon();
    nemisis::weapons::ShotTraceRequest request{};
    request.seed = 999;
    request.shotIndex = 12;

    const auto trace = nemisis::weapons::buildShotTrace(weapon, request);
    const float length = std::sqrt(trace.direction.lengthSquared());

    expectNear(length, 1.0F, 0.001F, "shot trace direction is normalized");
}

} // namespace

int main() {
    testShotTraceIsDeterministic();
    testShotTraceUsesWeaponRangeAndDamage();
    testAdsHasLessSpreadThanHipFire();
    testMovementAndAirborneIncreaseSpread();
    testRuntimeRecoilOverridesFallback();
    testDirectionIsNormalized();

    if (failures > 0) {
        std::cerr << failures << " weapon shot test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis weapon shot tests passed\n";
    return EXIT_SUCCESS;
}
