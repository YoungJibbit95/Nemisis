#include "nemisis/net/PredictionHistory.hpp"

#include "novacore/math/Types.hpp"

#include <algorithm>
#include <cmath>

namespace nemisis::net {

namespace {

[[nodiscard]] float vec3Distance(novacore::math::Vec3 a, novacore::math::Vec3 b) {
    return novacore::math::length(a - b);
}

[[nodiscard]] float angleDeltaDegrees(float a, float b) {
    float delta = std::fmod(a - b, 360.0F);
    if (delta > 180.0F) {
        delta -= 360.0F;
    } else if (delta < -180.0F) {
        delta += 360.0F;
    }
    return std::abs(delta);
}

[[nodiscard]] std::uint64_t spanAfterAck(
    bool hasRecordedTick,
    std::uint64_t recordedTick,
    bool hasAcknowledgedTick,
    std::uint64_t acknowledgedTick) {
    if (!hasRecordedTick) {
        return 0;
    }
    if (!hasAcknowledgedTick || acknowledgedTick >= recordedTick) {
        return 0;
    }
    return recordedTick - acknowledgedTick;
}

} // namespace

PlayerPredictionHistory::PlayerPredictionHistory(float correctionThresholdMeters)
    : correctionThresholdMeters_(std::max(0.001F, correctionThresholdMeters)) {
}

bool PlayerPredictionHistory::record(PredictionSnapshot snapshot) {
    if (snapshot.tick == 0U) {
        snapshot.tick = snapshot.command.tick;
    }
    if (!snapshots_.store(snapshot.tick, snapshot)) {
        return false;
    }
    if (!hasRecordedTick_ || snapshot.tick > lastRecordedTick_) {
        lastRecordedTick_ = snapshot.tick;
        hasRecordedTick_ = true;
    }
    return true;
}

PredictionReconciliationResult PlayerPredictionHistory::acknowledgeThrough(std::uint64_t tick) {
    PredictionReconciliationResult result{};
    result.prunedSamples = snapshots_.eraseThrough(tick);
    if (!hasAcknowledgedTick_ || tick > lastAcknowledgedTick_) {
        lastAcknowledgedTick_ = tick;
        hasAcknowledgedTick_ = true;
    }
    return result;
}

PredictionReconciliationResult PlayerPredictionHistory::reconcile(
    const AuthoritativePlayerState& authoritativeState) {
    PredictionReconciliationResult result{};
    if (const auto* predicted = snapshots_.find(authoritativeState.tick); predicted != nullptr) {
        result.error = compare(*predicted, authoritativeState);
        latestError_ = result.error;
        hasLatestError_ = true;
    } else {
        result.error.tick = authoritativeState.tick;
        result.error.hasAuthoritativeState =
            authoritativeState.hasMovement ||
            authoritativeState.hasView ||
            authoritativeState.hasWeapon;
        latestError_ = result.error;
        hasLatestError_ = true;
    }

    result.prunedSamples = acknowledgeThrough(authoritativeState.tick).prunedSamples;
    return result;
}

void PlayerPredictionHistory::clear() {
    snapshots_.clear();
    latestError_ = {};
    lastRecordedTick_ = 0;
    lastAcknowledgedTick_ = 0;
    hasRecordedTick_ = false;
    hasAcknowledgedTick_ = false;
    hasLatestError_ = false;
}

const PredictionSnapshot* PlayerPredictionHistory::find(std::uint64_t tick) const {
    return snapshots_.find(tick);
}

PredictionHistoryStats PlayerPredictionHistory::stats() const {
    PredictionHistoryStats result{};
    result.capacity = snapshots_.capacity();
    result.storedSamples = snapshots_.size();
    result.lastRecordedTick = lastRecordedTick_;
    result.lastAcknowledgedTick = lastAcknowledgedTick_;
    result.unacknowledgedTickSpan = spanAfterAck(
        hasRecordedTick_,
        lastRecordedTick_,
        hasAcknowledgedTick_,
        lastAcknowledgedTick_);
    result.latestError = latestError_;
    result.hasRecordedTick = hasRecordedTick_;
    result.hasAcknowledgedTick = hasAcknowledgedTick_;
    result.hasLatestError = hasLatestError_;
    return result;
}

float PlayerPredictionHistory::correctionThresholdMeters() const {
    return correctionThresholdMeters_;
}

PredictionError PlayerPredictionHistory::compare(
    const PredictionSnapshot& predicted,
    const AuthoritativePlayerState& authoritativeState) const {
    PredictionError error{};
    error.tick = authoritativeState.tick;
    error.hasPrediction = true;
    error.hasAuthoritativeState =
        authoritativeState.hasMovement ||
        authoritativeState.hasView ||
        authoritativeState.hasWeapon;

    if (authoritativeState.hasMovement) {
        error.positionErrorMeters = vec3Distance(
            predicted.movement.position,
            authoritativeState.movement.position);
        error.velocityErrorMetersPerSecond = vec3Distance(
            predicted.movement.velocity,
            authoritativeState.movement.velocity);
    }
    if (authoritativeState.hasView) {
        error.yawErrorDegrees = angleDeltaDegrees(predicted.view.yawDegrees, authoritativeState.view.yawDegrees);
        error.pitchErrorDegrees = angleDeltaDegrees(predicted.view.pitchDegrees, authoritativeState.view.pitchDegrees);
    }
    if (authoritativeState.hasWeapon) {
        error.ammoDelta =
            static_cast<std::int32_t>(authoritativeState.weapon.ammoInMagazine) -
            static_cast<std::int32_t>(predicted.weapon.ammoInMagazine);
        error.shotIndexDelta =
            static_cast<std::int64_t>(authoritativeState.weapon.shotIndex) -
            static_cast<std::int64_t>(predicted.weapon.shotIndex);
    }

    error.exceedsCorrectionThreshold =
        error.positionErrorMeters > correctionThresholdMeters_ ||
        error.velocityErrorMetersPerSecond > 0.50F ||
        error.yawErrorDegrees > 2.0F ||
        error.pitchErrorDegrees > 2.0F ||
        error.ammoDelta != 0 ||
        error.shotIndexDelta != 0;
    return error;
}

AuthoritativePlayerState makeAuthoritativeState(
    std::uint64_t tick,
    const movement::PlayerMovementState* movement,
    const player::PlayerViewComponent* view,
    const weapons::WeaponRuntimeState* weapon) {
    AuthoritativePlayerState result{};
    result.tick = tick;
    if (movement != nullptr) {
        result.movement = *movement;
        result.hasMovement = true;
    }
    if (view != nullptr) {
        result.view = *view;
        result.hasView = true;
    }
    if (weapon != nullptr) {
        result.weapon = *weapon;
        result.hasWeapon = true;
    }
    return result;
}

} // namespace nemisis::net
