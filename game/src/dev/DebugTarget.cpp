#include "nemisis/dev/DebugTarget.hpp"

#include <algorithm>
#include <cmath>

namespace nemisis::dev {

namespace {

[[nodiscard]] float dot(novacore::math::Vec3 lhs, novacore::math::Vec3 rhs) {
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}

} // namespace

void resetDebugTarget(DebugTargetState& target) {
    target.health = target.maxHealth;
    target.hitsTaken = 0;
    target.eliminated = false;
}

DebugTargetTraceResult traceShotToDebugTarget(
    const DebugTargetState& target,
    const weapons::ShotTraceResult& shot) {
    DebugTargetTraceResult result{};
    if (target.eliminated) {
        return result;
    }

    const auto originToTarget = shot.origin - target.position;
    const float b = dot(originToTarget, shot.direction);
    const float c = dot(originToTarget, originToTarget) - (target.radiusMeters * target.radiusMeters);
    const float discriminant = (b * b) - c;
    if (discriminant < 0.0F) {
        return result;
    }

    const float sqrtDiscriminant = std::sqrt(discriminant);
    float distance = -b - sqrtDiscriminant;
    if (distance < 0.0F) {
        distance = -b + sqrtDiscriminant;
    }
    if (distance < 0.0F || distance > shot.rangeMeters) {
        return result;
    }

    result.hit = true;
    result.distanceMeters = distance;
    return result;
}

DebugTargetHitResult applyShotToDebugTarget(
    DebugTargetState& target,
    const weapons::ShotTraceResult& shot) {
    DebugTargetHitResult result{};
    result.healthRemaining = target.health;

    const auto trace = traceShotToDebugTarget(target, shot);
    if (!trace.hit) {
        return result;
    }

    target.health = std::max(0.0F, target.health - shot.damage);
    ++target.hitsTaken;
    target.eliminated = target.health <= 0.0F;

    result.hit = true;
    result.eliminated = target.eliminated;
    result.distanceMeters = trace.distanceMeters;
    result.damageApplied = shot.damage;
    result.healthRemaining = target.health;
    return result;
}

} // namespace nemisis::dev
