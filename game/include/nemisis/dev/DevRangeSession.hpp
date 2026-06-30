#pragma once

#include "nemisis/dev/DebugTarget.hpp"
#include "nemisis/player/PlayerComponents.hpp"
#include "nemisis/player/PlayerHealth.hpp"
#include "nemisis/weapons/WeaponSimulation.hpp"

#include <array>
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

enum class DevRangeDrillVariant : std::uint8_t {
    Precision,
    RecoilControl,
    SpeedClear,
    Count
};

inline constexpr std::size_t kDevRangeDrillVariantCount =
    static_cast<std::size_t>(DevRangeDrillVariant::Count);

struct DevRangeDrillRules final {
    DevRangeDrillVariant variant = DevRangeDrillVariant::Precision;
    std::string_view name;
    std::string_view objectiveLabel;
    float timeLimitSeconds = 60.0F;
    float hitBasePoints = 90.0F;
    float damagePointScale = 1.0F;
    float streakPointScale = 8.0F;
    float eliminationBasePoints = 320.0F;
    float perfectClearBonus = 260.0F;
    float ttkBonusWindowSeconds = 2.2F;
    float ttkBonusScale = 80.0F;
    float recoilPointScale = 0.20F;
    float missPenaltyPoints = 0.0F;
    float minScoreMultiplier = 1.0F;
    float maxScoreMultiplier = 1.0F;
    float accuracyMultiplierScale = 0.0F;
    float recoilMultiplierScale = 0.0F;
    float speedMultiplierScale = 0.0F;
    float streakMultiplierScale = 0.0F;
    float recoilErrorSoftCapDegrees = 4.0F;
};

struct DevRangeDrillScoreTelemetry final {
    float scoreMultiplier = 1.0F;
    float accuracyFactor = 0.0F;
    float recoilFactor = 1.0F;
    float speedFactor = 1.0F;
    float streakFactor = 0.0F;
    float latestRawPoints = 0.0F;
    float latestBonusPoints = 0.0F;
    float latestScaledPoints = 0.0F;
    float latestPenaltyPoints = 0.0F;
    int latestScoreDelta = 0;
};

struct DevRangeRecoilHudTelemetry final {
    bool valid = false;
    bool hit = false;
    std::uint32_t shotIndex = 0;
    float pitchDegrees = 0.0F;
    float yawDegrees = 0.0F;
    float spreadDegrees = 0.0F;
    float errorDegrees = 0.0F;
    float averageErrorDegrees = 0.0F;
    float control01 = 1.0F;
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
    DevRangeDrillVariant variant = DevRangeDrillVariant::Precision;
    float timeLimitSeconds = 60.0F;
    float timeRemainingSeconds = 60.0F;
    float elapsedSeconds = 0.0F;
    std::uint32_t score = 0;
    std::uint32_t bestScore = 0;
    std::array<std::uint32_t, kDevRangeDrillVariantCount> bestScoreByVariant{};
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
    DevRangeDrillScoreTelemetry scoring{};
    DevRangeRecoilHudTelemetry recoilHud{};
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

void setDevRangeDrillVariant(
    DevRangeSessionState& session,
    DevRangeDrillVariant variant,
    const DevRangeSessionTuning& tuning = {});

void cycleDevRangeDrillVariant(
    DevRangeSessionState& session,
    const DevRangeSessionTuning& tuning = {});

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
[[nodiscard]] float devRangeDrillScoreMultiplier(const DevRangeDrillState& drill);
[[nodiscard]] float devRangeRecoilControl01(const DevRangeDrillState& drill);
[[nodiscard]] std::string_view devRangeDrillStatusName(DevRangeDrillStatus status);
[[nodiscard]] std::string_view devRangeDrillVariantName(DevRangeDrillVariant variant);
[[nodiscard]] std::string_view devRangeDrillObjectiveLabel(DevRangeDrillVariant variant);
[[nodiscard]] const DevRangeDrillRules& devRangeDrillRules(DevRangeDrillVariant variant);
[[nodiscard]] const DevRangeLaneScore* activeDevRangeLaneScore(
    const DevRangeSessionState& session,
    std::size_t laneIndex);

} // namespace nemisis::dev
