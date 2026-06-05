#include "nemisis/dev/DebugTarget.hpp"

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

nemisis::weapons::ShotTraceResult forwardShot(float damage = 50.0F) {
    nemisis::weapons::ShotTraceResult shot{};
    shot.origin = novacore::math::Vec3{0.0F, 1.65F, 0.0F};
    shot.direction = novacore::math::Vec3{0.0F, 0.0F, 1.0F};
    shot.rangeMeters = 100.0F;
    shot.damage = damage;
    return shot;
}

void testForwardShotHitsDebugTarget() {
    nemisis::dev::DebugTargetState target{};

    const auto result = nemisis::dev::applyShotToDebugTarget(target, forwardShot());

    expect(result.hit, "forward shot hits target ahead");
    expect(result.damageApplied == 50.0F, "hit applies shot damage");
    expect(target.health == 100.0F, "target health is reduced");
    expect(target.hitsTaken == 1, "target records hit count");
}

void testMissDoesNotDamageTarget() {
    nemisis::dev::DebugTargetState target{};
    auto shot = forwardShot();
    shot.direction = novacore::math::Vec3{1.0F, 0.0F, 0.0F};

    const auto result = nemisis::dev::applyShotToDebugTarget(target, shot);

    expect(!result.hit, "sideways shot misses target ahead");
    expect(target.health == target.maxHealth, "miss does not damage target");
}

void testTargetCanBeEliminatedAndReset() {
    nemisis::dev::DebugTargetState target{};

    const auto result = nemisis::dev::applyShotToDebugTarget(target, forwardShot(200.0F));

    expect(result.hit, "lethal shot hits");
    expect(result.eliminated, "lethal shot eliminates target");
    expect(target.eliminated, "target stores eliminated state");
    expect(target.health == 0.0F, "target health reaches zero");

    nemisis::dev::resetDebugTarget(target);
    expect(!target.eliminated, "reset clears eliminated state");
    expect(target.health == target.maxHealth, "reset restores health");
    expect(target.hitsTaken == 0, "reset clears hits");
}

} // namespace

int main() {
    testForwardShotHitsDebugTarget();
    testMissDoesNotDamageTarget();
    testTargetCanBeEliminatedAndReset();

    if (failures > 0) {
        std::cerr << failures << " debug target test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis debug target tests passed\n";
    return EXIT_SUCCESS;
}
