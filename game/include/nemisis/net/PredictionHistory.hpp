#pragma once

#include "nemisis/movement/MovementSystem.hpp"
#include "nemisis/player/PlayerAnimation.hpp"
#include "nemisis/player/PlayerCameraRig.hpp"
#include "nemisis/player/PlayerComponents.hpp"
#include "nemisis/player/PlayerInputCommand.hpp"
#include "nemisis/weapons/WeaponShot.hpp"
#include "nemisis/weapons/WeaponTypes.hpp"

#include "novacore/net/SequenceBuffer.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace nemisis::net {

inline constexpr std::size_t kPredictionHistoryCapacity = 256;

struct PredictionSnapshot final {
    std::uint64_t tick = 0;
    player::PlayerInputCommand command{};
    movement::PlayerMovementState movement{};
    player::PlayerViewComponent view{};
    weapons::WeaponRuntimeState weapon{};
    weapons::FireResult fire{};
    weapons::ShotTraceResult shot{};
    player::CameraRigState camera{};
    player::CharacterAnimationFrame animation{};
    bool hasShot = false;
    bool hasCamera = false;
    bool hasAnimation = false;
};

struct AuthoritativePlayerState final {
    std::uint64_t tick = 0;
    movement::PlayerMovementState movement{};
    player::PlayerViewComponent view{};
    weapons::WeaponRuntimeState weapon{};
    bool hasMovement = false;
    bool hasView = false;
    bool hasWeapon = false;
};

struct PredictionError final {
    std::uint64_t tick = 0;
    float positionErrorMeters = 0.0F;
    float velocityErrorMetersPerSecond = 0.0F;
    float yawErrorDegrees = 0.0F;
    float pitchErrorDegrees = 0.0F;
    std::int32_t ammoDelta = 0;
    std::int64_t shotIndexDelta = 0;
    bool hasPrediction = false;
    bool hasAuthoritativeState = false;
    bool exceedsCorrectionThreshold = false;
};

struct PredictionReconciliationResult final {
    std::size_t prunedSamples = 0;
    PredictionError error{};
};

struct PredictionHistoryStats final {
    std::size_t capacity = kPredictionHistoryCapacity;
    std::size_t storedSamples = 0;
    std::uint64_t lastRecordedTick = 0;
    std::uint64_t lastAcknowledgedTick = 0;
    std::uint64_t unacknowledgedTickSpan = 0;
    PredictionError latestError{};
    bool hasRecordedTick = false;
    bool hasAcknowledgedTick = false;
    bool hasLatestError = false;
};

class PlayerPredictionHistory final {
public:
    explicit PlayerPredictionHistory(float correctionThresholdMeters = 0.16F);

    [[nodiscard]] bool record(PredictionSnapshot snapshot);
    [[nodiscard]] PredictionReconciliationResult acknowledgeThrough(std::uint64_t tick);
    [[nodiscard]] PredictionReconciliationResult reconcile(
        const AuthoritativePlayerState& authoritativeState);

    void clear();

    [[nodiscard]] const PredictionSnapshot* find(std::uint64_t tick) const;
    [[nodiscard]] PredictionHistoryStats stats() const;
    [[nodiscard]] float correctionThresholdMeters() const;

private:
    [[nodiscard]] PredictionError compare(
        const PredictionSnapshot& predicted,
        const AuthoritativePlayerState& authoritativeState) const;

    novacore::net::SequenceBuffer<PredictionSnapshot, kPredictionHistoryCapacity> snapshots_;
    float correctionThresholdMeters_ = 0.16F;
    PredictionError latestError_{};
    std::uint64_t lastRecordedTick_ = 0;
    std::uint64_t lastAcknowledgedTick_ = 0;
    bool hasRecordedTick_ = false;
    bool hasAcknowledgedTick_ = false;
    bool hasLatestError_ = false;
};

[[nodiscard]] AuthoritativePlayerState makeAuthoritativeState(
    std::uint64_t tick,
    const movement::PlayerMovementState* movement,
    const player::PlayerViewComponent* view,
    const weapons::WeaponRuntimeState* weapon);

} // namespace nemisis::net
