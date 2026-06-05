#include "nemisis/weapons/WeaponMetrics.hpp"

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

bool near(float actual, float expected) {
    return std::abs(actual - expected) < 0.001F;
}

nemisis::weapons::WeaponDefinition prototypeAr() {
    nemisis::weapons::WeaponDefinition weapon{};
    weapon.id = "ar_01";
    weapon.displayName = "Prototype AR";
    weapon.fireRateRpm = 720;
    weapon.maxRangeMeters = 90.0F;
    weapon.damage.closeDamage = 28.0F;
    weapon.damage.midDamage = 24.0F;
    weapon.damage.longDamage = 19.0F;
    weapon.damage.headMultiplier = 1.35F;
    return weapon;
}

nemisis::weapons::WeaponDefinition prototypeSmg() {
    nemisis::weapons::WeaponDefinition weapon{};
    weapon.id = "smg_01";
    weapon.displayName = "Prototype SMG";
    weapon.fireRateRpm = 900;
    weapon.maxRangeMeters = 55.0F;
    weapon.damage.closeDamage = 23.0F;
    weapon.damage.midDamage = 18.0F;
    weapon.damage.longDamage = 14.0F;
    weapon.damage.headMultiplier = 1.25F;
    return weapon;
}

void testRangeBandsAndDamage() {
    const auto weapon = prototypeAr();

    expect(nemisis::weapons::damageBandForDistance(weapon, 10.0F) == nemisis::weapons::DamageRangeBand::Close, "close band");
    expect(nemisis::weapons::damageBandForDistance(weapon, 50.0F) == nemisis::weapons::DamageRangeBand::Mid, "mid band");
    expect(nemisis::weapons::damageBandForDistance(weapon, 80.0F) == nemisis::weapons::DamageRangeBand::Long, "long band");
    expect(nemisis::weapons::damageAtDistance(weapon, 10.0F) == 28.0F, "close damage");
    expect(near(nemisis::weapons::damageAtDistance(weapon, 10.0F, true), 37.8F), "headshot damage");
}

void testArTtkTargetsBoLikeBaseline() {
    const auto weapon = prototypeAr();
    const auto body = nemisis::weapons::estimateTimeToKill(weapon, 150.0F, 10.0F);
    const auto head = nemisis::weapons::estimateTimeToKill(weapon, 150.0F, 10.0F, true);
    const auto longRange = nemisis::weapons::estimateTimeToKill(weapon, 150.0F, 80.0F);

    expect(body.shotsToEliminate == 6, "AR close body takes six shots into 150 hp");
    expect(near(body.seconds, 5.0F * (60.0F / 720.0F)), "AR close body TTK uses fire rate spacing");
    expect(head.shotsToEliminate == 4, "AR close headshots reduce shot count");
    expect(head.seconds < body.seconds, "headshot TTK is faster");
    expect(longRange.shotsToEliminate > body.shotsToEliminate, "long range requires more shots");
}

void testSmgCloseTtk() {
    const auto weapon = prototypeSmg();
    const auto body = nemisis::weapons::estimateTimeToKill(weapon, 150.0F, 8.0F);

    expect(body.shotsToEliminate == 7, "SMG close body takes seven shots into 150 hp");
    expect(near(body.seconds, 6.0F * (60.0F / 900.0F)), "SMG close body TTK uses fire rate spacing");
}

} // namespace

int main() {
    testRangeBandsAndDamage();
    testArTtkTargetsBoLikeBaseline();
    testSmgCloseTtk();

    if (failures > 0) {
        std::cerr << failures << " weapon metrics test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis weapon metrics tests passed\n";
    return EXIT_SUCCESS;
}
