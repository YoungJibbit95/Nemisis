#pragma once

#include "nemisis/weapons/WeaponShot.hpp"
#include "nemisis/player/PlayerHealth.hpp"

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
    // Opt in only for the floor-rooted Facility mannequin; range targets keep spheres.
    bool humanoidHitZones = false;
};

struct DebugTargetHitResult final {
    bool hit = false;
    bool eliminated = false;
    float distanceMeters = 0.0F;
    float damageApplied = 0.0F;
    float healthRemaining = 0.0F;
};

struct DebugTargetTraceResult final {
    bool hit = false;
    float distanceMeters = 0.0F;
    player::DamageZone zone = player::DamageZone::Body;
};

void resetDebugTarget(DebugTargetState& target);

[[nodiscard]] DebugTargetTraceResult traceShotToDebugTarget(
    const DebugTargetState& target,
    const weapons::ShotTraceResult& shot);

[[nodiscard]] DebugTargetHitResult applyShotToDebugTarget(
    DebugTargetState& target,
    const weapons::ShotTraceResult& shot);

} // namespace nemisis::dev
