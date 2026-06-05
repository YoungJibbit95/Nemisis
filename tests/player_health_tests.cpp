#include "nemisis/player/PlayerHealth.hpp"

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

void testBodyDamageAndReset() {
    nemisis::player::PlayerHealthComponent health{};
    health.maxHealth = 150.0F;
    health.health = 75.0F;
    nemisis::player::resetHealth(health);

    expect(health.health == 150.0F, "reset restores max health");
    expect(nemisis::player::isAlive(health), "reset player is alive");

    const auto result = nemisis::player::applyDamage(
        health,
        nemisis::player::DamageRequest{30.0F, nemisis::player::DamageZone::Body});
    expect(result.damageApplied == 30.0F, "body damage applies directly");
    expect(result.healthRemaining == 120.0F, "body damage lowers health");
    expect(!result.eliminated, "nonlethal body damage does not eliminate");
}

void testHeadAndLegMultipliers() {
    nemisis::player::PlayerHealthComponent health{};

    const auto head = nemisis::player::applyDamage(
        health,
        nemisis::player::DamageRequest{50.0F, nemisis::player::DamageZone::Head, 1.5F});
    expect(head.critical, "head damage reports critical");
    expect(head.damageApplied == 75.0F, "head multiplier applies");

    const auto legs = nemisis::player::applyDamage(
        health,
        nemisis::player::DamageRequest{20.0F, nemisis::player::DamageZone::Legs, 1.0F, 0.5F});
    expect(legs.damageApplied == 10.0F, "leg multiplier applies");
    expect(legs.healthRemaining == 65.0F, "leg damage lowers remaining health");
}

void testEliminationClampsDamage() {
    nemisis::player::PlayerHealthComponent health{};
    health.health = 24.0F;

    const auto result = nemisis::player::applyDamage(
        health,
        nemisis::player::DamageRequest{80.0F, nemisis::player::DamageZone::Body});
    expect(result.damageApplied == 24.0F, "lethal damage clamps to remaining health");
    expect(result.healthRemaining == 0.0F, "lethal damage reaches zero");
    expect(result.eliminated, "lethal damage eliminates");
    expect(!nemisis::player::isAlive(health), "eliminated player is not alive");

    const auto after = nemisis::player::applyDamage(
        health,
        nemisis::player::DamageRequest{10.0F, nemisis::player::DamageZone::Body});
    expect(after.damageApplied == 0.0F, "dead player ignores extra damage");
}

} // namespace

int main() {
    testBodyDamageAndReset();
    testHeadAndLegMultipliers();
    testEliminationClampsDamage();

    if (failures > 0) {
        std::cerr << failures << " player health test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis player health tests passed\n";
    return EXIT_SUCCESS;
}
