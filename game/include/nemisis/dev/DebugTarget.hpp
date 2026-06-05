#pragma once

#include "nemisis/weapons/WeaponShot.hpp"

#include "novacore/math/Types.hpp"

#include <cstdint>

namespace nemisis::dev {

struct DebugTargetState final {
    novacore::math::Vec3 position{0.0F, 1.65F, 18.0F};
    float radiusMeters = 0.85F;
    float maxHealth = 150.0F;
    float health = 150.0F;
    std::uint32_t hitsTaken = 0;
    bool eliminated = false;
};

struct DebugTargetHitResult final {
    bool hit = false;
    bool eliminated = false;
    float distanceMeters = 0.0F;
    float damageApplied = 0.0F;
    float healthRemaining = 0.0F;
};

void resetDebugTarget(DebugTargetState& target);

[[nodiscard]] DebugTargetHitResult applyShotToDebugTarget(
    DebugTargetState& target,
    const weapons::ShotTraceResult& shot);

} // namespace nemisis::dev
