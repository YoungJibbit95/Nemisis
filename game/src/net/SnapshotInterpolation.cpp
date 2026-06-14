#include "nemisis/net/SnapshotInterpolation.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace nemisis::net {

namespace {

[[nodiscard]] float lerp(float a, float b, float t) {
    return a + ((b - a) * t);
}

[[nodiscard]] novacore::math::Vec3 lerpVec3(
    novacore::math::Vec3 a,
    novacore::math::Vec3 b,
    float t) {
    return {
        lerp(a.x, b.x, t),
        lerp(a.y, b.y, t),
        lerp(a.z, b.z, t),
    };
}

[[nodiscard]] float shortestAngleDelta(float from, float to) {
    float delta = std::fmod(to - from, 360.0F);
    if (delta > 180.0F) {
        delta -= 360.0F;
    } else if (delta < -180.0F) {
        delta += 360.0F;
    }
    return delta;
}

[[nodiscard]] float lerpAngle(float from, float to, float t) {
    return from + (shortestAngleDelta(from, to) * t);
}

[[nodiscard]] weapons::WeaponRuntimeState chooseWeapon(
    const weapons::WeaponRuntimeState& from,
    const weapons::WeaponRuntimeState& to,
    float alpha) {
    weapons::WeaponRuntimeState result = alpha >= 0.5F ? to : from;
    if (from.weaponId == to.weaponId) {
        result.weaponId = from.weaponId;
        result.adsAlpha = lerp(from.adsAlpha, to.adsAlpha, alpha);
        result.recoilPitchOffsetDegrees = lerp(from.recoilPitchOffsetDegrees, to.recoilPitchOffsetDegrees, alpha);
        result.recoilYawOffsetDegrees = lerp(from.recoilYawOffsetDegrees, to.recoilYawOffsetDegrees, alpha);
        result.reloadProgress = lerp(from.reloadProgress, to.reloadProgress, alpha);
        result.timeSinceLastShotSeconds = lerp(from.timeSinceLastShotSeconds, to.timeSinceLastShotSeconds, alpha);
    }
    return result;
}

[[nodiscard]] movement::MovementMode chooseMode(
    movement::MovementMode from,
    movement::MovementMode to,
    float alpha) {
    return alpha >= 0.5F ? to : from;
}

} // namespace

bool PlayerSnapshotInterpolator::store(PlayerSnapshotState snapshot) {
    return snapshots_.store(snapshot.serverTick, std::move(snapshot));
}

std::optional<PlayerSnapshotState> PlayerSnapshotInterpolator::find(std::uint64_t serverTick) const {
    if (const auto* snapshot = snapshots_.find(serverTick); snapshot != nullptr) {
        return *snapshot;
    }
    return std::nullopt;
}

InterpolatedPlayerSnapshot PlayerSnapshotInterpolator::sample(std::uint64_t renderTick) const {
    return interpolate(renderTick, snapshots_.sample(renderTick));
}

InterpolatedPlayerSnapshot PlayerSnapshotInterpolator::sampleDelayed(
    std::uint64_t newestServerTick,
    std::uint64_t delayTicks) const {
    const auto renderTick = newestServerTick > delayTicks ? newestServerTick - delayTicks : 0U;
    return sample(renderTick);
}

std::size_t PlayerSnapshotInterpolator::pruneThrough(std::uint64_t serverTick) {
    return snapshots_.eraseThrough(serverTick);
}

std::size_t PlayerSnapshotInterpolator::rollbackAfter(std::uint64_t serverTick) {
    return snapshots_.eraseAfter(serverTick);
}

void PlayerSnapshotInterpolator::clear() {
    snapshots_.clear();
}

PlayerSnapshotInterpolationStats PlayerSnapshotInterpolator::stats() const {
    PlayerSnapshotInterpolationStats stats{};
    stats.storedSnapshots = snapshots_.size();
    if (const auto newest = snapshots_.newestSequence(); newest.has_value()) {
        stats.newestServerTick = *newest;
        stats.hasNewestServerTick = true;
    }
    if (const auto oldest = snapshots_.oldestSequence(); oldest.has_value()) {
        stats.oldestServerTick = *oldest;
        stats.hasOldestServerTick = true;
    }
    return stats;
}

InterpolatedPlayerSnapshot PlayerSnapshotInterpolator::interpolate(
    std::uint64_t renderTick,
    const novacore::net::InterpolationBuffer<
        PlayerSnapshotState,
        kPlayerSnapshotInterpolationCapacity>::Sample& sample) const {
    InterpolatedPlayerSnapshot result{};
    result.renderTick = renderTick;
    if (!sample.valid()) {
        return result;
    }

    const auto& from = *sample.from;
    const auto& to = *sample.to;
    const float alpha = std::clamp(sample.alpha, 0.0F, 1.0F);
    result.playerId = from.playerId != 0U ? from.playerId : to.playerId;
    result.fromTick = sample.fromSequence;
    result.toTick = sample.toSequence;
    result.alpha = alpha;
    result.valid = true;
    result.exact = sample.exact;
    result.clamped = sample.clampedToNewest || sample.clampedToOldest;

    result.state.tick = renderTick;
    result.state.hasMovement = from.state.hasMovement && to.state.hasMovement;
    result.state.hasView = from.state.hasView && to.state.hasView;
    result.state.hasWeapon = from.state.hasWeapon && to.state.hasWeapon;
    if (result.state.hasMovement) {
        result.state.movement = alpha >= 0.5F ? to.state.movement : from.state.movement;
        result.state.movement.position = lerpVec3(from.state.movement.position, to.state.movement.position, alpha);
        result.state.movement.velocity = lerpVec3(from.state.movement.velocity, to.state.movement.velocity, alpha);
        result.state.movement.wallRunNormal = lerpVec3(from.state.movement.wallRunNormal, to.state.movement.wallRunNormal, alpha);
        result.state.movement.wallRunTangent = lerpVec3(from.state.movement.wallRunTangent, to.state.movement.wallRunTangent, alpha);
        result.state.movement.lastHorizontalSpeed = lerp(from.state.movement.lastHorizontalSpeed, to.state.movement.lastHorizontalSpeed, alpha);
        result.state.movement.inputMagnitude = lerp(from.state.movement.inputMagnitude, to.state.movement.inputMagnitude, alpha);
        result.state.movement.mode = chooseMode(from.state.movement.mode, to.state.movement.mode, alpha);
    }
    if (result.state.hasView) {
        result.state.view.yawDegrees = lerpAngle(from.state.view.yawDegrees, to.state.view.yawDegrees, alpha);
        result.state.view.pitchDegrees = lerp(from.state.view.pitchDegrees, to.state.view.pitchDegrees, alpha);
    }
    if (result.state.hasWeapon) {
        result.state.weapon = chooseWeapon(from.state.weapon, to.state.weapon, alpha);
    }
    return result;
}

} // namespace nemisis::net
