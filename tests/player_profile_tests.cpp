#include "nemisis/player/PlayerProfile.hpp"

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

void testPrototypeAccountStatsAreUsefulForUi() {
    const auto stats = nemisis::player::prototypeAccountStats();

    expect(stats.level > 1U, "prototype account has a level");
    expect(stats.matchesPlayed > 0U, "prototype account has matches");
    expect(stats.bestWeapon.weaponId == "ar_01", "prototype best weapon is linked to AR id");
    expect(stats.bestOperator.operatorId == "pilot_vanguard", "prototype best operator has stable id");
    expect(nemisis::player::killDeathRatio(stats) > 1.0F, "prototype account has positive K/D");
    expect(nemisis::player::winRate(stats) > 0.0F, "prototype account has positive win rate");
    expect(nemisis::player::weaponAccuracy(stats.bestWeapon) > 0.0F, "prototype best weapon has accuracy");
}

void testStatsHandleZeroDenominators() {
    nemisis::player::AccountStats stats{};
    stats.eliminations = 5;
    stats.deaths = 0;
    stats.matchesPlayed = 0;

    nemisis::player::WeaponStatLine weapon{};
    weapon.shotsFired = 0;
    weapon.shotsHit = 10;

    expect(nemisis::player::killDeathRatio(stats) == 5.0F, "zero deaths uses eliminations as K/D fallback");
    expect(nemisis::player::winRate(stats) == 0.0F, "zero matches gives zero win rate");
    expect(nemisis::player::weaponAccuracy(weapon) == 0.0F, "zero shots gives zero accuracy");
}

} // namespace

int main() {
    testPrototypeAccountStatsAreUsefulForUi();
    testStatsHandleZeroDenominators();

    if (failures > 0) {
        std::cerr << failures << " player profile test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis player profile tests passed\n";
    return EXIT_SUCCESS;
}
