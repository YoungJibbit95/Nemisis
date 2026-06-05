#include "nemisis/weapons/WeaponMetrics.hpp"

#include <algorithm>
#include <cmath>

namespace nemisis::weapons {

DamageRangeBand damageBandForDistance(const WeaponDefinition& weapon, float distanceMeters) {
    const float range = std::max(1.0F, weapon.maxRangeMeters);
    const float clampedDistance = std::clamp(distanceMeters, 0.0F, range);
    if (clampedDistance <= range * 0.35F) {
        return DamageRangeBand::Close;
    }
    if (clampedDistance <= range * 0.70F) {
        return DamageRangeBand::Mid;
    }
    return DamageRangeBand::Long;
}

float damageAtDistance(const WeaponDefinition& weapon, float distanceMeters, bool headshot) {
    float damage = 0.0F;
    switch (damageBandForDistance(weapon, distanceMeters)) {
    case DamageRangeBand::Close:
        damage = weapon.damage.closeDamage;
        break;
    case DamageRangeBand::Mid:
        damage = weapon.damage.midDamage;
        break;
    case DamageRangeBand::Long:
        damage = weapon.damage.longDamage;
        break;
    }

    if (headshot) {
        damage *= weapon.damage.headMultiplier;
    }
    return std::max(0.0F, damage);
}

std::uint32_t shotsToEliminate(
    const WeaponDefinition& weapon,
    float targetHealth,
    float distanceMeters,
    bool headshot) {
    const float damage = damageAtDistance(weapon, distanceMeters, headshot);
    if (targetHealth <= 0.0F || damage <= 0.0F) {
        return 0;
    }
    return static_cast<std::uint32_t>(std::ceil(targetHealth / damage));
}

TtkEstimate estimateTimeToKill(
    const WeaponDefinition& weapon,
    float targetHealth,
    float distanceMeters,
    bool headshot) {
    TtkEstimate estimate{};
    estimate.damagePerShot = damageAtDistance(weapon, distanceMeters, headshot);
    estimate.shotsToEliminate = shotsToEliminate(weapon, targetHealth, distanceMeters, headshot);

    if (estimate.shotsToEliminate <= 1 || weapon.fireRateRpm == 0) {
        estimate.seconds = 0.0F;
        return estimate;
    }

    const float secondsPerShot = 60.0F / static_cast<float>(weapon.fireRateRpm);
    estimate.seconds = static_cast<float>(estimate.shotsToEliminate - 1U) * secondsPerShot;
    return estimate;
}

} // namespace nemisis::weapons
