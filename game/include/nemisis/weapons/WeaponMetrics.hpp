#pragma once

#include "nemisis/weapons/WeaponTypes.hpp"

#include <cstdint>

namespace nemisis::weapons {

enum class DamageRangeBand {
    Close,
    Mid,
    Long
};

struct TtkEstimate final {
    std::uint32_t shotsToEliminate = 0;
    float seconds = 0.0F;
    float damagePerShot = 0.0F;
};

[[nodiscard]] DamageRangeBand damageBandForDistance(const WeaponDefinition& weapon, float distanceMeters);
[[nodiscard]] float damageAtDistance(const WeaponDefinition& weapon, float distanceMeters, bool headshot = false);
[[nodiscard]] std::uint32_t shotsToEliminate(
    const WeaponDefinition& weapon,
    float targetHealth,
    float distanceMeters,
    bool headshot = false);
[[nodiscard]] TtkEstimate estimateTimeToKill(
    const WeaponDefinition& weapon,
    float targetHealth,
    float distanceMeters,
    bool headshot = false);

} // namespace nemisis::weapons
