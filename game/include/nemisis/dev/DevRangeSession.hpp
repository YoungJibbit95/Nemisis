#pragma once

#include "nemisis/dev/DebugTarget.hpp"
#include "nemisis/player/PlayerComponents.hpp"
#include "nemisis/player/PlayerHealth.hpp"
#include "nemisis/weapons/WeaponSimulation.hpp"

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace nemisis::dev {

enum class DevRangeDrillStatus {
    Active,
    Complete
};

struct DevRangeScoreboard final {
    std::uint32_t shotsFired = 0;
    std::uint32_t shotsHit = 0;
    std::uint32_t targetsEliminated = 0;
    std::uint32_t rangeResets = 0;
    std::uint32_t playerRespawns = 0;
    std::uint32_t currentStreak = 0;
    std::uint32_t bestStreak = 0;
    float damageDealt = 0.0F;
};

struct DevRangeLaneScore final {
    std::size_t laneIndex = 0;
    std::string laneId;
    std::string laneName;
    std::uint32_t shotsFired = 0;
    std::uint32_t shotsHit = 0;
    std::uint32_t targetsEliminated = 0;
    std::uint32_t currentTargetShots = 0;
    std::uint32_t currentTargetHits = 0;
    float damageDealt = 0.0F;
    float currentTargetDamage = 0.0F;
    float currentTargetFirstHitSeconds = -1.0F;
    float latestTtkSeconds = -1.0F;
    float bestTtkSeconds = -1.0F;
    float latestDistanceMeters = 0.0F;
};

struct DevRangeDrillState final {
    DevRangeDrillStatus status = DevRangeDrillStatus::Active;
    float timeLimitSeconds = 60.0F;
    float timeRemainingSeconds = 60.0F;
    float elapsedSeconds = 0.0F;
    std::uint32_t score = 0;
    std::uint32_t bestScore = 0;
    std::uint32_t shotsFired = 0;
    std::uint32_t shotsHit = 0;
    std::uint32_t targetsEliminated = 0;
    std::uint32_t misses = 0;
    std::uint32_t perfectLaneClears = 0;
    float latestTtkSeconds = -1.0F;
    float bestTtkSeconds = -1.0F;
    float latestRecoilErrorDegrees = 0.0F;
    float averageRecoilErrorDegrees = 0.0F;
    float recoilControlScore = 100.0F;
};

struct DevRangeSessionState final {
    DevRangeScoreboard score{};
    DevRangeDrillState drill{};
    std::vector<DevRangeLaneScore> laneScores;
    float targetRespawnSeconds = 0.0F;
    float playerRespawnSeconds = 0.0F;
    float eventTextSeconds = 0.0F;
    float playerRegenDelaySeconds = 0.0F;
    std::string eventText;
};

struct DevRangeSessionTuning final {
    float targetRespawnDelaySeconds = 1.5F;
    float playerRespawnDelaySeconds = 1.75F;
    float eventTextSeconds = 1.4F;
    float playerRegenDelaySeconds = 3.0F;
    float playerRegenPerSecond = 12.0F;
    float drillTimeLimitSeconds = 60.0F;
};

struct DevRangeShotScoreContext final {
    bool laneKnown = false;
    std::size_t laneIndex = 0;
    std::string_view laneId;
    std::string_view laneName;
    float targetMaxHealth = 150.0F;
    float distanceMeters = 0.0F;
};

void resetDevRangeSession(DevRangeSessionState& session);

void recordRangeReset(DevRangeSessionState& session, const DevRangeSessionTuning& tuning = {});

void ensureDevRangeLaneScore(
    DevRangeSessionState& session,
    std::size_t laneIndex,
    std::string_view laneId,
    std::string_view laneName);

void recordShotResult(
    DevRangeSessionState& session,
    const weapons::FireResult& fire,
    const DebugTargetHitResult& hit,
    const DevRangeSessionTuning& tuning = {});

void recordShotResult(
    DevRangeSessionState& session,
    const weapons::FireResult& fire,
    const DebugTargetHitResult& hit,
    const DevRangeShotScoreContext& context,
    const DevRangeSessionTuning& tuning = {});

void beginTargetRespawn(DevRangeSessionState& session, const DevRangeSessionTuning& tuning = {});

[[nodiscard]] bool tickTargetRespawn(
    DevRangeSessionState& session,
    float deltaSeconds);

void recordPlayerDamage(
    DevRangeSessionState& session,
    const player::DamageResult& damage,
    const DevRangeSessionTuning& tuning = {});

void beginPlayerRespawn(
    DevRangeSessionState& session,
    player::PlayerHealthComponent& health,
    const DevRangeSessionTuning& tuning = {});

[[nodiscard]] bool tickPlayerRespawn(
    DevRangeSessionState& session,
    player::PlayerHealthComponent& health,
    float deltaSeconds);

void tickPlayerRegen(
    DevRangeSessionState& session,
    player::PlayerHealthComponent& health,
    float deltaSeconds,
    const DevRangeSessionTuning& tuning = {});

void tickDevRangeDrill(
    DevRangeSessionState& session,
    float deltaSeconds,
    const DevRangeSessionTuning& tuning = {});

void tickSessionFeedback(DevRangeSessionState& session, float deltaSeconds);

[[nodiscard]] float devRangeAccuracy(const DevRangeScoreboard& score);
[[nodiscard]] float devRangeDrillAccuracy(const DevRangeDrillState& drill);
[[nodiscard]] float devRangeLaneAccuracy(const DevRangeLaneScore& lane);
[[nodiscard]] float devRangeDrillProgress(const DevRangeDrillState& drill);
[[nodiscard]] std::string_view devRangeDrillStatusName(DevRangeDrillStatus status);
[[nodiscard]] const DevRangeLaneScore* activeDevRangeLaneScore(
    const DevRangeSessionState& session,
    std::size_t laneIndex);

} // namespace nemisis::dev
