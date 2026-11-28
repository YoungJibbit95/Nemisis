#include "nemisis/dev/DebugTarget.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <cmath>

namespace nemisis::dev {

namespace {

[[nodiscard]] float dot(novacore::math::Vec3 lhs, novacore::math::Vec3 rhs) {
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}

// Facility mannequins render the normalized, 0.92-scale 1.786 m model at
// floor level. Keep the closest zone and its surface distance for cover ordering.
[[nodiscard]] std::optional<float> hitZone(
    const weapons::ShotTraceResult& shot, novacore::math::Vec3 center, novacore::math::Vec3 half) {
    float entry = 0.0F;
    float exit = shot.rangeMeters;
    const auto clip = [&](float origin, float direction, float c, float h) {
        if (std::abs(direction) < 1.0e-7F) return origin >= c - h && origin <= c + h;
        const float a = (c - h - origin) / direction;
        const float b = (c + h - origin) / direction;
        entry = std::max(entry, std::min(a, b));
        exit = std::min(exit, std::max(a, b));
        return entry <= exit;
    };
    if (shot.rangeMeters < 0 || !clip(shot.origin.x, shot.direction.x, center.x, half.x) ||
        !clip(shot.origin.y, shot.direction.y, center.y, half.y) ||
        !clip(shot.origin.z, shot.direction.z, center.z, half.z)) return std::nullopt;
    return entry;
}

[[nodiscard]] DebugTargetTraceResult traceHumanoid(
    const DebugTargetState& target, const weapons::ShotTraceResult& shot) {
    const auto feet = target.position - novacore::math::Vec3{0, 1.65F, 0};
    DebugTargetTraceResult nearest{};
    for (const auto& zone : humanoidHitZones) {
        if (const auto distance = hitZone(shot, feet + zone.center, zone.half);
            distance && (!nearest.hit || *distance < nearest.distanceMeters)) {
            nearest = {true, *distance, zone.type};
        }
    }
    return nearest;
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

    if (target.humanoidHitZones) return traceHumanoid(target, shot);

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
